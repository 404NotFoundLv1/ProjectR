// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PRCombatFeedbackInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRCombatFeedbackInterface : public UInterface
{
	GENERATED_BODY()
};

/** Pawn-side placeholder feedback surface independent of Combat's private types. */
class PROJECTR_API IPRCombatFeedbackInterface
{
	GENERATED_BODY()

public:
	virtual void HandleCombatHitReaction() = 0;
	virtual void HandleCombatLifeStateChanged(bool bIsDead) = 0;
};
