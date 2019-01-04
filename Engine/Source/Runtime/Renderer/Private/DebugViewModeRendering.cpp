// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
DebugViewModeRendering.cpp: Contains definitions for rendering debug viewmodes.
=============================================================================*/

#include "DebugViewModeRendering.h"
#include "Materials/Material.h"
#include "ShaderComplexityRendering.h"
#include "PrimitiveDistanceAccuracyRendering.h"
#include "MeshTexCoordSizeAccuracyRendering.h"
#include "MaterialTexCoordScalesRendering.h"
#include "RequiredTextureResolutionRendering.h"
#include "PrimitiveSceneInfo.h"
#include "ScenePrivate.h"
#include "PostProcessing.h"
#include "PostProcess/PostProcessVisualizeComplexity.h"
#include "PostProcess/PostProcessStreamingAccuracyLegend.h"
#include "CompositionLighting/PostProcessPassThrough.h"
#include "PostProcess/PostProcessCompositeEditorPrimitives.h"
#include "SceneRendering.h"
#include "DeferredShadingRenderer.h"
#include "MeshPassProcessor.inl"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FDebugViewModePassPassUniformParameters, "DebugViewModePass");

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


void SetupDebugViewModePassUniformBuffer(FSceneRenderTargets& SceneContext, ERHIFeatureLevel::Type FeatureLevel, FDebugViewModePassPassUniformParameters& PassParameters)
{
	SetupSceneTextureUniformParameters(SceneContext, FeatureLevel, ESceneTextureSetupMode::None, PassParameters.SceneTextures);

	const int32 NumEngineColors = FMath::Min<int32>(GEngine->StreamingAccuracyColors.Num(), NumStreamingAccuracyColors);
	int32 ColorIndex = 0;
	for (; ColorIndex < NumEngineColors; ++ColorIndex)
	{
		PassParameters.AccuracyColors[ColorIndex] = GEngine->StreamingAccuracyColors[ColorIndex];
	}
	for (; ColorIndex < NumStreamingAccuracyColors; ++ColorIndex)
	{
		PassParameters.AccuracyColors[ColorIndex] = FLinearColor::Black;
	}
}


IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeVS,TEXT("/Engine/Private/DebugViewModeVertexShader.usf"),TEXT("Main"),SF_Vertex);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeHS,TEXT("/Engine/Private/DebugViewModeVertexShader.usf"),TEXT("MainHull"),SF_Hull);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeDS,TEXT("/Engine/Private/DebugViewModeVertexShader.usf"),TEXT("MainDomain"),SF_Domain);

#if 0 // todo : implement FMissingShaderPS
/**
* Pixel shader that renders texture streamer wanted mips accuracy.
*/
class FMissingShaderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMissingShaderPS,Global);

public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return AllowDebugViewVSDSHS(Parameters.Platform); }

	FMissingShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):	FGlobalShader(Initializer) {}
	FMissingShaderPS() {}

	virtual bool Serialize(FArchive& Ar) override { return FGlobalShader::Serialize(Ar); }

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("UNDEFINED_VALUE"), UndefinedStreamingAccuracyIntensity);
	}

	virtual void SetParameters(
		FRHICommandList& RHICmdList, 
		int32 NumVSInstructions, 
		int32 NumPSInstructions, 
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View,
		const FDrawingPolicyRenderState& DrawRenderState
		) override {}

	virtual void SetMesh(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FSceneView& View,
		const FPrimitiveSceneProxy* Proxy,
		int32 VisualizeLODIndex,
		const FMeshBatchElement& BatchElement, 
		const FDrawingPolicyRenderState& DrawRenderState
		) override{}

	virtual void SetMesh(FRHICommandList& RHICmdList, const FSceneView& View) override {}

	virtual FShader* GetShader() override { return static_cast<FShader*>(this); }
};

IMPLEMENT_SHADER_TYPE(,FMissingShaderPS,TEXT("/Engine/Private/MissingShaderPixelShader.usf"),TEXT("Main"),SF_Pixel);

#endif // todo : implement FMissingShaderPS

ENGINE_API bool GetDebugViewMaterial(const UMaterialInterface* InMaterialInterface, EDebugViewShaderMode InDebugViewMode, const FMaterialRenderProxy*& OutMaterialRenderProxy, const FMaterial*& OutMaterial);

void FDeferredShadingSceneRenderer::DoDebugViewModePostProcessing(FRHICommandListImmediate& RHICmdList, const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& VelocityRT)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_PostProcessing_Process );

	check(IsInRenderingThread());
	check(View.VerifyMembersChecks());

	GRenderTargetPool.AddPhaseEvent(TEXT("PostProcessing"));

	// so that the passes can register themselves to the graph
	FMemMark Mark(FMemStack::Get());
	FRenderingCompositePassContext CompositeContext(RHICmdList, View);

	FPostprocessContext Context(RHICmdList, CompositeContext.Graph, View);
	ensure(Context.View.PrimaryScreenPercentageMethod != EPrimaryScreenPercentageMethod::TemporalUpscale);

	const bool bHDROutputEnabled = GRHISupportsHDROutput && IsHDREnabled();

	// Shader complexity does not actually output a color
	if (!View.Family->EngineShowFlags.ShaderComplexity)
	{
		GPostProcessing.AddGammaOnlyTonemapper(Context);
	}

	switch (View.Family->GetDebugViewShaderMode())
	{
		case DVSM_QuadComplexity:
		{
			float ComplexityScale = 1.f / (float)(GEngine->QuadComplexityColors.Num() - 1) / NormalizedQuadComplexityValue; // .1f comes from the values used in LightAccumulator_GetResult
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeComplexity(GEngine->QuadComplexityColors, FVisualizeComplexityApplyPS::CS_STAIR, ComplexityScale, true));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
			break;
		}
		case DVSM_ShaderComplexity:
		case DVSM_ShaderComplexityContainedQuadOverhead:
		case DVSM_ShaderComplexityBleedingQuadOverhead:
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeComplexity(GEngine->ShaderComplexityColors, FVisualizeComplexityApplyPS::CS_RAMP, 1.f, true));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
			break;
		}
		case DVSM_PrimitiveDistanceAccuracy:
		case DVSM_MeshUVDensityAccuracy:
		case DVSM_MaterialTextureScaleAccuracy:
		case DVSM_RequiredTextureResolution:
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessStreamingAccuracyLegend(GEngine->StreamingAccuracyColors));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
			break;
		}
		default:
			ensure(false);
			break;
	};

#if WITH_EDITOR
	if (GIsEditor)
	{
		GPostProcessing.AddSelectionOutline(Context);
	}
#endif

	// After the graph is built but before the graph is processed.
	// If a postprocess material is using a GBuffer it adds the refcount int FRCPassPostProcessMaterial::Process()
	// and when it gets processed it removes the refcount
	// We only release the GBuffers after the last view was processed (SplitScreen)
	if(View.Family->Views[View.Family->Views.Num() - 1] == &View)
	{
		// Generally we no longer need the GBuffers, anyone that wants to keep the GBuffers for longer should have called AdjustGBufferRefCount(1) to keep it for longer
		// and call AdjustGBufferRefCount(-1) once it's consumed. This needs to happen each frame. PostProcessMaterial do that automatically
		FSceneRenderTargets::Get(RHICmdList).AdjustGBufferRefCount(RHICmdList, -1);
	}

	// Add a pass-through for the final step if a backbuffer UAV is required but unsupported by this RHI
	if (Context.FinalOutput.IsComputePass() && !View.Family->RenderTarget->GetRenderTargetUAV().IsValid())
	{
		FRenderingCompositePass* PassthroughNode = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessPassThrough(nullptr));
		PassthroughNode->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
		Context.FinalOutput = FRenderingCompositeOutputRef(PassthroughNode);
	}

	// The graph setup should be finished before this line ----------------------------------------
	{
		// currently created on the heap each frame but View.Family->RenderTarget could keep this object and all would be cleaner
		TRefCountPtr<IPooledRenderTarget> Temp;
		FSceneRenderTargetItem Item;
		Item.TargetableTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();
		Item.ShaderResourceTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();
		Item.UAV = View.Family->RenderTarget->GetRenderTargetUAV();

		FPooledRenderTargetDesc Desc;

		// Texture could be bigger than viewport
		if (View.Family->RenderTarget->GetRenderTargetTexture())
		{
			Desc.Extent.X = View.Family->RenderTarget->GetRenderTargetTexture()->GetSizeX();
			Desc.Extent.Y = View.Family->RenderTarget->GetRenderTargetTexture()->GetSizeY();
		}
		else
		{
			Desc.Extent = View.Family->RenderTarget->GetSizeXY();
		}

		const bool bIsFinalOutputComputePass = Context.FinalOutput.IsComputePass();
		Desc.TargetableFlags |= bIsFinalOutputComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
		Desc.Format = bIsFinalOutputComputePass ? PF_R8G8B8A8 : PF_B8G8R8A8;

		// todo: this should come from View.Family->RenderTarget
		Desc.Format = bHDROutputEnabled ? GRHIHDRDisplayOutputFormat : Desc.Format;
		Desc.NumMips = 1;
		Desc.DebugName = TEXT("FinalPostProcessColor");

		GRenderTargetPool.CreateUntrackedElement(Desc, Temp, Item);

		GPostProcessing.OverrideRenderTarget(Context.FinalOutput, Temp, Desc);

		TArray<FRenderingCompositePass*> TargetedRoots;
		TargetedRoots.Add(Context.FinalOutput.GetPass());

		// execute the graph/DAG
		CompositeContext.Process(TargetedRoots, TEXT("PostProcessing"));

		// May need to wait on the final pass to complete
		if (Context.FinalOutput.IsAsyncComputePass())
		{
			FComputeFenceRHIParamRef ComputeFinalizeFence = Context.FinalOutput.GetComputePassEndFence();
			if (ComputeFinalizeFence)
			{
				Context.RHICmdList.WaitComputeFence(ComputeFinalizeFence);
			}
		}
	}

	GRenderTargetPool.AddPhaseEvent(TEXT("AfterPostprocessing"));
}

bool FDeferredShadingSceneRenderer::RenderDebugViewMode(FRHICommandListImmediate& RHICmdList)
{
	bool bDirty=0;
	SCOPED_DRAW_EVENT(RHICmdList, DebugViewMode);

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		SCOPED_CONDITIONAL_DRAW_EVENTF(RHICmdList, EventView, Views.Num() > 1, TEXT("View%d"), ViewIndex);

		FViewInfo& View = Views[ViewIndex];

		Scene->UniformBuffers.UpdateViewUniformBuffer(View);

		// Some of the viewmodes use SCENE_TEXTURES_DISABLED to prevent issues when running in commandlet mode.
		FDebugViewModePassPassUniformParameters PassParameters;
		SetupDebugViewModePassUniformBuffer(SceneContext, View.GetFeatureLevel(), PassParameters);
		Scene->UniformBuffers.DebugViewModePassUniformBuffer.UpdateUniformBufferImmediate(PassParameters);

		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
		{
			SCOPED_DRAW_EVENT(RHICmdList, Dynamic);

			SubmitMeshDrawCommandsForView(View, EMeshPass::DebugViewMode, nullptr, RHICmdList);
		}
	}

	return bDirty;
}

FDebugViewModePS::FDebugViewModePS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer) 
{
	PassUniformBuffer.Bind(Initializer.ParameterMap, FDebugViewModePassPassUniformParameters::StaticStructMetadata.GetShaderVariableName());
}

FDebugViewModeMeshProcessor::FDebugViewModeMeshProcessor(
	const FScene* InScene, 
	ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand, 
	FUniformBufferRHIParamRef InPassUniformBuffer, 
	bool bTranslucentBasePass,
	FMeshPassDrawListContext& InDrawListContext
)
	: FMeshPassProcessor(InScene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
	, PassUniformBuffer(InPassUniformBuffer)
	, DebugViewMode(DVSM_None)
	, ViewModeParam(INDEX_NONE)
	, DebugViewModeInterface(nullptr)
{
	if (InViewIfDynamicMeshCommand)
	{
		DebugViewMode = InViewIfDynamicMeshCommand->Family->GetDebugViewShaderMode();
		ViewModeParam = InViewIfDynamicMeshCommand->Family->GetViewModeParam();
		ViewModeParamName = InViewIfDynamicMeshCommand->Family->GetViewModeParamName();

		ViewUniformBuffer = InViewIfDynamicMeshCommand->ViewUniformBuffer;

		DebugViewModeInterface = FDebugViewModeInterface::GetInterface(DebugViewMode);
	}
	if (InScene)
	{
		if (!ViewUniformBuffer)
		{
			ViewUniformBuffer = InScene->UniformBuffers.ViewUniformBuffer;
		}
		if (!PassUniformBuffer)
		{
			PassUniformBuffer = InScene->UniformBuffers.DebugViewModePassUniformBuffer;
		}
	}
}

void FDebugViewModeMeshProcessor::AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId)
{
	const FMaterialRenderProxy* MaterialRenderProxy = nullptr;
	const FMaterial* Material = nullptr;
	const FMaterial* BatchMaterial = MeshBatch.MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);

	if (!DebugViewModeInterface || !BatchMaterial)
	{
		return;
	}

	const UMaterialInterface* ResolvedMaterial = MeshBatch.MaterialRenderProxy->GetMaterialInterface();
	if (!DebugViewModeInterface->bNeedsMaterialProperties && FDebugViewModeInterface::AllowFallbackToDefaultMaterial(BatchMaterial))
	{
		ResolvedMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!GetDebugViewMaterial(ResolvedMaterial, DebugViewMode, MaterialRenderProxy,  Material))
	{
		return;
	}

	MaterialRenderProxy->UpdateUniformExpressionCacheIfNeeded(FeatureLevel);

	FVertexFactoryType* VertexFactoryType = MeshBatch.VertexFactory->GetType();

	const EMaterialTessellationMode MaterialTessellationMode = Material->GetTessellationMode();
	const bool bNeedsHSDS = RHISupportsTessellation(GShaderPlatformForFeatureLevel[FeatureLevel])
			&& VertexFactoryType->SupportsTessellationShaders()
			&& MaterialTessellationMode != MTM_NoTessellation;

	TMeshProcessorShaders<FDebugViewModeVS,	FDebugViewModeHS, FDebugViewModeDS,	FDebugViewModePS> DebugViewModePassShaders;
	DebugViewModePassShaders.VertexShader = Material->GetShader<FDebugViewModeVS>(VertexFactoryType);
	if (bNeedsHSDS)
	{
		DebugViewModePassShaders.DomainShader = Material->GetShader<FDebugViewModeDS>(VertexFactoryType);
		DebugViewModePassShaders.HullShader = Material->GetShader<FDebugViewModeHS>(VertexFactoryType);
	}
	DebugViewModePassShaders.PixelShader = DebugViewModeInterface->GetPixelShader(Material, VertexFactoryType);

	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, *BatchMaterial);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, *BatchMaterial);

	FDrawingPolicyRenderState DrawRenderState;
	DrawRenderState.SetViewUniformBuffer(ViewUniformBuffer);
	DrawRenderState.SetPassUniformBuffer(PassUniformBuffer);

	FDebugViewModeInterface::FRenderState InterfaceRenderState;
	DebugViewModeInterface->SetDrawRenderState(Material->GetBlendMode(), InterfaceRenderState);
	DrawRenderState.SetBlendState(InterfaceRenderState.BlendState);
	DrawRenderState.SetDepthStencilState(InterfaceRenderState.DepthStencilState);

	FDebugViewModeShaderElementData ShaderElementData(
		*MaterialRenderProxy,
		*Material,
		DebugViewMode, 
		ViewIfDynamicMeshCommand->ViewMatrices.GetViewOrigin(), 
		MeshBatch.VisualizeLODIndex, 
		ViewModeParam, 
		ViewModeParamName);

	if (DebugViewModeInterface->bNeedsInstructionCount)
	{
		ShaderElementData.NumVSInstructions = BatchMaterial->GetShader<TBasePassVS<TUniformLightMapPolicy<LMP_NO_LIGHTMAP>, false>>(VertexFactoryType)->GetNumInstructions();
		ShaderElementData.NumPSInstructions = BatchMaterial->GetShader<TBasePassPS<TUniformLightMapPolicy<LMP_NO_LIGHTMAP>, false>>(VertexFactoryType)->GetNumInstructions();
	}

	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, true);

	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		*MaterialRenderProxy,
		*Material,
		DrawRenderState,
		DebugViewModePassShaders,
		MeshFillMode,
		MeshCullMode,
		1,
		FMeshDrawCommandSortKey::Default,
		EMeshPassFeatures::Default,
		ShaderElementData);
}

FMeshPassProcessor* CreateDebugViewModePassProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext& InDrawListContext)
{
	const ERHIFeatureLevel::Type FeatureLevel = Scene ? Scene->GetFeatureLevel() : (InViewIfDynamicMeshCommand ? InViewIfDynamicMeshCommand->GetFeatureLevel() : GMaxRHIFeatureLevel);
	return new(FMemStack::Get()) FDebugViewModeMeshProcessor(Scene, FeatureLevel, InViewIfDynamicMeshCommand, nullptr, false, InDrawListContext);
}

FRegisterPassProcessorCreateFunction RegisterDebugViewModePass(&CreateDebugViewModePassProcessor, EShadingPath::Deferred, EMeshPass::DebugViewMode, EMeshPassFlags::MainView);


void InitDebugViewModeInterfaces()
{
	FDebugViewModeInterface::SetInterface(DVSM_ShaderComplexity, new FComplexityAccumulateInterface(true, false));
	FDebugViewModeInterface::SetInterface(DVSM_ShaderComplexityContainedQuadOverhead, new FComplexityAccumulateInterface(true, false));
	FDebugViewModeInterface::SetInterface(DVSM_ShaderComplexityBleedingQuadOverhead, new FComplexityAccumulateInterface(true, true));
	FDebugViewModeInterface::SetInterface(DVSM_QuadComplexity, new FComplexityAccumulateInterface(false, false));

	FDebugViewModeInterface::SetInterface(DVSM_PrimitiveDistanceAccuracy, new FPrimitiveDistanceAccuracyInterface());
	FDebugViewModeInterface::SetInterface(DVSM_MeshUVDensityAccuracy, new FMeshTexCoordSizeAccuracyInterface());
	FDebugViewModeInterface::SetInterface(DVSM_MaterialTextureScaleAccuracy, new FMaterialTexCoordScaleAccuracyInterface());
	FDebugViewModeInterface::SetInterface(DVSM_OutputMaterialTextureScales, new FOutputMaterialTexCoordScaleInterface());
	FDebugViewModeInterface::SetInterface(DVSM_RequiredTextureResolution, new FRequiredTextureResolutionInterface());
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


