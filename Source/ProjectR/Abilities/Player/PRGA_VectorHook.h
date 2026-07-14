// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_VectorHook.generated.h"

/** Configuration-only native parent for VectorHook; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_VectorHook : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_VectorHook();
};
