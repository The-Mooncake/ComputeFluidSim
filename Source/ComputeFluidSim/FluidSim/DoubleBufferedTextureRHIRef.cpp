
#include "DoubleBufferedTextureRHIRef.h"
#include "RenderGraphUtils.h"


UDoubleBufferedTextureRHIRef::~UDoubleBufferedTextureRHIRef()
{
	ReleaseTextureResource(RT_Even);
	ReleaseTextureResource(RT_Odd);
}

void UDoubleBufferedTextureRHIRef::Setup(const FString& TexName, const EPixelFormat& TexType, const FIntVector& TexSize, const FLinearColor& ClearColour)
{
	TextureName = TexName;
	TextureType = TexType;
	TexClearColour = ClearColour;
	TextureSize = TexSize;

	CreateRHITextureResource(RT_Even, TextureName + "_Even");
	CreateRHITextureResource(RT_Odd, TextureName + "_Odd");
}

void UDoubleBufferedTextureRHIRef::RegisterGPUTexture(FRDGBuilder& GraphBuilder) 
{
	FString BaseName = FString("FluidSimCompute_") += TextureName;
	RT_Shader_Even = RegisterExternalTexture(GraphBuilder, RT_Even, *(BaseName += "_Even") ) ;
	RT_Shader_Odd = RegisterExternalTexture(GraphBuilder, RT_Odd, *(BaseName += "_Odd")  );
}

void UDoubleBufferedTextureRHIRef::Flip()
{
	Even = !Even;
}

FRDGTextureRef UDoubleBufferedTextureRHIRef::GetCurrent()
{
	return Even ? RT_Shader_Even : RT_Shader_Odd;
}

FRDGTextureRef UDoubleBufferedTextureRHIRef::GetTarget()
{
	return Even ? RT_Shader_Odd : RT_Shader_Even;
}

void UDoubleBufferedTextureRHIRef::ClearRenderTargets(FRHICommandListImmediate& RHICmdList)
{
	ClearRenderTarget(RHICmdList, RT_Even);
	ClearRenderTarget(RHICmdList, RT_Odd);
}

void UDoubleBufferedTextureRHIRef::CreateRHITextureResource(FTextureRHIRef& TexReference, FString TexName)
{
	const FRHITextureCreateDesc CDesc = FRHITextureCreateDesc::Create3D(
		*TexName,
		TextureSize.X,
		TextureSize.Y,
		TextureSize.Z,
		TextureType)
		.SetFlags(ETextureCreateFlags::External | ETextureCreateFlags::UAV | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource)
		.SetInitialState(ERHIAccess::UAVCompute)
		.SetExtent(TextureSize.X, TextureSize.Y)
		.SetDepth(TextureSize.Z)
		.SetClearValue(FClearValueBinding(TexClearColour) );

	TexReference = RHICreateTexture(CDesc);
}

void UDoubleBufferedTextureRHIRef::ReleaseTextureResource(FTextureRHIRef& Tex)
{
	if (Tex && Tex->GetRefCount() > 0 )
	{
		Tex = nullptr;
	}
}
