// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AccessedNone : ModuleRules
{
	public AccessedNone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets headers use folder-relative includes, e.g. "Core/MetaLog.h"
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"EnhancedInput", "CommonUI", "UMG",
			"DeveloperSettings", "AIModule", "NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", "SlateCore",
			"Niagara", "LevelSequence", "MovieScene",
			"AudioMixer", "MetasoundEngine", "SignalProcessing"
		});
	}
}
