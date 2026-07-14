// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_FireSlash.h"

UPRGA_FireSlash::UPRGA_FireSlash()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}
