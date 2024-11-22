#pragma once
#include "ScreenPass.h"
struct FRDGDrawTextureInfo;
//[Sketch-Pipeline][Add-Begin]素描阴影处理
void AddSketchShadowPass(
	FRDGBuilder& GraphBuilder,
	const FGlobalShaderMap* ShaderMap,
	FRDGTextureRef SceneColor,
	FRDGTextureRef SceneDepth,
	FRDGTextureRef BaseColor,
	FRDGTextureRef SketchData,
	FRDGTextureRef ShadowMask,
	const FRDGDrawTextureInfo& DrawInfo);
//[Sketch-Pipeline][Add-End]

//[Sketch-Pipeline][Add-Begin]后处理描边
struct FSketchOutlineInputs
{
	FScreenPassRenderTarget OverrideOutput;
	FScreenPassTexture SceneColor;
	FScreenPassTexture WorldNormal;
	FScreenPassTexture SketchData;
	FVector4f OutlineColor;
	FVector2f Resolution;
};

FScreenPassTexture AddSketchOutlinePass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	const FSketchOutlineInputs Inputs);
//[Sketch-Pipeline][Add-End]