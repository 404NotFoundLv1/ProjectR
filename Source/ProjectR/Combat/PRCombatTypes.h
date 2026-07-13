// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class AActor;
class UObject;

/** Validated request submitted to ProjectR's single damage entry point. */
struct PROJECTR_API FPRDamageRequest
{
	FName SourceId;
	TWeakObjectPtr<UObject> DamageSource;
	TWeakObjectPtr<AActor> Instigator;
	TWeakObjectPtr<AActor> Target;
	FGameplayTag AbilityTag;
	float RawDamage = 0.0f;
	bool bCritical = false;
	FGameplayTagContainer DamageTags;
};

/** Immutable combat fact published to downstream runtime consumers. */
struct PROJECTR_API FPRCombatEvent
{
	FGuid EventId;
	FGameplayTag EventTag;
	FName SourceId;
	FName TargetId;
	TWeakObjectPtr<UObject> DamageSource;
	TWeakObjectPtr<AActor> Instigator;
	TWeakObjectPtr<AActor> Target;
	FGameplayTag AbilityTag;
	float RawDamage = 0.0f;
	float ShieldAbsorbed = 0.0f;
	float HealthDamage = 0.0f;
	float RemainingHealth = 0.0f;
	float RemainingShield = 0.0f;
	bool bCritical = false;
	FGameplayTagContainer DamageTags;
	FGameplayTagContainer ResponseTags;
	bool bFatal = false;
	double WorldTimeSeconds = 0.0;
};

/** Explicit request to restore a dead combatant without replacing its pawn. */
struct PROJECTR_API FPRReviveRequest
{
	FName SourceId;
	TWeakObjectPtr<UObject> DamageSource;
	TWeakObjectPtr<AActor> Instigator;
	TWeakObjectPtr<AActor> Target;
	float HealthFraction = 1.0f;
	float ShieldFraction = 1.0f;
};

enum class EPRCombatRequestStatus : uint8
{
	Applied,
	RejectedInvulnerable,
	RejectedDead,
	RejectedAlive,
	Invalid
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRCombatEventNative, const FPRCombatEvent&);
