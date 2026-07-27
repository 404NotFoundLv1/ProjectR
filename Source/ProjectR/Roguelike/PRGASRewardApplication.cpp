// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRewardApplication.h"

#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

namespace PRGASRewardApplication
{
FGameplayAttribute ResolveAttribute(const EPRRewardAttribute Attribute)
{
	switch (Attribute)
	{
	case EPRRewardAttribute::MaxHealth: return UPRAttributeSet::GetMaxHealthAttribute();
	case EPRRewardAttribute::MaxShield: return UPRAttributeSet::GetMaxShieldAttribute();
	case EPRRewardAttribute::MaxEnergy: return UPRAttributeSet::GetMaxEnergyAttribute();
	case EPRRewardAttribute::AttackPower: return UPRAttributeSet::GetAttackPowerAttribute();
	case EPRRewardAttribute::MoveSpeed: return UPRAttributeSet::GetMoveSpeedAttribute();
	case EPRRewardAttribute::CritChance: return UPRAttributeSet::GetCritChanceAttribute();
	case EPRRewardAttribute::Health: return UPRAttributeSet::GetHealthAttribute();
	case EPRRewardAttribute::Shield: return UPRAttributeSet::GetShieldAttribute();
	case EPRRewardAttribute::Energy: return UPRAttributeSet::GetEnergyAttribute();
	case EPRRewardAttribute::Resonance: return UPRAttributeSet::GetResonanceAttribute();
	default: return FGameplayAttribute();
	}
}
}

EPRRewardApplyResult FPRGASRewardApplication::Apply(UAbilitySystemComponent& AbilitySystem, const FPRRewardEffectSpec& Effect, FActiveGameplayEffectHandle& OutHandle)
{
	OutHandle.Invalidate();
	const FGameplayAttribute Attribute = PRGASRewardApplication::ResolveAttribute(Effect.Attribute);
	if (!Attribute.IsValid() || FMath::IsNearlyZero(Effect.Magnitude)) return EPRRewardApplyResult::Invalid;
	UGameplayEffect* GameplayEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	if (!GameplayEffect) return EPRRewardApplyResult::ApplyFailed;
	GameplayEffect->DurationPolicy = Effect.Duration == EPRRewardEffectDuration::Session ? EGameplayEffectDurationType::Infinite : EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = GameplayEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Effect.Magnitude));
	OutHandle = AbilitySystem.ApplyGameplayEffectToSelf(GameplayEffect, 1.0f, AbilitySystem.MakeEffectContext());
	return OutHandle.IsValid() || Effect.Duration == EPRRewardEffectDuration::Instant ? EPRRewardApplyResult::Applied : EPRRewardApplyResult::ApplyFailed;
}

void FPRGASRewardApplication::Remove(UAbilitySystemComponent& AbilitySystem, const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid()) AbilitySystem.RemoveActiveGameplayEffect(Handle);
}
