// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_ShadowThrust.h"

UPRGA_ShadowThrust::UPRGA_ShadowThrust()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}
