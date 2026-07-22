// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRBasicAttackDataAsset.h"

#include "Abilities/PRGameplayAbility.h"
#include "Abilities/PRAbilityTypes.h"
#include "Core/PRTagLibrary.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UPRBasicAttackDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRBasicAttack")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPRBasicAttackDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	bool bValid = true;
	auto AddError = [&Context, &bValid](const TCHAR* Message)
	{
		Context.AddError(FText::FromString(Message));
		bValid = false;
	};
	const float NumericValues[] = {
		BaseDamage, AttackPowerScale, StartupDuration, ActiveDuration,
		RecoveryDuration, Range, Radius};
	for (const float Value : NumericValues)
	{
		if (!FMath::IsFinite(Value) || Value < 0.0f)
		{
			AddError(TEXT("BasicAttack tuning values must be finite and non-negative."));
			break;
		}
	}
	if (AbilityTag != UPRTagLibrary::GetSkillBasicAttackTag())
	{
		AddError(TEXT("BasicAttack must use Skill.BasicAttack."));
	}
	if (InputTag != UPRTagLibrary::GetInputAttackTag())
	{
		AddError(TEXT("BasicAttack must use Input.Attack."));
	}
	if (!AbilityClass || Range <= 0.0f || Radius <= 0.0f)
	{
		AddError(TEXT("BasicAttack requires an AbilityClass and positive ForwardSweep range/radius."));
	}
	else if (const UPRGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UPRGameplayAbility>())
	{
		if (AbilityCDO->GetProjectRAbilityTag() != AbilityTag
			|| AbilityCDO->GetActivationPolicy() != EPRAbilityActivationPolicy::OnInputTriggered
			|| AbilityCDO->GetCostGameplayEffect() != nullptr
			|| AbilityCDO->GetCooldownGameplayEffect() != nullptr)
		{
			AddError(TEXT("BasicAttack GA must match tag/policy and own no Cost or Cooldown GameplayEffect."));
		}
	}
	else
	{
		AddError(TEXT("BasicAttack AbilityClass has no ProjectR ability CDO."));
	}
	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
