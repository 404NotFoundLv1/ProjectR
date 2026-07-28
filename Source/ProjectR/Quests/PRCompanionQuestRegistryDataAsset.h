// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Engine/DataAsset.h"
#include "PRCompanionQuestRegistryDataAsset.generated.h"
class UPRCompanionQuestDataAsset;
UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionQuestRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsRegistryReady() const;
	const UPRCompanionQuestDataAsset* FindQuest(FName QuestId) const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") TArray<TSoftObjectPtr<UPRCompanionQuestDataAsset>> Quests;
};
