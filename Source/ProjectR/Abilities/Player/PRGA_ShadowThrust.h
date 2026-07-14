// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_ShadowThrust.generated.h"

/** Configuration-only native parent for ShadowThrust; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_ShadowThrust : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_ShadowThrust();
};
