// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRMemoryPersonaDataAsset.h"

FPrimaryAssetId UPRMemoryPersonaDataAsset::GetPrimaryAssetId() const { return FPrimaryAssetId(TEXT("ProjectRMemoryPersona"), GetFName()); }

bool UPRMemoryPersonaDataAsset::IsDefinitionValid() const
{
	const bool bKnown = ProviderCompanionId == TEXT("Axiom") || ProviderCompanionId == TEXT("Kindle") || ProviderCompanionId == TEXT("Null");
	if (!bKnown || !CompanionId.IsValid() || EmotionIds.Num() != 3 || SummaryTemplates.Num() != 3 || PlayerOptions.Num() != 3) return false;
	TSet<FName> Emotions;
	TSet<FName> Options;
	for (const FName Id : EmotionIds) if (Id.IsNone() || Emotions.Contains(Id)) return false; else Emotions.Add(Id);
	for (const FPRMemoryPlayerOptionDefinition& Option : PlayerOptions) if (Option.OptionId.IsNone() || Option.DisplayText.IsEmpty() || Options.Contains(Option.OptionId)) return false; else Options.Add(Option.OptionId);
	return true;
}
