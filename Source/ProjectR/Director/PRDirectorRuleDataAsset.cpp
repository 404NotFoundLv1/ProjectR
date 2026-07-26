// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorRuleDataAsset.h"

bool UPRDirectorRuleDataAsset::IsRuleDefinitionValid() const
{
	if (!RuleId.IsValid() || !RuleId.ToString().StartsWith(TEXT("Rule.")) || MaximumLevel != 3) return false;
	TSet<FName> Names;
	for (const FPRDirectorParameterDefinition& Definition : ParameterSchema)
	{
		if (Definition.Name.IsNone() || Names.Contains(Definition.Name) || !FMath::IsFinite(Definition.Minimum) || !FMath::IsFinite(Definition.Maximum) || !FMath::IsFinite(Definition.DefaultValue) || Definition.Minimum > Definition.DefaultValue || Definition.DefaultValue > Definition.Maximum) return false;
		Names.Add(Definition.Name);
	}
	return true;
}
