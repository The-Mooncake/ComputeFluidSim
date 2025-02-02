
#pragma once

#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "RenderGraphDefinitions.h"

// Double buffer render target implementation for convenience.
// This isn't used in the end as its possible to RW a render target in DX12+
class UDoubleBufferedTextureRHIRef
{
public:
	~UDoubleBufferedTextureRHIRef();

	void Setup(const FString& TexName, const EPixelFormat& TexType, const FIntVector& TexSize, const FLinearColor& ClearColour =
		           FLinearColor::Black);
	void RegisterGPUTexture(FRDGBuilder& GraphBuilder);
	FRDGTextureRef GetCurrent();
	FRDGTextureRef GetTarget();
	void Flip();
	void ClearRenderTargets(FRHICommandListImmediate& RHICmdList);

private:
	void CreateRHITextureResource(FTextureRHIRef& TexReference, FString TexName);
	void ReleaseTextureResource(FTextureRHIRef& Tex);

private: 
	bool Even = false;

	FString TextureName;
	EPixelFormat TextureType = PF_A32B32G32R32F;
	FLinearColor TexClearColour = FLinearColor::Black;
	FIntVector TextureSize = FIntVector(64, 64, 32);

	FTextureRHIRef RT_Even = nullptr;
	FTextureRHIRef RT_Odd = nullptr;

	FRDGTextureRef RT_Shader_Even = nullptr;
	FRDGTextureRef RT_Shader_Odd = nullptr;
};
