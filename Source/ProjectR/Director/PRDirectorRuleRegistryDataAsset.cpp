// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorRuleRegistryDataAsset.h"

bool UPRDirectorRuleRegistryDataAsset::IsRegistryReady() const
{
	if (Rules.Num() != 4) return false;
	FString Previous;
	TSet<FGameplayTag> Seen;
	for (const TSoftObjectPtr<UPRDirectorRuleDataAsset>& Reference : Rules)
	{
		const UPRDirectorRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (!Rule || !Rule->IsRuleDefinitionValid() || Seen.Contains(Rule->RuleId)) return false;
		const FString Current = Rule->RuleId.ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current; Seen.Add(Rule->RuleId);
	}
	return true;
}

const UPRDirectorRuleDataAsset* UPRDirectorRuleRegistryDataAsset::FindRule(const FGameplayTag RuleId) const
{
	for (const TSoftObjectPtr<UPRDirectorRuleDataAsset>& Reference : Rules)
	{
		const UPRDirectorRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (Rule && Rule->RuleId == RuleId) return Rule;
	}
	return nullptr;
}
