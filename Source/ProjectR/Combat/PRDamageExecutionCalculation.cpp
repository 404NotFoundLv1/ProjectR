// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/PRDamageExecutionCalculation.h"

#include "Abilities/PRAttributeSet.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "ProjectR.h"

void UPRDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const float Damage = Spec.GetSetByCallerMagnitude(
		UPRTagLibrary::GetCombatDataDamageTag(), false, 0.0f);
	if (!FMath::IsFinite(Damage) || Damage <= 0.0f || Damage > 200000.0f)
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Damage execution rejected invalid Combat.Data.Damage magnitude %.9g."),
			Damage);
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UPRAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		Damage));
}
