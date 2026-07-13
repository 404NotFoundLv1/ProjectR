// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PRCombatantInterface.generated.h"

class UGameplayEffect;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRCombatantInterface : public UInterface
{
	GENERATED_BODY()
};

/** Stable owner-side identity and damage-effect contract consumed by Combat. */
class PROJECTR_API IPRCombatantInterface
{
	GENERATED_BODY()

public:
	virtual FName GetCombatantId() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetDamageEffectClass() const = 0;
};
