// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyPrototypeDataAsset.h"

#include "PRAllocatorBossDataAsset.generated.h"

/** Allocator-specific data stays within the existing Enemy prototype contract. */
UCLASS(BlueprintType)
class PROJECTR_API UPRAllocatorBossDataAsset final : public UPREnemyPrototypeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Allocator") float ResourceLockThreshold = 0.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Allocator") float RewardDeprivationThreshold = 0.50f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Allocator") float PriceAuditThreshold = 0.25f;
};
