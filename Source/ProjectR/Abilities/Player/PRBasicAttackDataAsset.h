// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRBasicAttackDataAsset.generated.h"

class UPRGameplayAbility;

/** Data-owned zero-resource standard attack contract; it remains distinct from the six formal PlayerSkill assets. */
UCLASS(BlueprintType)
class PROJECTR_API UPRBasicAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack")
	TSubclassOf<UPRGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Damage", meta=(ClampMin="0"))
	float BaseDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Damage", meta=(ClampMin="0"))
	float AttackPowerScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Timing", meta=(ClampMin="0"))
	float StartupDuration = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Timing", meta=(ClampMin="0"))
	float ActiveDuration = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Timing", meta=(ClampMin="0"))
	float RecoveryDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Targeting", meta=(ClampMin="0"))
	float Range = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|BasicAttack|Targeting", meta=(ClampMin="0"))
	float Radius = 45.0f;
};
