// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRRewardPolicyDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRRewardPolicyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsPolicyDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FName PolicyId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 CommonWeight = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 RareWeight = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 EpicWeight = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPrimaryAssetId> RewardIds;
};
