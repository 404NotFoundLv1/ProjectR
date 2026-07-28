// Copyright Epic Games, Inc. All Rights Reserved.
#include "Quests/PRCompanionQuestDataAsset.h"
FPrimaryAssetId UPRCompanionQuestDataAsset::GetPrimaryAssetId() const { return FPrimaryAssetId(TEXT("ProjectRCompanionQuest"), QuestId); }
bool UPRCompanionQuestDataAsset::IsDefinitionValid() const { return !QuestId.IsNone() && CompanionId.IsValid() && !EntitlementId.IsNone() && !CompletionLineId.IsNone() && !DisplayName.IsEmpty() && !ObjectiveText.IsEmpty(); }
