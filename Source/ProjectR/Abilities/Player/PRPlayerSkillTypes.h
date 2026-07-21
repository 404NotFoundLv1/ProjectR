// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRCombatMitigationInterface.h"
#include "GameplayTagContainer.h"

#include "PRPlayerSkillTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EPRPlayerSkillTargetPolicy : uint8
{
	ForwardSweep,
	ForwardArea,
	GroundArea,
	SingleTarget,
	Self
};

UENUM(BlueprintType)
enum class EPRAbilityTargetMobility : uint8
{
	Light,
	Heavy,
	Anchored
};

UENUM(BlueprintType)
enum class EPRPlayerSkillPhase : uint8
{
	Idle,
	Startup,
	Active,
	Recovery,
	Cancelled
};

UENUM()
enum class EPRAbilityDisplacementEndReason : uint8
{
	Completed,
	Blocked,
	Cancelled
};

USTRUCT()
struct PROJECTR_API FPRAbilityDisplacementRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector DesiredEndLocation = FVector::ZeroVector;

	UPROPERTY()
	float Duration = 0.0f;

	UPROPERTY()
	float StopDistance = 0.0f;

	UPROPERTY()
	bool bSweep = true;

	UPROPERTY()
	bool bStopOnBlockingHit = true;

	UPROPERTY()
	FGameplayTag AbilityTag;
};

USTRUCT()
struct PROJECTR_API FPRPlayerSkillExecutionSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag AbilityTag;

	UPROPERTY()
	FGameplayTag InputTag;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> PrimaryTarget;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Targets;

	UPROPERTY()
	FVector Origin = FVector::ZeroVector;

	UPROPERTY()
	FVector AimDirection = FVector::ZeroVector;

	UPROPERTY()
	float AttackPower = 0.0f;

	UPROPERTY()
	float BaseDamage = 0.0f;

	UPROPERTY()
	float AttackPowerScale = 0.0f;

	UPROPERTY()
	float EnergyCost = 0.0f;

	UPROPERTY()
	float CooldownDuration = 0.0f;

	UPROPERTY()
	bool bCritical = false;
};

/** Native-only deterministic target query. */
struct PROJECTR_API FPRAbilityTargetQuery
{
	TWeakObjectPtr<AActor> SourceActor;
	FGameplayTag AbilityTag;
	EPRPlayerSkillTargetPolicy TargetPolicy = EPRPlayerSkillTargetPolicy::Self;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	FVector AreaCenter = FVector::ZeroVector;
	float Range = 0.0f;
	float Radius = 0.0f;
	float HalfAngleDegrees = 0.0f;
};

struct PROJECTR_API FPRAbilityTargetQueryResult
{
	TWeakObjectPtr<AActor> PrimaryTarget;
	TArray<TWeakObjectPtr<AActor>> Targets;
	FVector ResolvedPoint = FVector::ZeroVector;
};

struct PROJECTR_API FPRAbilityDisplacementResult
{
	FGuid RequestId;
	FGameplayTag AbilityTag;
	TWeakObjectPtr<AActor> SourceActor;
	TWeakObjectPtr<AActor> TargetActor;
	FVector FinalLocation = FVector::ZeroVector;
	EPRAbilityDisplacementEndReason EndReason = EPRAbilityDisplacementEndReason::Cancelled;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FPRAbilityDisplacementFinishedNative,
	const FPRAbilityDisplacementResult&);
