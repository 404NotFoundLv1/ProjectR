// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_ThunderDrop.h"

UPRGA_ThunderDrop::UPRGA_ThunderDrop()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}
