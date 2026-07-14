// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_FireSlash.generated.h"

/** Configuration-only native parent for FireSlash; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_FireSlash : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_FireSlash();
};
