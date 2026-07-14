// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_VectorHook.h"

UPRGA_VectorHook::UPRGA_VectorHook()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}
