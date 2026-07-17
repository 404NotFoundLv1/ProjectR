// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/PREnemyTypes.h"
#include "Engine/DataAsset.h"

#include "PREnemyAttackDataAsset.generated.h"

class AActor;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

UCLASS(BlueprintType)
class PROJECTR_API UPREnemyAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FName AttackId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTag AttackTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	EPREnemyAttackKind Kind = EPREnemyAttackKind::MeleeSweep;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MinRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MaxRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float BaseDamage = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float AttackPowerScale = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Windup = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float ActiveWindow = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Recovery = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Cooldown = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float SweepRadius = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float SweepHalfAngleDegrees = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TSoftClassPtr<AActor> ProjectileClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float ProjectileSpeed = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float ProjectileLifetime = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTagContainer DamageTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UNiagaraSystem> VFX;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<USoundBase> SFX;
};
