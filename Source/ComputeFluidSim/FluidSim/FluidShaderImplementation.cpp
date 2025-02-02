
#include "FluidShaderImplementation.h"
#include "ShaderCompilerCore.h"


const int FluidSimThreads = 8;

void FObjectGPUAdvectionShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}


void FObjectGPUInjectionShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}


void FObjectGPUDissipationShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}

void FObjectGPUDiffusionShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}

void FObjectGPUDivergenceShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}

void FObjectGPUProjectPressureShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}

void FObjectGPUProjectGradientShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("THREADS_X"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Y"), FluidSimThreads);
	OutEnvironment.SetDefine(TEXT("THREADS_Z"), FluidSimThreads);
	OutEnvironment.CompilerFlags.Add(ECompilerFlags::CFLAG_AllowTypedUAVLoads); // DX12 feature for the float4 type
}