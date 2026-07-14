// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillFragment.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult UPRPlayerSkillFragment::IsDataValid(FDataValidationContext& Context) const
{
	auto IsFiniteNonNegative = [](const float Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.0f;
	};

	bool bValid = true;
	if (const UPRFireSlashSkillFragment* Fire = Cast<UPRFireSlashSkillFragment>(this))
	{
		bValid = IsFiniteNonNegative(Fire->BurningBaseDamage)
			&& IsFiniteNonNegative(Fire->BurningAttackPowerScale)
			&& FMath::IsFinite(Fire->BurningTickInterval)
			&& Fire->BurningTickInterval > 0.0f
			&& Fire->BurningTickCount > 0;
	}
	else if (const UPRThunderDropSkillFragment* Thunder = Cast<UPRThunderDropSkillFragment>(this))
	{
		bValid = FMath::IsFinite(Thunder->FallbackDistance) && Thunder->FallbackDistance > 0.0f;
	}
	else if (const UPRAfterimageDodgeSkillFragment* Dodge = Cast<UPRAfterimageDodgeSkillFragment>(this))
	{
		bValid = IsFiniteNonNegative(Dodge->PerfectTimingWindow)
			&& FMath::IsFinite(Dodge->DecoyLifetime)
			&& Dodge->DecoyLifetime > 0.0f;
	}
	else if (const UPRVectorHookSkillFragment* Hook = Cast<UPRVectorHookSkillFragment>(this))
	{
		bValid = FMath::IsFinite(Hook->StopDistance) && Hook->StopDistance > 0.0f;
	}
	else if (const UPRCounterProofWallSkillFragment* Wall = Cast<UPRCounterProofWallSkillFragment>(this))
	{
		bValid = IsFiniteNonNegative(Wall->PerfectTimingWindow);
	}

	if (!bValid)
	{
		Context.AddError(FText::FromString(TEXT("SkillFragment contains invalid tuning values.")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif
