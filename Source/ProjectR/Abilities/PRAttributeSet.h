// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "PRAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS_BASIC(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** Replicated baseline combat attributes. Gameplay semantics are added by later versions. */
UCLASS()
class PROJECTR_API UPRAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPRAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, Health)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, MaxShield)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, Shield)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, MaxEnergy)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, Energy)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, AttackPower)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, MoveSpeed)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, CritChance)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, Permission)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, Resonance)
	ATTRIBUTE_ACCESSORS_BASIC(UPRAttributeSet, IncomingDamage)

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="ProjectR|Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="ProjectR|Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxShield, Category="ProjectR|Attributes")
	FGameplayAttributeData MaxShield;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Shield, Category="ProjectR|Attributes")
	FGameplayAttributeData Shield;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxEnergy, Category="ProjectR|Attributes")
	FGameplayAttributeData MaxEnergy;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Energy, Category="ProjectR|Attributes")
	FGameplayAttributeData Energy;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackPower, Category="ProjectR|Attributes")
	FGameplayAttributeData AttackPower;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, Category="ProjectR|Attributes")
	FGameplayAttributeData MoveSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CritChance, Category="ProjectR|Attributes")
	FGameplayAttributeData CritChance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Permission, Category="ProjectR|Attributes")
	FGameplayAttributeData Permission;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Resonance, Category="ProjectR|Attributes")
	FGameplayAttributeData Resonance;

	/** Transient GAS meta attribute consumed once by PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Transient, Category="ProjectR|Attributes")
	FGameplayAttributeData IncomingDamage;

private:
	void AdjustAttributeForMaxChange(
		const FGameplayAttributeData& AffectedAttribute,
		const FGameplayAttributeData& MaxAttribute,
		float NewMaxValue,
		const FGameplayAttribute& AffectedAttributeProperty) const;
	void ClampEvaluatedAttribute(const FGameplayAttribute& Attribute);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_Energy(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_Permission(const FGameplayAttributeData& OldValue) const;
	UFUNCTION()
	void OnRep_Resonance(const FGameplayAttributeData& OldValue) const;
};
