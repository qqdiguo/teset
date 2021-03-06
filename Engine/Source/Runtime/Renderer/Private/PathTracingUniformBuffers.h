// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PathTracingUniformBuffers.h
=============================================================================*/

#pragma once

#include "UniformBuffer.h"

// Lights
const int32 GLightCountMaximum = 64;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPathTracingLightData, )
	SHADER_PARAMETER(uint32, Count)
	SHADER_PARAMETER_ARRAY(uint32, Type, [GLightCountMaximum])
	// Geometry
	SHADER_PARAMETER_ARRAY(FVector, Position, [GLightCountMaximum])
	SHADER_PARAMETER_ARRAY(FVector, Normal, [GLightCountMaximum])
	SHADER_PARAMETER_ARRAY(FVector, dPdu, [GLightCountMaximum])
	SHADER_PARAMETER_ARRAY(FVector, dPdv, [GLightCountMaximum])
	// Color
	SHADER_PARAMETER_ARRAY(FVector, Color, [GLightCountMaximum])
	// Light-specific
	SHADER_PARAMETER_ARRAY(FVector, Dimensions, [GLightCountMaximum])
END_GLOBAL_SHADER_PARAMETER_STRUCT()


BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPathTracingSkyLightData, )
	SHADER_PARAMETER(uint32, SkyLightRowCount)
	SHADER_PARAMETER(uint32, SkyLightColumnCount)
	SHADER_PARAMETER_SRV(Buffer<float>, SkyLightRowCdf)
	SHADER_PARAMETER_SRV(Buffer<float>, SkyLightColumnCdf)
	SHADER_PARAMETER_SRV(Buffer<float>, SkyLightCubeFaceCdf)
	SHADER_PARAMETER_TEXTURE(TextureCube, SkyLightTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, SkyLightTextureSampler)
END_GLOBAL_SHADER_PARAMETER_STRUCT()


BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPathTracingAdaptiveSamplingData, )
	SHADER_PARAMETER(uint32, UseAdaptiveSampling)
	SHADER_PARAMETER(uint32, RandomSequence)
	SHADER_PARAMETER(uint32, MinimumSamplesPerPixel)
	SHADER_PARAMETER(uint32, Iteration)
	SHADER_PARAMETER(FIntVector, VarianceDimensions)
	SHADER_PARAMETER_SRV(Buffer<float>, VarianceMipTree)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
