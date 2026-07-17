// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"

#include "PREnemyTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EPREnemyBrainState : uint8
{
	Dormant = 0,
	AcquireTarget = 1,
	Pursue = 2,
	Retreat = 3,
	HoldRange = 4,
	Attack = 5,
	Staggered = 6,
	Dead = 7
};

UENUM(BlueprintType)
enum class EPREnemyAttackPhase : uint8
{
	None = 0,
	Windup = 1,
	Active = 2,
	Recovery = 3,
	Cooldown = 4
};

UENUM(BlueprintType)
enum class EPREnemyAttackKind : uint8
{
	MeleeSweep = 0,
	Projectile = 1
};

UENUM(BlueprintType)
enum class EPREnemySpawnStatus : uint8
{
	Spawned = 0,
	NotReady = 1,
	NotAuthoritative = 2,
	UnknownPrototype = 3,
	InvalidTransform = 4,
	SpawnFailed = 5,
	Invalid = 6
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyAttributeDefaults
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Health = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MaxHealth = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Shield = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MaxShield = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Energy = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MaxEnergy = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float AttackPower = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float MoveSpeed = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float CritChance = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Permission = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float Resonance = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyPerceptionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float SenseInterval = 0.10f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float AcquireRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float LoseRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float PreferredMinRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float PreferredMaxRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float EdgeProbeForward = 60.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy")
	float EdgeProbeDepth = 120.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	FGuid SpawnId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	FName CombatantId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTag PrototypeTag;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	EPRAbilityTargetMobility Mobility = EPRAbilityTargetMobility::Light;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	EPREnemyBrainState BrainState = EPREnemyBrainState::Dormant;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	EPREnemyAttackPhase AttackPhase = EPREnemyAttackPhase::None;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	FGameplayTag ActiveAttackTag;
	TWeakObjectPtr<AActor> Target;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	bool bAlive = false;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	bool bGrounded = false;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Enemy")
	double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPREnemyStateChangedNative, const FPREnemyRuntimeState&);
