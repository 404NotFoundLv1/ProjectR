// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "PRCombatAttackProxyInterface.generated.h"

class AActor;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRCombatAttackProxyInterface : public UInterface
{
	GENERATED_BODY()
};

/** One-shot native attack-proxy contract reserved for the later afterimage checkpoint. */
class PROJECTR_API IPRCombatAttackProxyInterface
{
	GENERATED_BODY()

public:
	virtual FName GetAttackProxyId() const = 0;
	virtual AActor* GetAttackProxyOwner() const = 0;
	virtual bool ConsumeAttackProxy(AActor* Attacker) = 0;
};
