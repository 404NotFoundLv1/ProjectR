// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectR : ModuleRules
{
	public ProjectR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectR",
			"ProjectR/Variant_Platforming",
			"ProjectR/Variant_Platforming/Animation",
			"ProjectR/Variant_Combat",
			"ProjectR/Variant_Combat/AI",
			"ProjectR/Variant_Combat/Animation",
			"ProjectR/Variant_Combat/Gameplay",
			"ProjectR/Variant_Combat/Interfaces",
			"ProjectR/Variant_Combat/UI",
			"ProjectR/Variant_SideScrolling",
			"ProjectR/Variant_SideScrolling/AI",
			"ProjectR/Variant_SideScrolling/Gameplay",
			"ProjectR/Variant_SideScrolling/Interfaces",
			"ProjectR/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
