// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "PRCombatEventSubjectInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRCombatEventSubjectInterface : public UInterface
{
	GENERATED_BODY()
};

/** Stable subject identity for non-damage combat outcomes without an ASC combatant. */
class PROJECTR_API IPRCombatEventSubjectInterface
{
	GENERATED_BODY()

public:
	virtual FName GetCombatEventSubjectId() const = 0;
};
