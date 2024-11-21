//[Sketch-Pipeline][Add-Begin]素描阴影处理
#include "SketchRendering.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
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

	PassParameters->InputHatchTexture0 = GEngine->HatchLevelTextures[0]->Resource->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture1 = GEngine->HatchLevelTextures[1]->Resource->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture2 = GEngine->HatchLevelTextures[2]->Resource->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture3 = GEngine->HatchLevelTextures[3]->Resource->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture4 = GEngine->HatchLevelTextures[4]->Resource->TextureRHI->GetTexture2D();
	PassParameters->InputHatchTexture5 = GEngine->HatchLevelTextures[5]->Resource->TextureRHI->GetTexture2D();

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