// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRAbilityTypes.h"
#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "Engine/DataAsset.h"

#include "PRPlayerSkillDataAsset.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class UPRPlayerSkillFragment;
class UPRPlayerSkillGameplayAbility;
class USoundBase;

/** Primary tuning and reference contract for one formal P0 player skill. */
UCLASS(BlueprintType)
class PROJECTR_API UPRPlayerSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	TSubclassOf<UPRPlayerSkillGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	TSubclassOf<UGameplayEffect> CostEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	EPRAbilityActivationPolicy ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill")
	EPRPlayerSkillTargetPolicy TargetPolicy = EPRPlayerSkillTargetPolicy::Self;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Damage", meta=(ClampMin="0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Damage", meta=(ClampMin="0"))
	float AttackPowerScale = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|GAS", meta=(ClampMin="0"))
	float EnergyCost = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|GAS", meta=(ClampMin="0"))
	float CooldownDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float StartupDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float ActiveDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float RecoveryDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Targeting", meta=(ClampMin="0"))
	float Range = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Targeting", meta=(ClampMin="0"))
	float Radius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Targeting", meta=(ClampMin="0", ClampMax="180"))
	float HalfAngleDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Movement", meta=(ClampMin="0"))
	float DisplacementDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Status", meta=(ClampMin="0"))
	float StatusDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="ProjectR|Skill")
	TObjectPtr<UPRPlayerSkillFragment> SkillFragment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Presentation")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Presentation")
	TSoftObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Presentation")
	TSoftObjectPtr<USoundBase> SFX;
};
