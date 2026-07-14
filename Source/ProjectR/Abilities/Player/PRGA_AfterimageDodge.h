// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_AfterimageDodge.generated.h"

/** Configuration-only native parent for AfterimageDodge; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_AfterimageDodge : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_AfterimageDodge();
};
