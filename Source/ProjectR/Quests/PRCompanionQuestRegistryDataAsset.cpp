// Copyright Epic Games, Inc. All Rights Reserved.
#include "Quests/PRCompanionQuestRegistryDataAsset.h"
#include "Quests/PRCompanionQuestDataAsset.h"
FPrimaryAssetId UPRCompanionQuestRegistryDataAsset::GetPrimaryAssetId() const { return FPrimaryAssetId(TEXT("ProjectRCompanionQuestRegistry"), GetFName()); }
bool UPRCompanionQuestRegistryDataAsset::IsRegistryReady() const
{
	static const TArray<FName> Expected = { TEXT("Quest.Axiom.ImperfectOptimum"), TEXT("Quest.Axiom.LowProbabilitySample"), TEXT("Quest.Kindle.LearnToRetreat"), TEXT("Quest.Kindle.NoRetreatLine"), TEXT("Quest.Null.GarbageCollection"), TEXT("Quest.Null.RememberMe") };
	if (Quests.Num() != Expected.Num()) return false;
	for (int32 Index = 0; Index < Expected.Num(); ++Index)
	{
		const UPRCompanionQuestDataAsset* Quest = Quests[Index].LoadSynchronous();
		if (!Quest || !Quest->IsDefinitionValid() || Quest->QuestId != Expected[Index]) return false;
	}
	return true;
}
const UPRCompanionQuestDataAsset* UPRCompanionQuestRegistryDataAsset::FindQuest(const FName QuestId) const { for (const TSoftObjectPtr<UPRCompanionQuestDataAsset>& Ref : Quests) { const UPRCompanionQuestDataAsset* Quest = Ref.LoadSynchronous(); if (Quest && Quest->QuestId == QuestId) return Quest; } return nullptr; }
