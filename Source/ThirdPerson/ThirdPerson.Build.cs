// Copyright Epic Games, Inc. All Rights Reserved.

// Configuration script for an Unreal Engine project. This script is used to define the dependencies of the project.

using UnrealBuildTool;

public class ThirdPerson : ModuleRules
{
	public ThirdPerson(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Json", "HTTP", "JsonUtilities" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Networking" });
	}
}
