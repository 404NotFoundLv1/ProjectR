// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyPrototypeDataAsset.h"

#include "PRAuditorBossDataAsset.generated.h"

class UGameplayEffect;

/** Data-only Auditor tuning. v0.2.4 may change values without changing Boss code. */
UCLASS(BlueprintType)
class PROJECTR_API UPRAuditorBossDataAsset : public UPREnemyPrototypeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Boss") float RuleAuditHealthRatio = 0.66f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Boss") float PredictionShieldHealthRatio = 0.33f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Boss") float PredictionShieldValue = 240.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Boss") int32 CounterproofFragmentsAwarded = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Boss") TSubclassOf<UGameplayEffect> PredictionShieldEffect;
};
