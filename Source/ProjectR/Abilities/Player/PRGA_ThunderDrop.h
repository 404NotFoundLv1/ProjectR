// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_ThunderDrop.generated.h"

/** Configuration-only native parent for ThunderDrop; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_ThunderDrop : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_ThunderDrop();
};
