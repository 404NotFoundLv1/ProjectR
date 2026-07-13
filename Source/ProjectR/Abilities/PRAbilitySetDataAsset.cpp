// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/PRAbilitySetDataAsset.h"

#include "Abilities/PRAttributeSet.h"
#include "Abilities/PRGameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "Misc/DataValidation.h"

namespace PRAbilitySetValidation
{
void AddError(
	FDataValidationContext& Context,
	EDataValidationResult& Result,
	const FString& Message)
{
	Context.AddError(FText::FromString(Message));
	Result = EDataValidationResult::Invalid;
}

bool IsZeroPeriod(const UGameplayEffect* Effect)
{
	return Effect && FMath::IsNearlyZero(Effect->Period.GetValueAtLevel(1.0f));
}
} // namespace PRAbilitySetValidation

FPrimaryAssetId UPRAbilitySetDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRAbilitySet")), GetFName());
}

const TArray<FPRAbilitySetEntry>& UPRAbilitySetDataAsset::GetAbilityEntries() const
{
	return AbilityEntries;
}

#if WITH_EDITOR
EDataValidationResult UPRAbilitySetDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TSet<FGameplayTag> AbilityTags;
	TSet<FGameplayTag> InputTags;
	TSet<FGameplayTag> CooldownTags;
	TSet<TSubclassOf<UPRGameplayAbility>> AbilityClasses;

	for (int32 Index = 0; Index < AbilityEntries.Num(); ++Index)
	{
		const FPRAbilitySetEntry& Entry = AbilityEntries[Index];
		const FString Prefix = FString::Printf(TEXT("AbilityEntries[%d]"), Index);
		const UPRGameplayAbility* AbilityCDO = Entry.AbilityClass
			? Entry.AbilityClass->GetDefaultObject<UPRGameplayAbility>()
			: nullptr;
		if (!AbilityCDO)
		{
			Context.AddError(FText::FromString(Prefix + TEXT(" has no ProjectR AbilityClass.")));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		if (Entry.AbilityLevel < 1)
		{
			Context.AddError(FText::FromString(Prefix + TEXT(" has AbilityLevel below one.")));
			Result = EDataValidationResult::Invalid;
		}

		const FGameplayTag AbilityTag = AbilityCDO->GetProjectRAbilityTag();
		if (!AbilityTag.IsValid() || AbilityTags.Contains(AbilityTag))
		{
			Context.AddError(FText::FromString(Prefix + TEXT(" has an invalid or duplicate AbilityTag.")));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			AbilityTags.Add(AbilityTag);
		}
		if (AbilityClasses.Contains(Entry.AbilityClass))
		{
			Context.AddError(FText::FromString(Prefix + TEXT(" duplicates an AbilityClass.")));
			Result = EDataValidationResult::Invalid;
		}
		AbilityClasses.Add(Entry.AbilityClass);

		const bool bPassive = AbilityCDO->GetActivationPolicy() == EPRAbilityActivationPolicy::Passive;
		const bool bInputTagValid = Entry.InputTag.IsValid()
			&& Entry.InputTag.ToString().StartsWith(TEXT("Input."));
		if ((bPassive && Entry.InputTag.IsValid()) || (!bPassive && !bInputTagValid))
		{
			Context.AddError(FText::FromString(Prefix + TEXT(" violates its activation-policy InputTag rule.")));
			Result = EDataValidationResult::Invalid;
		}
		if (Entry.InputTag.IsValid())
		{
			if (InputTags.Contains(Entry.InputTag))
			{
				Context.AddError(FText::FromString(Prefix + TEXT(" duplicates an InputTag.")));
				Result = EDataValidationResult::Invalid;
			}
			InputTags.Add(Entry.InputTag);
		}
		for (const FGameplayTag& GrantedTag : Entry.GrantedSpecTags)
		{
			if (GrantedTag.ToString().StartsWith(TEXT("Input.")))
			{
				Context.AddError(FText::FromString(Prefix + TEXT(" contains Input.* in GrantedSpecTags.")));
				Result = EDataValidationResult::Invalid;
			}
		}

		if (const UGameplayEffect* Cost = AbilityCDO->GetCostGameplayEffect())
		{
			float CostMagnitude = 0.0f;
			const bool bValidCost = Cost->DurationPolicy == EGameplayEffectDurationType::Instant
				&& Cost->Modifiers.Num() == 1
				&& Cost->Modifiers[0].Attribute == UPRAttributeSet::GetEnergyAttribute()
				&& Cost->Modifiers[0].ModifierOp == EGameplayModOp::Additive
				&& Cost->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(
					Entry.AbilityLevel, CostMagnitude)
				&& FMath::IsFinite(CostMagnitude)
				&& CostMagnitude < 0.0f
				&& Cost->Executions.IsEmpty()
				&& Cost->FindComponent<UAbilitiesGameplayEffectComponent>() == nullptr
				&& Cost->GameplayCues.IsEmpty()
				&& Cost->GetGrantedTags().IsEmpty()
				&& Cost->GetStackingType() == EGameplayEffectStackingType::None
				&& PRAbilitySetValidation::IsZeroPeriod(Cost);
			if (!bValidCost)
			{
				PRAbilitySetValidation::AddError(
					Context, Result, Prefix + TEXT(" has an invalid Energy Cost GameplayEffect."));
			}
		}

		if (const UGameplayEffect* Cooldown = AbilityCDO->GetCooldownGameplayEffect())
		{
			float Duration = 0.0f;
			const FGameplayTagContainer& GrantedTags = Cooldown->GetGrantedTags();
			const bool bValidCooldown = Cooldown->DurationPolicy == EGameplayEffectDurationType::HasDuration
				&& Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(
					Entry.AbilityLevel, Duration)
				&& FMath::IsFinite(Duration)
				&& Duration > 0.0f
				&& Cooldown->Modifiers.IsEmpty()
				&& Cooldown->Executions.IsEmpty()
				&& Cooldown->FindComponent<UAbilitiesGameplayEffectComponent>() == nullptr
				&& Cooldown->GameplayCues.IsEmpty()
				&& Cooldown->GetStackingType() == EGameplayEffectStackingType::None
				&& PRAbilitySetValidation::IsZeroPeriod(Cooldown)
				&& GrantedTags.Num() == 1;
			if (!bValidCooldown)
			{
				PRAbilitySetValidation::AddError(
					Context, Result, Prefix + TEXT(" has an invalid Cooldown GameplayEffect."));
			}
			else
			{
				const FGameplayTag CooldownTag = GrantedTags.First();
				if (!CooldownTag.ToString().StartsWith(TEXT("Cooldown."))
					|| CooldownTags.Contains(CooldownTag))
				{
					PRAbilitySetValidation::AddError(
						Context, Result,
						Prefix + TEXT(" has an invalid or duplicate Cooldown Tag."));
				}
				else
				{
					CooldownTags.Add(CooldownTag);
				}
			}
		}
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif
