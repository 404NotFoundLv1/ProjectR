// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_AfterimageDodge.h"

UPRGA_AfterimageDodge::UPRGA_AfterimageDodge()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}
