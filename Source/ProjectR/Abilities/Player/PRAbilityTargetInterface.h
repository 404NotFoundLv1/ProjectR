// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "UObject/Interface.h"

#include "PRAbilityTargetInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRAbilityTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/** Native target identity and mobility contract used by deterministic player-skill queries. */
class PROJECTR_API IPRAbilityTargetInterface
{
	GENERATED_BODY()

public:
	virtual FName GetAbilityTargetId() const = 0;
	virtual FVector GetAbilityTargetPoint() const = 0;
	virtual EPRAbilityTargetMobility GetAbilityTargetMobility() const = 0;
	virtual bool CanBeTargetedByAbility(FGameplayTag AbilityTag) const = 0;
};
