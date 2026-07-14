// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_CounterProofWall.h"

UPRGA_CounterProofWall::UPRGA_CounterProofWall()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"));
	ActivationPolicy = EPRAbilityActivationPolicy::WhileInputActive;
}
