// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectRAuthoringTools : ModuleRules
{
	public ProjectRAuthoringTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"ToolsetRegistry"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"InputCore",
			"Json",
			"ProjectR",
			"UnrealEd"
		});
	}
}
