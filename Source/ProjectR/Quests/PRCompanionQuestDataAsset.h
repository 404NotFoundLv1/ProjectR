// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRCompanionQuestDataAsset.generated.h"
UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionQuestDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsDefinitionValid() const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FName QuestId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FName EntitlementId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FName CompletionLineId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FText ObjectiveText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Quest") FText RewardText;
};
