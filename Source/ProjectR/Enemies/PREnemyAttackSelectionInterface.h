// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "PREnemyAttackSelectionInterface.generated.h"

class AActor;

/** Narrow, optional attack-selection seam. Enemies without it retain first-configured-attack behaviour. */
UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPREnemyAttackSelectionInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPREnemyAttackSelectionInterface
{
	GENERATED_BODY()

public:
	virtual bool SelectEnemyAttack(AActor* Target, FGameplayTag& OutAttackTag) const = 0;

	/** Optional range override for a selected tactical response. Returning false preserves prototype movement. */
	virtual bool GetPreferredRangeOverride(AActor* Target, float& OutMinimumRange, float& OutMaximumRange) const
	{
		return false;
	}

	/** Called only after GAS accepted the selected attack activation. */
	virtual void NotifyEnemyAttackActivated(FGameplayTag AttackTag)
	{
	}
};
