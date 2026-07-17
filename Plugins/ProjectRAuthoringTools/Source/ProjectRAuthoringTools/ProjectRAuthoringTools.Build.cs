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
			"AudioEditor",
			"AssetTools",
			"CoreUObject",
			"AssetRegistry",
			"Engine",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"KismetCompiler",
			"Niagara",
			"NiagaraEditor",
			"InputCore",
			"Json",
			"ProjectR",
			"StateTreeModule",
			"StateTreeDeveloper",
			"StateTreeEditorModule",
			"GameplayStateTreeModule",
			"PropertyBindingUtils",
			"PropertyBindingUtilsEditor",
			"PropertyAccessEditor",
			"StructUtilsEditor",
			"UnrealEd"
		});
	}
}
