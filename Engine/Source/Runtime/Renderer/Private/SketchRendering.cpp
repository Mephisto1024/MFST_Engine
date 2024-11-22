//[Sketch-Pipeline][Add-Begin]素描阴影处理
#include "SketchRendering.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
#include "SceneRendering.h"
#include "ShaderParameterStruct.h"

class FSketchShadowPassPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSketchShadowPassPS);
	SHADER_USE_PARAMETER_STRUCT(FSketchShadowPassPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputSceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputSceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputBaseColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputShadowMaskTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputSketchDataTexture)

		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture0)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture1)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture2)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture3)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture4)
		SHADER_PARAMETER_TEXTURE(Texture2D, InputHatchTexture5)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FSketchShadowPassPS, "/Engine/Private/SketchShadowShader.usf", "MainPS", SF_Pixel);

void AddSketchShadowPass(
	FRDGBuilder& GraphBuilder,
	const FGlobalShaderMap* ShaderMap,
	FRDGTextureRef SceneColor,
	FRDGTextureRef SceneDepth,
	FRDGTextureRef BaseColor,
	FRDGTextureRef SketchData,
	FRDGTextureRef ShadowMask,
	const FRDGDrawTextureInfo& DrawInfo)
{
	TShaderMapRef<FSketchShadowPassPS> PixelShader(ShaderMap);

	auto* PassParameters = GraphBuilder.AllocParameters<FSketchShadowPassPS::FParameters>();
	PassParameters->InputSceneColorTexture = SceneColor;
	PassParameters->InputSceneDepthTexture = SceneDepth;
	PassParameters->InputBaseColorTexture = BaseColor;
	PassParameters->InputShadowMaskTexture = ShadowMask;
	PassParameters->InputSketchDataTexture = SketchData;

	PassParameters->InputHatchTexture0 = GEngine->HatchLevelTextures[0]->GetResource()->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture1 = GEngine->HatchLevelTextures[1]->GetResource()->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture2 = GEngine->HatchLevelTextures[2]->GetResource()->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture3 = GEngine->HatchLevelTextures[3]->GetResource()->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture4 = GEngine->HatchLevelTextures[4]->GetResource()->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture5 = GEngine->HatchLevelTextures[5]->GetResource()->TextureRHI->GetTexture2D();

	PassParameters->RenderTargets[0] = FRenderTargetBinding(SketchData, ERenderTargetLoadAction::ELoad);

	const FIntPoint DrawSize = DrawInfo.Size == FIntPoint::ZeroValue ? SceneColor->Desc.Extent : DrawInfo.Size;
	const FIntRect ViewRect(DrawInfo.DestPosition, DrawInfo.DestPosition + DrawSize); 

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		ShaderMap,
		RDG_EVENT_NAME("SketchShadowPassPS"),
		PixelShader,
		PassParameters,
		ViewRect
	);
}

//[Sketch-Pipeline][Add-End]

//[Sketch-Pipeline][Add-Begin]后处理描边

class FSketchOutlinePassPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FSketchOutlinePassPS);
	SHADER_USE_PARAMETER_STRUCT(FSketchOutlinePassPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputSceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputWorldNormalTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputSketchDataTexture)
		SHADER_PARAMETER(FVector4f, SketchOutlineColor)
		SHADER_PARAMETER(FVector2f, Resolution)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_SHADER_TYPE(, FSketchOutlinePassPS, TEXT("/Engine/Private/SketchOutlineShader.usf"), TEXT("MainPS"), SF_Pixel);

FScreenPassTexture AddSketchOutlinePass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	const FSketchOutlineInputs Inputs)
{
	check(Inputs.SceneColor.IsValid());

	RDG_EVENT_SCOPE(GraphBuilder, "SketchOutlinePass");

	FScreenPassRenderTarget Output = Inputs.OverrideOutput;
	if (!Output.IsValid())
	{
		FRDGTextureDesc OutputTextureDesc = Inputs.SceneColor.Texture->Desc;
		OutputTextureDesc.Flags |= TexCreate_DisableDCC; // DCC will cause errors on this resource when running splitscreen so disable
		OutputTextureDesc.Reset();
		OutputTextureDesc.ClearValue = FClearValueBinding(FLinearColor::Transparent);

		Output = FScreenPassRenderTarget(
			GraphBuilder.CreateTexture(OutputTextureDesc, TEXT("Sketch")),
			Inputs.SceneColor.ViewRect,
			ERenderTargetLoadAction::EClear);

		Output.Texture = GraphBuilder.CreateTexture(OutputTextureDesc, TEXT("SketchTexture"));
		Output.LoadAction = ERenderTargetLoadAction::EClear;
	}

	const FScreenPassTextureViewport Viewport(Inputs.SketchData);
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(); // *3

	FSketchOutlinePassPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FSketchOutlinePassPS::FParameters>();
	PassParameters->Resolution = FVector2f(
		FScreenPassTextureViewport(Inputs.SceneColor.Texture).Rect.Width(), 
		FScreenPassTextureViewport(Inputs.SceneColor.Texture).Rect.Height());
	PassParameters->SketchOutlineColor = Inputs.OutlineColor;
	PassParameters->InputSceneColorTexture = Inputs.SceneColor.Texture;
	PassParameters->InputWorldNormalTexture = Inputs.WorldNormal.Texture;
	PassParameters->InputSketchDataTexture = Inputs.SketchData.Texture;
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();


	TShaderMapRef<FSketchOutlinePassPS> PixelShader(View.ShaderMap);

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		View.ShaderMap,
		RDG_EVENT_NAME("SketchOutlinePassPS"),
		PixelShader,
		PassParameters,
		Viewport.Rect);

	return MoveTemp(Output);
}

//[Sketch-Pipeline][Add-End]