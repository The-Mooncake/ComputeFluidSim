// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComputeFluidSim.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FComputeFluidSimModule"

void FComputeFluidSimModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ComputeFluidSim"))->GetBaseDir(), TEXT("Shaders/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/DynamicsShaders"), PluginShaderDir);
}

void FComputeFluidSimModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FComputeFluidSimModule, ComputeFluidSim)