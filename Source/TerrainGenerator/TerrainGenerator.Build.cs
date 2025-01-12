// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TerrainGenerator : ModuleRules
{
	public TerrainGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "RealtimeMeshComponent", "UnrealFastNoise2" });
    }
}