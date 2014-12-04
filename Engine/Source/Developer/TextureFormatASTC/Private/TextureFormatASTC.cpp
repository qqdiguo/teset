// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "CoreMisc.h"
#include "ImageCore.h"
#include "ImageWrapper.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "TargetPlatform.h"
#include "TextureCompressorModule.h"
#include "PixelFormat.h"
#include "GenericPlatformProcess.h"


#define MAX_QUALITY_BY_SIZE 4
#define MAX_QUALITY_BY_SPEED 3
#define FORCED_NORMAL_MAP_COMPRESSION_SIZE_VALUE 1


DEFINE_LOG_CATEGORY_STATIC(LogTextureFormatASTC, Log, All);

/**
 * Macro trickery for supported format names.
 */
#define ENUM_SUPPORTED_FORMATS(op) \
	op(ASTC_RGB) \
	op(ASTC_RGBA) \
	op(ASTC_RGBAuto) \
	op(ASTC_NormalAG) \
	op(ASTC_NormalRG)

	#define DECL_FORMAT_NAME(FormatName) static FName GTextureFormatName##FormatName = FName(TEXT(#FormatName));
		ENUM_SUPPORTED_FORMATS(DECL_FORMAT_NAME);
	#undef DECL_FORMAT_NAME

	#define DECL_FORMAT_NAME_ENTRY(FormatName) GTextureFormatName##FormatName ,
		static FName GSupportedTextureFormatNames[] =
		{
			ENUM_SUPPORTED_FORMATS(DECL_FORMAT_NAME_ENTRY)
		};
	#undef DECL_FORMAT_NAME_ENTRY
#undef ENUM_SUPPORTED_FORMATS

// ASTC file header format
#if PLATFORM_SUPPORTS_PRAGMA_PACK
	#pragma pack(push, 4)
#endif

	#define ASTC_MAGIC_CONSTANT 0x5CA1AB13
	struct FASTCHeader
	{
		uint32 Magic;
		uint8  BlockSizeX;
		uint8  BlockSizeY;
		uint8  BlockSizeZ;
		uint8  TexelCountX[3];
		uint8  TexelCountY[3];
		uint8  TexelCountZ[3];
	};

#if PLATFORM_SUPPORTS_PRAGMA_PACK
	#pragma pack(pop)
#endif

IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));


static int32 GetDefaultCompressionBySizeValue()
{
	// start at default quality, then lookup in .ini file
	int32 CompressionModeValue = 0;
	GConfig->GetInt(TEXT("/Script/UnrealEd.CookerSettings"), TEXT("DefaultASTCQualityBySize"), CompressionModeValue, GEngineIni);
	
	FParse::Value(FCommandLine::Get(), TEXT("-astcqualitybysize="), CompressionModeValue);
	CompressionModeValue = FMath::Min<uint32>(CompressionModeValue, MAX_QUALITY_BY_SIZE);
	
	return CompressionModeValue;
}

static int32 GetDefaultCompressionBySpeedValue()
{
	// start at default quality, then lookup in .ini file
	int32 CompressionModeValue = 0;
	GConfig->GetInt(TEXT("/Script/UnrealEd.CookerSettings"), TEXT("DefaultASTCQualityBySpeed"), CompressionModeValue, GEngineIni);
	
	FParse::Value(FCommandLine::Get(), TEXT("-astcqualitybyspeed="), CompressionModeValue);
	CompressionModeValue = FMath::Min<uint32>(CompressionModeValue, MAX_QUALITY_BY_SPEED);
	
	return CompressionModeValue;
}

static FString GetQualityString(int32 OverrideSizeValue=-1, int32 OverrideSpeedValue=-1)
{
	// convert to a string
	FString CompressionMode;
	switch (OverrideSizeValue >= 0 ? OverrideSizeValue : GetDefaultCompressionBySizeValue())
	{
		case 0:	CompressionMode = TEXT("12x12"); break;
		case 1:	CompressionMode = TEXT("10x10"); break;
		case 2:	CompressionMode = TEXT("8x8"); break;
		case 3:	CompressionMode = TEXT("6x6"); break;
		case 4:	CompressionMode = TEXT("4x4"); break;
		default: UE_LOG(LogTemp, Fatal, TEXT("ASTC size quality higher than expected"));
	}
	
	switch (OverrideSpeedValue >= 0 ? OverrideSpeedValue : GetDefaultCompressionBySpeedValue())
	{
		case 0:	CompressionMode += TEXT(" -veryfast"); break;
		case 1:	CompressionMode += TEXT(" -fast"); break;
		case 2:	CompressionMode += TEXT(" -medium"); break;
		case 3:	CompressionMode += TEXT(" -thorough"); break;
		default: UE_LOG(LogTemp, Fatal, TEXT("ASTC speed quality higher than expected"));
	}

	return CompressionMode;
}

static EPixelFormat GetQualityFormat(int32 OverrideSizeValue=-1)
{
	// convert to a string
	EPixelFormat Format = PF_Unknown;
	switch (OverrideSizeValue >= 0 ? OverrideSizeValue : GetDefaultCompressionBySizeValue())
	{
		case 0:	Format = PF_ASTC_12x12; break;
		case 1:	Format = PF_ASTC_10x10; break;
		case 2:	Format = PF_ASTC_8x8; break;
		case 3:	Format = PF_ASTC_6x6; break;
		case 4:	Format = PF_ASTC_4x4; break;
		default: UE_LOG(LogTemp, Fatal, TEXT("Max quality higher than expected"));
	}
	
	return Format;
}

static uint16 GetQualityVersion()
{
	// top 3 bits for size compression value, and next 3 for speed
	return (GetDefaultCompressionBySizeValue() << 13) | (GetDefaultCompressionBySpeedValue() << 10);
}


static bool CompressSliceToASTC(
	const void* SourceData,
	int32 SizeX,
	int32 SizeY,
	FString CompressionParameters,
	TArray<uint8>& OutCompressedData
)
{
	// Always Y-invert the image prior to compression for proper orientation post-compression
	uint8 LineBuffer[16384 * 4];
	uint32 LineSize = SizeX * 4;
	for (int32 LineIndex = 0; LineIndex < (SizeY / 2); LineIndex++)
	{
		uint8* LineData0 = ((uint8*)SourceData) + (LineSize * LineIndex);
		uint8* LineData1 = ((uint8*)SourceData) + (LineSize * (SizeY - LineIndex - 1));
		FMemory::Memcpy(LineBuffer, LineData0,  LineSize);
		FMemory::Memcpy(LineData0,  LineData1,  LineSize);
		FMemory::Memcpy(LineData1,  LineBuffer, LineSize);
	}
	
	// Compress and retrieve the PNG data to write out to disk
	IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	ImageWrapper->SetRaw(SourceData, SizeX * SizeY * 4, SizeX, SizeY, ERGBFormat::RGBA, 8);
	const TArray<uint8>& FileData = ImageWrapper->GetCompressed();
	int32 FileDataSize = FileData.Num();

	FGuid Guid;
	FPlatformMisc::CreateGuid(Guid);
	FString InputFilePath = FString::Printf(TEXT("Cache/%08x-%08x-%08x-%08x-RGBToASTCIn.png"), Guid.A, Guid.B, Guid.C, Guid.D);
	FString OutputFilePath = FString::Printf(TEXT("Cache/%08x-%08x-%08x-%08x-RGBToASTCOut.astc"), Guid.A, Guid.B, Guid.C, Guid.D);

	InputFilePath  = FPaths::GameIntermediateDir() + InputFilePath;
	OutputFilePath = FPaths::GameIntermediateDir() + OutputFilePath;

	FArchive* PNGFile = NULL;
	while (!PNGFile)
	{
		PNGFile = IFileManager::Get().CreateFileWriter(*InputFilePath);   // Occasionally returns NULL due to error code ERROR_SHARING_VIOLATION
		FPlatformProcess::Sleep(0.01f);                             // ... no choice but to wait for the file to become free to access
	}
	PNGFile->Serialize((void*)&FileData[0], FileDataSize);
	delete PNGFile;

	// Compress PNG file to ASTC (using the reference astcenc.exe from ARM)
	FString Params = FString::Printf(TEXT("-c %s %s %s"),
		*InputFilePath,
		*OutputFilePath,
		*CompressionParameters
	);

	UE_LOG(LogTextureFormatASTC, Display, TEXT("Compressing to ASTC (%s)..."), *CompressionParameters);

	// Start Compressor
#if PLATFORM_MAC
	FString CompressorPath(FPaths::EngineDir() + TEXT("Binaries/ThirdParty/ARM/Mac/astcenc"));
#elif PLATFORM_LINUX
	FString CompressorPath(FPaths::EngineDir() + TEXT("Binaries/ThirdParty/ARM/Linux32/astcenc"));
#elif PLATFORM_WINDOWS
	FString CompressorPath(FPaths::EngineDir() + TEXT("Binaries/ThirdParty/ARM/Win32/astcenc.exe"));
#else
#error Unsupported platform
#endif
	FProcHandle Proc = FPlatformProcess::CreateProc(*CompressorPath, *Params, true, false, false, NULL, -1, NULL, NULL);

	// Failed to start the compressor process
	if (!Proc.IsValid())
	{
		UE_LOG(LogTextureFormatASTC, Error, TEXT("Failed to start astcenc for compressing images (%s)"), *CompressorPath);
		return false;
	}

	// Wait for the process to complete
	int ReturnCode;
	while (!FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode))
	{
		FPlatformProcess::Sleep(0.01f);
	}

	// Did it work?
	bool bConversionWasSuccessful = (ReturnCode == 0);

	// Open compressed file and put the data in OutCompressedImage
	if (bConversionWasSuccessful)
	{
		// Get raw file data
		TArray<uint8> ASTCData;
		FFileHelper::LoadFileToArray(ASTCData, *OutputFilePath);
			
		// Process it
		FASTCHeader* Header = (FASTCHeader*)ASTCData.GetData();
			
		// Fiddle with the texel count data to get the right value
		uint32 TexelCountX =
			(Header->TexelCountX[0] <<  0) + 
			(Header->TexelCountX[1] <<  8) + 
			(Header->TexelCountX[2] << 16);
		uint32 TexelCountY =
			(Header->TexelCountY[0] <<  0) + 
			(Header->TexelCountY[1] <<  8) + 
			(Header->TexelCountY[2] << 16);
		uint32 TexelCountZ =
			(Header->TexelCountZ[0] <<  0) + 
			(Header->TexelCountZ[1] <<  8) + 
			(Header->TexelCountZ[2] << 16);

//		UE_LOG(LogTextureFormatASTC, Display, TEXT("    Compressed Texture Header:"));
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("             Magic: %x"), Header->Magic);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("        BlockSizeX: %u"), Header->BlockSizeX);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("        BlockSizeY: %u"), Header->BlockSizeY);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("        BlockSizeZ: %u"), Header->BlockSizeZ);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("       TexelCountX: %u"), TexelCountX);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("       TexelCountY: %u"), TexelCountY);
//		UE_LOG(LogTextureFormatASTC, Display, TEXT("       TexelCountZ: %u"), TexelCountZ);

		// Calculate size of this mip in blocks
		uint32 MipSizeX = (TexelCountX + Header->BlockSizeX - 1) / Header->BlockSizeX;
		uint32 MipSizeY = (TexelCountY + Header->BlockSizeY - 1) / Header->BlockSizeY;

		// A block is always 16 bytes
		uint32 MipSize = MipSizeX * MipSizeY * 16;

		// Copy the compressed data
		OutCompressedData.Empty(MipSize);
		OutCompressedData.AddUninitialized(MipSize);
		void* MipData = OutCompressedData.GetData();

		// Calculate the offset to get to the mip data
		check(sizeof(FASTCHeader) == 16);
		check(ASTCData.Num() == (sizeof(FASTCHeader) + MipSize));
		FMemory::Memcpy(MipData, ASTCData.GetData() + sizeof(FASTCHeader), MipSize);
	}
	else
	{
		UE_LOG(LogTextureFormatASTC, Error, TEXT("ASTC encoder failed with return code %d, mip size (%d, %d)"), ReturnCode, SizeX, SizeY);
		IFileManager::Get().Delete(*InputFilePath);
		IFileManager::Get().Delete(*OutputFilePath);
		return false;
	}
		
	// Delete intermediate files
	IFileManager::Get().Delete(*InputFilePath);
	IFileManager::Get().Delete(*OutputFilePath);
	return true;
}

/**
 * ASTC texture format handler.
 */
class FTextureFormatASTC : public ITextureFormat
{
	virtual bool AllowParallelBuild() const OVERRIDE
	{
		return true;
	}

	virtual uint16 GetVersion(FName Format) const OVERRIDE
	{
		return GetQualityVersion() + 29;
	}

	virtual FTextureFormatCompressorCaps GetFormatCapabilities() const override
	{
		FTextureFormatCompressorCaps RetCaps;
		// use defaults
		return RetCaps;
	}

	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const OVERRIDE
	{
		for (int32 i = 0; i < ARRAY_COUNT(GSupportedTextureFormatNames); ++i)
		{
			OutFormats.Add(GSupportedTextureFormatNames[i]);
		}
	}

	virtual bool CompressImage(
			const FImage& InImage,
			const struct FTextureBuildSettings& BuildSettings,
			bool bImageHasAlphaChannel,
			FCompressedImage2D& OutCompressedImage
		) const OVERRIDE
	{
		// Get Raw Image Data from passed in FImage
		FImage Image;
		InImage.CopyTo(Image, ERawImageFormat::BGRA8, BuildSettings.bSRGB);

		// Determine the compressed pixel format and compression parameters
		EPixelFormat CompressedPixelFormat = PF_Unknown;
		FString CompressionParameters = TEXT("");

		if (BuildSettings.TextureFormatName == GTextureFormatNameASTC_RGB ||
		  ((BuildSettings.TextureFormatName == GTextureFormatNameASTC_RGBAuto) && !bImageHasAlphaChannel))
		{
			CompressedPixelFormat = GetQualityFormat();
			CompressionParameters = FString::Printf(TEXT("%s -esw bgra -ch 1 1 1 0"), *GetQualityString());
		}
		else if (BuildSettings.TextureFormatName == GTextureFormatNameASTC_RGBA ||
			   ((BuildSettings.TextureFormatName == GTextureFormatNameASTC_RGBAuto) && bImageHasAlphaChannel))
		{
			CompressedPixelFormat = GetQualityFormat();
			CompressionParameters = FString::Printf(TEXT("%s -esw bgra -ch 1 1 1 1 -alphablend"), *GetQualityString());
		}
		else if (BuildSettings.TextureFormatName == GTextureFormatNameASTC_NormalAG)
		{
			CompressedPixelFormat = GetQualityFormat(FORCED_NORMAL_MAP_COMPRESSION_SIZE_VALUE);
			CompressionParameters = FString::Printf(TEXT("%s -esw 0g0b -ch 0 1 0 1 -oplimit 1000 -mincorrel 0.99 -dblimit 60 -b 2.5 -v 3 1 1 0 50 0 -va 1 1 0 50"), *GetQualityString(FORCED_NORMAL_MAP_COMPRESSION_SIZE_VALUE, -1));
		}
		else if (BuildSettings.TextureFormatName == GTextureFormatNameASTC_NormalRG)
		{
			CompressedPixelFormat = GetQualityFormat(FORCED_NORMAL_MAP_COMPRESSION_SIZE_VALUE);
			CompressionParameters = FString::Printf(TEXT("%s -esw bg00 -ch 1 1 0 0 -oplimit 1000 -mincorrel 0.99 -dblimit 60 -b 2.5 -v 3 1 1 0 50 0 -va 1 1 0 50"), *GetQualityString(FORCED_NORMAL_MAP_COMPRESSION_SIZE_VALUE, -1));
		}

		// Compress the image, slice by slice
		bool bCompressionSucceeded = true;
		int32 SliceSizeInTexels = Image.SizeX * Image.SizeY;
		for (int32 SliceIndex = 0; SliceIndex < Image.NumSlices && bCompressionSucceeded; ++SliceIndex)
		{
			TArray<uint8> CompressedSliceData;
			bCompressionSucceeded = CompressSliceToASTC(
				Image.AsBGRA8() + (SliceIndex * SliceSizeInTexels),
				Image.SizeX,
				Image.SizeY,
				CompressionParameters,
				CompressedSliceData
			);
			OutCompressedImage.RawData.Append(CompressedSliceData);
		}

		if (bCompressionSucceeded)
		{
			OutCompressedImage.SizeX = Image.SizeX;
			OutCompressedImage.SizeY = Image.SizeY;
			OutCompressedImage.PixelFormat = CompressedPixelFormat;
		}
		return bCompressionSucceeded;
	}
};

/**
 * Module for ASTC texture compression.
 */
static ITextureFormat* Singleton = NULL;

class FTextureFormatASTCModule : public ITextureFormatModule
{
public:
	virtual ~FTextureFormatASTCModule()
	{
		delete Singleton;
		Singleton = NULL;
	}
	virtual ITextureFormat* GetTextureFormat()
	{
		if (!Singleton)
		{
			Singleton = new FTextureFormatASTC();
		}
		return Singleton;
	}
};

IMPLEMENT_MODULE(FTextureFormatASTCModule, TextureFormatASTC);
