// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Roguelike/PRRewardTypes.h"
#include "Roguelike/PRRoomTypes.h"

#include "PRRewardDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRRewardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsRewardDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RewardId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag RarityTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag RewardTypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FName FamilyId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTagContainer MutualExclusionTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRoomCondition> WeightConditions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FName ApplicationId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPRRewardEffectSpec EffectSpec;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText EffectText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText CostText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 Tier = 0;
};
