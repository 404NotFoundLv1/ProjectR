// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"
#include "GameplayTagContainer.h"

#include "PRQTETypes.generated.h"

UENUM(BlueprintType)
enum class EPRQTEState : uint8
{
	Idle,
	Active,
	Resolving,
	Cooldown
};

UENUM(BlueprintType)
enum class EPRQTETriggerSource : uint8
{
	CombatEvent,
	CompanionSupportEvent
};

UENUM(BlueprintType)
enum class EPRQTETriggerKind : uint8
{
	ShieldBroken,
	LowHealthDamage,
	SupportDuringAuditorWindup,
	PredictionBlocked,
	KindleCrowdSupport,
	FireSlashThreeHits,
	ShadowThrustHit,
	DecoyConsumed,
	PerfectDecoyConsumed,
	NormalLowHealth
};

UENUM(BlueprintType)
enum class EPRQTEEffectKind : uint8
{
	Damage,
	ShieldAndInvulnerable,
	Stun,
	FutureSample,
	PulseDamage,
	ConfirmBasicAttackKill
};

/** Fixed whitelist used by v0.3.2 trigger and effect target selection. */
UENUM(BlueprintType)
enum class EPRQTETargetScope : uint8
{
	AnyEnemy,
	LightEnemy,
	EliteOrBoss,
	NonBossEnemy,
	NormalEnemy
};

UENUM(BlueprintType)
enum class EPRQTETimingGrade : uint8
{
	None,
	Perfect,
	Standard
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRQTERequest
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FPrimaryAssetId QTEId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid TriggerEventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETriggerSource TriggerSource = EPRQTETriggerSource::CombatEvent;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FName SourceId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FName TargetId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag AbilityTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTagContainer ResponseTags;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") double WorldTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRQTERuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FPrimaryAssetId QTEId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag QTEType;
	/** Presentation-only copies from the active approved QTE data. The UI never loads or owns gameplay data. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FText DisplayName;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FText PromptText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FName TargetId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTEState State = EPRQTEState::Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTagContainer AcceptedInputTags;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") double StartTimeSeconds = 0.0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") double DeadlineTimeSeconds = 0.0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") float RemainingSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRQTEEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTEEffectKind EffectKind = EPRQTEEffectKind::Damage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") float Magnitude = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") float Range = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") int32 MaxTargets = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") int32 PulseCount = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") float PulseIntervalSeconds = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FName FutureSampleId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETargetScope TargetScope = EPRQTETargetScope::AnyEnemy;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRQTEDeltaSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FPRRelationshipDelta Success;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FPRRelationshipDelta Failure;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FPRRelationshipDelta Rejected;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FPRRelationshipDelta Timeout;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRQTEResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid ResultId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FPrimaryAssetId QTEId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag ResultTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid TriggerEventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FName TargetId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag SubmittedInput;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETimingGrade TimingGrade = EPRQTETimingGrade::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") TArray<FName> AppliedEffectIds;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FPRRelationshipDelta RelationshipDeltaRequest;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") uint8 RelationshipApplyResult = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGuid SaveRequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTagContainer ProfileSampleTags;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|QTE") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRQTEStateChangedNative, const FPRQTERuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRQTEResultNative, const FPRQTEResult&);

/** Immutable identifiers and priority rules shared by asset validation and runtime selection. */
class PROJECTR_API FPRQTEContract
{
public:
	static const TArray<FName>& GetExpectedQTEIds();
	static int32 GetTypePriority(const FGameplayTag& QTEType);
	static bool IsExpectedQTEId(FName QTEId);
	static float CalculateEffectiveCooldown(float BaseCooldownSeconds, int32 Overload);
};
