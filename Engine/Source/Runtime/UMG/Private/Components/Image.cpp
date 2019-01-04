// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Components/Image.h"
#include "Slate/SlateBrushAsset.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UImage

UImage::UImage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ColorAndOpacity(FLinearColor::White)
{
}

#if WITH_EDITORONLY_DATA
void UImage::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS && Image_DEPRECATED != nullptr )
	{
		Brush = Image_DEPRECATED->Brush;
		Image_DEPRECATED = nullptr;
	}
}
#endif

void UImage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyImage.Reset();
}

TSharedRef<SWidget> UImage::RebuildWidget()
{
	MyImage = SNew(SImage);
	return MyImage.ToSharedRef();
}

void UImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<FSlateColor> ColorAndOpacityBinding = PROPERTY_BINDING(FSlateColor, ColorAndOpacity);
	TAttribute<const FSlateBrush*> ImageBinding = OPTIONAL_BINDING_CONVERT(FSlateBrush, Brush, const FSlateBrush*, ConvertImage);

	if (MyImage.IsValid())
	{
		MyImage->SetImage(ImageBinding);
		MyImage->SetColorAndOpacity(ColorAndOpacityBinding);
		MyImage->SetOnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown));
	}
}

void UImage::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	if ( MyImage.IsValid() )
	{
		MyImage->SetColorAndOpacity(ColorAndOpacity);
	}
}

void UImage::SetOpacity(float InOpacity)
{
	ColorAndOpacity.A = InOpacity;
	if ( MyImage.IsValid() )
	{
		MyImage->SetColorAndOpacity(ColorAndOpacity);
	}
}

const FSlateBrush* UImage::ConvertImage(TAttribute<FSlateBrush> InImageAsset) const
{
	UImage* MutableThis = const_cast<UImage*>( this );
	MutableThis->Brush = InImageAsset.Get();

	return &Brush;
}

void UImage::SetBrush(const FSlateBrush& InBrush)
{
	if(Brush != InBrush)
	{
		Brush = InBrush;

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushSize(FVector2D DesiredSize)
{
	if(Brush.ImageSize != DesiredSize)
	{
		Brush.ImageSize = DesiredSize;

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushTintColor(FSlateColor TintColor)
{
	if(Brush.TintColor != TintColor)
	{
		Brush.TintColor = TintColor;

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::PaintAndVolatility);
		}
	}
}

void UImage::SetBrushFromAsset(USlateBrushAsset* Asset)
{
	if(!Asset || Brush != Asset->Brush)
	{
		CancelTextureStreaming();
		Brush = Asset ? Asset->Brush : FSlateBrush();

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushFromTexture(UTexture2D* Texture, bool bMatchSize)
{
	CancelTextureStreaming();

	if(Brush.GetResourceObject() != Texture)
	{
		Brush.SetResourceObject(Texture);

		if (Texture) // Since this texture is used as UI, don't allow it affected by budget.
		{
			Texture->bIgnoreStreamingMipBias = true;
		}

		if (bMatchSize)
		{
			if (Texture)
			{
				Brush.ImageSize.X = Texture->GetSizeX();
				Brush.ImageSize.Y = Texture->GetSizeY();
			}
			else
			{
				Brush.ImageSize = FVector2D(0, 0);
			}
		}

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushFromAtlasInterface(TScriptInterface<ISlateTextureAtlasInterface> AtlasRegion, bool bMatchSize)
{
	if(Brush.GetResourceObject() != AtlasRegion.GetObject())
	{
		CancelTextureStreaming();
		Brush.SetResourceObject(AtlasRegion.GetObject());

		if (bMatchSize)
		{
			if (AtlasRegion)
			{
				FSlateAtlasData AtlasData = AtlasRegion->GetSlateAtlasData();
				Brush.ImageSize = AtlasData.GetSourceDimensions();
			}
			else
			{
				Brush.ImageSize = FVector2D(0, 0);
			}
		}

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushFromTextureDynamic(UTexture2DDynamic* Texture, bool bMatchSize)
{
	if(Brush.GetResourceObject() != Texture)
	{
		CancelTextureStreaming();
		Brush.SetResourceObject(Texture);

		if (bMatchSize && Texture)
		{
			Brush.ImageSize.X = Texture->SizeX;
			Brush.ImageSize.Y = Texture->SizeY;
		}

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::SetBrushFromMaterial(UMaterialInterface* Material)
{
	if(Brush.GetResourceObject() != Material)
	{
		CancelTextureStreaming();
		Brush.SetResourceObject(Material);

		//TODO UMG Check if the material can be used with the UI

		if (MyImage.IsValid())
		{
			MyImage->SetImage(&Brush);
			MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UImage::CancelTextureStreaming()
{
	if (StreamingHandle.IsValid())
	{
		StreamingHandle->CancelHandle();
		StreamingHandle.Reset();
	}
}

void UImage::SetBrushFromSoftTexture(TSoftObjectPtr<UTexture2D> SoftTexture, bool bMatchSize)
{
	CancelTextureStreaming();

	TWeakObjectPtr<UImage> WeakThis(this); // using weak ptr in case 'this' has gone out of scope by the time this lambda is called

	StreamingHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftTexture.ToSoftObjectPath(),
			[WeakThis, SoftTexture, bMatchSize]() {
				if (WeakThis.IsValid())
				{
					WeakThis->SetBrushFromTexture(SoftTexture.Get(), bMatchSize);
				}
			}, FStreamableManager::AsyncLoadHighPriority);
}

UMaterialInstanceDynamic* UImage::GetDynamicMaterial()
{
	UMaterialInterface* Material = NULL;

	UObject* Resource = Brush.GetResourceObject();
	Material = Cast<UMaterialInterface>(Resource);

	if ( Material )
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);

		if ( !DynamicMaterial )
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			Brush.SetResourceObject(DynamicMaterial);

			if ( MyImage.IsValid() )
			{
				MyImage->SetImage(&Brush);
				MyImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
			}
		}
		
		return DynamicMaterial;
	}

	//TODO UMG can we do something for textures?  General purpose dynamic material for them?

	return NULL;
}

FReply UImage::HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseButtonDownEvent.IsBound() )
	{
		return OnMouseButtonDownEvent.Execute(Geometry, MouseEvent).NativeReply;
	}

	return FReply::Unhandled();
}

#if WITH_EDITOR

const FText UImage::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
