// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/PREnemyTypes.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Character.h"

#include "PREnemyPrototypeDataAsset.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UPRAbilitySetDataAsset;
class UPREnemyAttackDataAsset;

USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyPrototypeRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTag PrototypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TSoftObjectPtr<class UPREnemyPrototypeDataAsset> Prototype;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TSoftClassPtr<ACharacter> EnemyClass;
};

UCLASS(BlueprintType)
class PROJECTR_API UPREnemyPrototypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FName EnemyId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTag PrototypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	EPRAbilityTargetMobility Mobility = EPRAbilityTargetMobility::Light;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FPREnemyAttributeDefaults Attributes;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	FPREnemyPerceptionConfig Perception;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TObjectPtr<UPRAbilitySetDataAsset> InitialAbilitySet = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TArray<TObjectPtr<UPREnemyAttackDataAsset>> AttackDefinitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UAnimMontage> HitMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UAnimMontage> DeathMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UNiagaraSystem> HitVFX;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<UNiagaraSystem> DeathVFX;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<USoundBase> HitSFX;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy|Presentation")
	TSoftObjectPtr<USoundBase> DeathSFX;
};

UCLASS(BlueprintType)
class PROJECTR_API UPREnemyPrototypeRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	const TArray<FPREnemyPrototypeRegistryEntry>& GetEntries() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	TArray<FPREnemyPrototypeRegistryEntry> Entries;
};
