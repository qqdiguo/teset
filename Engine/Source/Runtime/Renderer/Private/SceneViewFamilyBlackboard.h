// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeferredShadingRenderer.h: Scene rendering definitions.
=============================================================================*/

#pragma once

#include "RenderGraph.h"


class FViewInfo;


/** Contains reference on all available buffer for a given scene. */
BEGIN_SHADER_PARAMETER_STRUCT(FSceneViewFamilyBlackboard, )
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthBuffer)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneVelocityBuffer)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneGBufferA)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneGBufferB)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneGBufferC)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneGBufferD)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneGBufferE)
END_SHADER_PARAMETER_STRUCT()


/** Sets up the the blackboard from available scene view family.
 *
 * Note: Once the entire renderer is built with a single render graph, would no longer need this function.
 */
void SetupSceneViewFamilyBlackboard(
	FRDGBuilder& GraphBuilder,
	FSceneViewFamilyBlackboard* OutBlackboard);

/** Returns a render graph texture resource reference onto the eye adaptation or fallback.
 */
FRDGTextureRef GetEyeAdaptationTexture(FRDGBuilder& GraphBuilder, const FViewInfo& View);
