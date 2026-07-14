// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillDataAsset.h"

#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UPRPlayerSkillDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRPlayerSkill")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPRPlayerSkillDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	bool bValid = true;
	auto AddError = [&Context, &bValid](const TCHAR* Message)
	{
		Context.AddError(FText::FromString(Message));
		bValid = false;
	};
	auto IsFiniteNonNegative = [](const float Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.0f;
	};

	if (!AbilityTag.IsValid() || !AbilityTag.ToString().StartsWith(TEXT("Skill.")))
	{
		AddError(TEXT("AbilityTag must be a valid Skill.* tag."));
	}
	if (!InputTag.IsValid() || !InputTag.ToString().StartsWith(TEXT("Input.")))
	{
		AddError(TEXT("InputTag must be a valid Input.* tag."));
	}
	if (!AbilityClass || !CostEffectClass || !CooldownEffectClass)
	{
		AddError(TEXT("AbilityClass, CostEffectClass, and CooldownEffectClass are required."));
	}

	const float NumericValues[] = {
		BaseDamage, AttackPowerScale, EnergyCost, CooldownDuration, StartupDuration,
		ActiveDuration, RecoveryDuration, Range, Radius, HalfAngleDegrees,
		DisplacementDistance, StatusDuration};
	for (const float Value : NumericValues)
	{
		if (!IsFiniteNonNegative(Value))
		{
			AddError(TEXT("All player skill tuning values must be finite and non-negative."));
			break;
		}
	}
	if (EnergyCost <= 0.0f || CooldownDuration <= 0.0f)
	{
		AddError(TEXT("EnergyCost and CooldownDuration must be positive."));
	}
	if (HalfAngleDegrees > 180.0f)
	{
		AddError(TEXT("HalfAngleDegrees must be in [0, 180]."));
	}

	switch (TargetPolicy)
	{
	case EPRPlayerSkillTargetPolicy::ForwardSweep:
		if (Range <= 0.0f || Radius <= 0.0f || DisplacementDistance <= 0.0f)
		{
			AddError(TEXT("ForwardSweep requires positive Range, Radius, and DisplacementDistance."));
		}
		break;
	case EPRPlayerSkillTargetPolicy::ForwardArea:
	case EPRPlayerSkillTargetPolicy::SingleTarget:
		if (Range <= 0.0f || HalfAngleDegrees <= 0.0f)
		{
			AddError(TEXT("ForwardArea and SingleTarget require positive Range and HalfAngleDegrees."));
		}
		break;
	case EPRPlayerSkillTargetPolicy::GroundArea:
		if (Range <= 0.0f || Radius <= 0.0f)
		{
			AddError(TEXT("GroundArea requires positive Range and Radius."));
		}
		break;
	case EPRPlayerSkillTargetPolicy::Self:
		break;
	}

	const UPRPlayerSkillGameplayAbility* AbilityCDO = AbilityClass
		? AbilityClass->GetDefaultObject<UPRPlayerSkillGameplayAbility>()
		: nullptr;
	if (AbilityCDO)
	{
		if (AbilityCDO->GetProjectRAbilityTag() != AbilityTag
			|| AbilityCDO->GetActivationPolicy() != ActivationPolicy)
		{
			AddError(TEXT("Ability CDO AbilityTag or ActivationPolicy does not match the Skill DataAsset."));
		}
		const UGameplayEffect* Cost = AbilityCDO->GetCostGameplayEffect();
		const UGameplayEffect* Cooldown = AbilityCDO->GetCooldownGameplayEffect();
		if (!Cost || Cost->GetClass() != CostEffectClass || !Cooldown || Cooldown->GetClass() != CooldownEffectClass)
		{
			AddError(TEXT("Ability CDO Cost/Cooldown classes do not match the Skill DataAsset."));
		}
	}

	const UGameplayEffect* Cost = CostEffectClass ? CostEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	float CostMagnitude = 0.0f;
	if (!Cost || Cost->DurationPolicy != EGameplayEffectDurationType::Instant
		|| Cost->Modifiers.Num() != 1
		|| Cost->Modifiers[0].Attribute != UPRAttributeSet::GetEnergyAttribute()
		|| Cost->Modifiers[0].ModifierOp != EGameplayModOp::Additive
		|| !Cost->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, CostMagnitude)
		|| !FMath::IsNearlyEqual(CostMagnitude, -EnergyCost))
	{
		AddError(TEXT("Cost GameplayEffect must be an Instant Energy additive modifier equal to -EnergyCost."));
	}

	const UGameplayEffect* Cooldown = CooldownEffectClass
		? CooldownEffectClass->GetDefaultObject<UGameplayEffect>()
		: nullptr;
	float Duration = 0.0f;
	const FString SkillLeaf = AbilityTag.IsValid()
		? AbilityTag.ToString().RightChop(FCString::Strlen(TEXT("Skill.")))
		: FString();
	const FGameplayTag ExpectedCooldownTag = SkillLeaf.IsEmpty()
		? FGameplayTag()
		: FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Cooldown.Skill.%s"), *SkillLeaf)), false);
	if (!Cooldown || Cooldown->DurationPolicy != EGameplayEffectDurationType::HasDuration
		|| !Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, Duration)
		|| !FMath::IsNearlyEqual(Duration, CooldownDuration)
		|| Cooldown->GetGrantedTags().Num() != 1
		|| !ExpectedCooldownTag.IsValid()
		|| !Cooldown->GetGrantedTags().HasTagExact(ExpectedCooldownTag))
	{
		AddError(TEXT("Cooldown GameplayEffect duration/tag must match the Skill DataAsset."));
	}

	if (SkillFragment)
	{
		FDataValidationContext FragmentContext;
		if (SkillFragment->IsDataValid(FragmentContext) == EDataValidationResult::Invalid)
		{
			AddError(TEXT("SkillFragment validation failed."));
		}
	}

	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
