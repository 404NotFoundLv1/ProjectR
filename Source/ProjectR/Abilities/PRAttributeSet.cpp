// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/PRAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UPRAttributeSet::UPRAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitMaxShield(50.0f);
	InitShield(50.0f);
	InitMaxEnergy(100.0f);
	InitEnergy(100.0f);
	InitAttackPower(10.0f);
	InitMoveSpeed(600.0f);
	InitCritChance(0.05f);
	InitPermission(0.0f);
	InitResonance(0.0f);
	InitIncomingDamage(0.0f);
}

void UPRAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 1.0f, 100000.0f);
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 100000.0f);
		AdjustAttributeForMaxChange(Shield, MaxShield, NewValue, GetShieldAttribute());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 100000.0f);
		AdjustAttributeForMaxChange(Energy, MaxEnergy, NewValue, GetEnergyAttribute());
	}
	else if (Attribute == GetEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEnergy());
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 100000.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 2000.0f);
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	else if (Attribute == GetPermissionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 999999.0f);
	}
	else if (Attribute == GetResonanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
	}
}

void UPRAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.0f);
		if (!FMath::IsFinite(Damage) || Damage <= 0.0f)
		{
			return;
		}

		const float ShieldAbsorbed = FMath::Min(GetShield(), Damage);
		const float RemainingDamage = FMath::Max(0.0f, Damage - ShieldAbsorbed);
		SetShield(FMath::Clamp(GetShield() - ShieldAbsorbed, 0.0f, GetMaxShield()));
		SetHealth(FMath::Clamp(GetHealth() - RemainingDamage, 0.0f, GetMaxHealth()));
		return;
	}
	ClampEvaluatedAttribute(Data.EvaluatedData.Attribute);
}

void UPRAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, Energy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, Permission, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet, Resonance, COND_None, REPNOTIFY_Always);
}

void UPRAttributeSet::AdjustAttributeForMaxChange(
	const FGameplayAttributeData& AffectedAttribute,
	const FGameplayAttributeData& MaxAttribute,
	float NewMaxValue,
	const FGameplayAttribute& AffectedAttributeProperty) const
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const float OldMaxValue = MaxAttribute.GetCurrentValue();
	const float CurrentValue = AffectedAttribute.GetCurrentValue();
	const float AdjustedValue = OldMaxValue > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentValue * NewMaxValue / OldMaxValue, 0.0f, NewMaxValue)
		: FMath::Clamp(CurrentValue, 0.0f, NewMaxValue);
	if (!FMath::IsNearlyEqual(CurrentValue, AdjustedValue))
	{
		ASC->ApplyModToAttributeUnsafe(
			AffectedAttributeProperty,
			EGameplayModOp::Additive,
			AdjustedValue - CurrentValue);
	}
}

void UPRAttributeSet::ClampEvaluatedAttribute(const FGameplayAttribute& Attribute)
{
	if (Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Clamp(GetMaxHealth(), 1.0f, 100000.0f));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		SetMaxShield(FMath::Clamp(GetMaxShield(), 0.0f, 100000.0f));
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
	else if (Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
	else if (Attribute == GetMaxEnergyAttribute())
	{
		SetMaxEnergy(FMath::Clamp(GetMaxEnergy(), 0.0f, 100000.0f));
		SetEnergy(FMath::Clamp(GetEnergy(), 0.0f, GetMaxEnergy()));
	}
	else if (Attribute == GetEnergyAttribute())
	{
		SetEnergy(FMath::Clamp(GetEnergy(), 0.0f, GetMaxEnergy()));
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		SetAttackPower(FMath::Clamp(GetAttackPower(), 0.0f, 100000.0f));
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Clamp(GetMoveSpeed(), 0.0f, 2000.0f));
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		SetCritChance(FMath::Clamp(GetCritChance(), 0.0f, 1.0f));
	}
	else if (Attribute == GetPermissionAttribute())
	{
		SetPermission(FMath::Clamp(GetPermission(), 0.0f, 999999.0f));
	}
	else if (Attribute == GetResonanceAttribute())
	{
		SetResonance(FMath::Clamp(GetResonance(), 0.0f, 100.0f));
	}
}

#define PR_ATTRIBUTE_REPNOTIFY(PropertyName) \
	void UPRAttributeSet::OnRep_##PropertyName(const FGameplayAttributeData& OldValue) const \
	{ \
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet, PropertyName, OldValue); \
	}

PR_ATTRIBUTE_REPNOTIFY(MaxHealth)
PR_ATTRIBUTE_REPNOTIFY(Health)
PR_ATTRIBUTE_REPNOTIFY(MaxShield)
PR_ATTRIBUTE_REPNOTIFY(Shield)
PR_ATTRIBUTE_REPNOTIFY(MaxEnergy)
PR_ATTRIBUTE_REPNOTIFY(Energy)
PR_ATTRIBUTE_REPNOTIFY(AttackPower)
PR_ATTRIBUTE_REPNOTIFY(MoveSpeed)
PR_ATTRIBUTE_REPNOTIFY(CritChance)
PR_ATTRIBUTE_REPNOTIFY(Permission)
PR_ATTRIBUTE_REPNOTIFY(Resonance)

#undef PR_ATTRIBUTE_REPNOTIFY
