// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyPrototypeDataAsset.h"

#include "PRPacifierBossDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRPacifierBossDataAsset final : public UPREnemyPrototypeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float IllusionSplitThreshold = 0.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float LowRiskLureThreshold = 0.50f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float YieldSuppressionThreshold = 0.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float IllusionWarningSeconds = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float IllusionWindowSeconds = 4.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float LowRiskLureSeconds = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") float YieldSuppressionSeconds = 4.0f;
};
