// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyPrototypeDataAsset.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Misc/DataValidation.h"

namespace PREnemyDataValidation
{
void AddError(FDataValidationContext& Context, EDataValidationResult& Result, const FString& Message)
{
	Context.AddError(FText::FromString(Message));
	Result = EDataValidationResult::Invalid;
}

bool IsFinite(const FPREnemyAttributeDefaults& Values)
{
	return FMath::IsFinite(Values.Health) && FMath::IsFinite(Values.MaxHealth)
		&& FMath::IsFinite(Values.Shield) && FMath::IsFinite(Values.MaxShield)
		&& FMath::IsFinite(Values.Energy) && FMath::IsFinite(Values.MaxEnergy)
		&& FMath::IsFinite(Values.AttackPower) && FMath::IsFinite(Values.MoveSpeed)
		&& FMath::IsFinite(Values.CritChance) && FMath::IsFinite(Values.Permission)
		&& FMath::IsFinite(Values.Resonance);
}
} // namespace PREnemyDataValidation

FPrimaryAssetId UPREnemyPrototypeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectREnemyPrototype")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPREnemyPrototypeDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (EnemyId.IsNone() || !PrototypeTag.IsValid() || !PrototypeTag.ToString().StartsWith(TEXT("Enemy.Type.")))
	{
		PREnemyDataValidation::AddError(Context, Result, TEXT("EnemyId and Enemy.Type.* PrototypeTag are required."));
	}
	if (!PREnemyDataValidation::IsFinite(Attributes) || Attributes.MaxHealth <= 0.0f || Attributes.Health < 0.0f
		|| Attributes.MaxShield < 0.0f || Attributes.Shield < 0.0f || Attributes.MaxEnergy < 0.0f
		|| Attributes.Energy < 0.0f || Attributes.AttackPower < 0.0f || Attributes.MoveSpeed < 0.0f
		|| Attributes.CritChance < 0.0f || Attributes.CritChance > 1.0f || Attributes.Permission < 0.0f
		|| Attributes.Resonance < 0.0f)
	{
		PREnemyDataValidation::AddError(Context, Result, TEXT("Enemy attributes are invalid."));
	}
	if (!FMath::IsFinite(Perception.SenseInterval) || !FMath::IsFinite(Perception.AcquireRange)
		|| !FMath::IsFinite(Perception.LoseRange) || !FMath::IsFinite(Perception.PreferredMinRange)
		|| !FMath::IsFinite(Perception.PreferredMaxRange) || !FMath::IsFinite(Perception.EdgeProbeForward)
		|| !FMath::IsFinite(Perception.EdgeProbeDepth) || Perception.SenseInterval <= 0.0f
		|| Perception.AcquireRange <= 0.0f || Perception.LoseRange < Perception.AcquireRange
		|| Perception.PreferredMinRange < 0.0f || Perception.PreferredMaxRange <= Perception.PreferredMinRange
		|| Perception.EdgeProbeForward <= 0.0f || Perception.EdgeProbeDepth <= 0.0f)
	{
		PREnemyDataValidation::AddError(Context, Result, TEXT("Enemy perception configuration is invalid."));
	}
	if (!InitialAbilitySet || AttackDefinitions.IsEmpty() || AttackDefinitions.Contains(nullptr))
	{
		PREnemyDataValidation::AddError(Context, Result, TEXT("Enemy InitialAbilitySet and AttackDefinitions are required."));
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

FPrimaryAssetId UPREnemyPrototypeRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectREnemyPrototypeRegistry")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPREnemyPrototypeRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TSet<FGameplayTag> UniqueTags;
	for (const FPREnemyPrototypeRegistryEntry& Entry : Entries)
	{
		if (!Entry.PrototypeTag.IsValid() || !Entry.PrototypeTag.ToString().StartsWith(TEXT("Enemy.Type."))
			|| Entry.Prototype.IsNull() || Entry.EnemyClass.IsNull() || UniqueTags.Contains(Entry.PrototypeTag))
		{
			PREnemyDataValidation::AddError(Context, Result, TEXT("Registry entries require unique Enemy.Type.* tags, a Prototype, and an EnemyClass."));
		}
		UniqueTags.Add(Entry.PrototypeTag);
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

const TArray<FPREnemyPrototypeRegistryEntry>& UPREnemyPrototypeRegistryDataAsset::GetEntries() const
{
	return Entries;
}
