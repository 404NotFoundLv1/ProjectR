// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyPrototypeDataAsset.h"

#include "PRWardenBossDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRWardenBossDataAsset final : public UPREnemyPrototypeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float PredictiveAttackThreshold = 0.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float PlatformLockdownThreshold = 0.50f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float RiskMarkThreshold = 0.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float PredictiveWarningSeconds = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float LockdownWarningSeconds = 1.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float LockdownSeconds = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") float RiskMarkSeconds = 4.0f;
};
