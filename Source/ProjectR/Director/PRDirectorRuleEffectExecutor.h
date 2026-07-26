// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Director/PRDirectorTypes.h"

class UAbilitySystemComponent;
class UWorld;

/** Owns only transient, validated Director runtime presentation state. Gameplay binding is added by the subsystem. */
class PROJECTR_API FPRDirectorRuleEffectExecutor
{
public:
	void BindWorld(UWorld* InWorld);
	bool Apply(const FPRAppliedDirectorRuleHandle& Handle, const FText& VisibleReason, const FText& CounterDescription, double WorldTimeSeconds);
	bool GetState(FGameplayTag RuleId, FPRDirectorRuleRuntimeState& OutState) const;
	bool Remove(const FPRAppliedDirectorRuleHandle& Handle, double WorldTimeSeconds);
	void Reset();
	void GetStates(TArray<FPRDirectorRuleRuntimeState>& OutStates) const;
	void RebindWorldEffects();
	bool AdvanceCounter(const FGameplayTag RuleId, int32 Amount, double WorldTimeSeconds);
	bool SetRuleStatus(const FGameplayTag RuleId, EPRDirectorRuleRuntimeStatus Status, double WorldTimeSeconds);

private:
	struct FAppliedEffect
	{
		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
		FActiveGameplayEffectHandle Handle;
	};

	float GetRuleValue(const FGameplayTag RuleId, int32 Level, uint8 EffectTarget) const;
	void RefreshAggregateEffects();
	void RemoveAggregateEffects();
	void ApplyAggregateEffect(uint8 EffectTarget, float Magnitude);
	void ApplyCompanionPolicy();
	void UpdateState(FPRDirectorRuleRuntimeState& State, EPRDirectorRuleRuntimeStatus Status, double WorldTimeSeconds);
	TMap<FGameplayTag, FPRDirectorRuleRuntimeState> States;
	TWeakObjectPtr<UWorld> World;
	TMap<uint8, TArray<FAppliedEffect>> AppliedEffects;
	int64 RuntimeSequence = 0;
	bool bRefreshingEffects = false;
	bool bRefreshRequested = false;
};
