// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "Roguelike/PRRewardTypes.h"

class UAbilitySystemComponent;

/** GAS-only application boundary for bounded v0.4.2 reward values. */
class PROJECTR_API FPRGASRewardApplication
{
public:
	static EPRRewardApplyResult Apply(UAbilitySystemComponent& AbilitySystem, const FPRRewardEffectSpec& Effect, FActiveGameplayEffectHandle& OutHandle);
	static void Remove(UAbilitySystemComponent& AbilitySystem, const FActiveGameplayEffectHandle& Handle);
};
