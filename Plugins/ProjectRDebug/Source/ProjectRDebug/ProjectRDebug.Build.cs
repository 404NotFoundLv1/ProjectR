// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectRDebug : ModuleRules
{
	public ProjectRDebug(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"ProjectR"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Engine",
			"GameplayAbilities",
			"InputCore",
			"Slate",
			"SlateCore",
			"Projects"
		});
	}
}
