// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_CounterProofWall.generated.h"

/** Configuration-only native parent for CounterProofWall; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_CounterProofWall : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_CounterProofWall();
};
