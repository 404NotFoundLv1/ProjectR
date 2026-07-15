// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "GameplayEffectTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRPlayerSkillSubsystem.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UGameplayEffect;
class UObject;
class UPRAbilitySystemComponent;
class UPRPlayerSkillDataAsset;
class UPRFireSlashSkillFragment;
enum class EPRCombatRequestStatus : uint8;
struct FPRCombatOutcomeRequest;

/** World-owned deterministic target query and controlled-displacement dispatcher. */
UCLASS()
class PROJECTR_API UPRPlayerSkillSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool QueryTargets(
		const FPRAbilityTargetQuery& Query,
		FPRAbilityTargetQueryResult& OutResult,
		FGameplayTag& OutFailureTag) const;

	bool RequestDisplacement(
		const FPRAbilityDisplacementRequest& Request,
		FGuid& OutRequestId,
		FGameplayTag& OutFailureTag);
	bool CancelDisplacement(FGuid RequestId);

	FPRAbilityDisplacementFinishedNative& OnDisplacementFinished();
	bool PublishAbilityOutcome(const FPRCombatOutcomeRequest& Request) const;

private:
	struct FActiveDisplacement
	{
		FGuid RequestId;
		FGameplayTag AbilityTag;
		TWeakObjectPtr<AActor> SourceActor;
		TWeakObjectPtr<ACharacter> TargetCharacter;
		FVector PlaneOrigin = FVector::ZeroVector;
		FVector EffectiveEnd = FVector::ZeroVector;
		uint16 RootMotionSourceId = 0;
		FTimerHandle CompletionTimerHandle;
		FTimerHandle MonitorTimerHandle;
	};

	struct FActiveBurning
	{
		TWeakObjectPtr<UPRAbilitySystemComponent> TargetASC;
		TWeakObjectPtr<AActor> TargetOwner;
		TWeakObjectPtr<AActor> TargetAvatar;
		TWeakObjectPtr<UPRAbilitySystemComponent> SourceASC;
		TWeakObjectPtr<AActor> SourceOwner;
		TWeakObjectPtr<AActor> SourceAvatar;
		TWeakObjectPtr<UObject> AbilitySource;
		FName SourceId;
		FGameplayTag AbilityTag;
		float AttackPower = 0.0f;
		float BaseDamage = 0.0f;
		float AttackPowerScale = 0.0f;
		float TickInterval = 0.0f;
		FVector AimFallback = FVector::ZeroVector;
		FActiveGameplayEffectHandle EffectHandle;
		int32 RemainingTicks = 0;
		FTimerHandle TimerHandle;
		FDelegateHandle EffectRemovedHandle;
	};

	bool ResolveShadowPath(
		const ACharacter& Character,
		FVector AimDirection,
		float DesiredDistance,
		float PathRadius,
		FVector& OutEndLocation,
		float& OutSafeDistance,
		FGameplayTag& OutFailureTag) const;
	EPRCombatRequestStatus ApplySkillDamage(
		const FPRPlayerSkillExecutionSnapshot& Snapshot,
		AActor& Target,
		UObject& AbilitySource,
		FVector ImpactOrigin,
		float BaseDamage,
		float AttackPowerScale,
		bool bCritical,
		FVector DirectionFallback = FVector::ZeroVector,
		FName SourceIdOverride = NAME_None) const;
	bool ApplyOrRefreshBurning(
		const FPRPlayerSkillExecutionSnapshot& Snapshot,
		AActor& Target,
		UObject& AbilitySource,
		TSubclassOf<UGameplayEffect> BurningEffectClass,
		const UPRFireSlashSkillFragment& Fragment);
	void HandleBurningTick(TWeakObjectPtr<AActor> TargetAvatar);
	void HandleBurningEffectRemoved(
		const FGameplayEffectRemovalInfo& RemovalInfo,
		TWeakObjectPtr<AActor> TargetAvatar);
	void ExecuteBurningTick(TWeakObjectPtr<AActor> TargetAvatar, bool bAllowExpiredEffect);
	void CleanupBurning(TWeakObjectPtr<AActor> TargetAvatar, bool bRemoveEffect);
	void CleanupAllBurning(bool bRemoveEffects);
	bool IsBurningBindingValid(const FActiveBurning& Burning) const;

	void HandleDisplacementMonitor(FGuid RequestId);
	void HandleDisplacementDurationElapsed(FGuid RequestId);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void FinishDisplacement(
		FGuid RequestId,
		EPRAbilityDisplacementEndReason EndReason,
		bool bBroadcast);
	bool HasWorldStaticObstruction(
		const ACharacter& Character,
		FVector Start,
		FVector End,
		const AActor* SourceActor) const;
	void CleanupAllDisplacements();

	struct FActiveThunderDrop
	{
		FPRPlayerSkillExecutionSnapshot Snapshot;
		FVector ImpactPoint = FVector::ZeroVector;
		TWeakObjectPtr<UPRPlayerSkillDataAsset> SkillData;
		TSubclassOf<UGameplayEffect> StunnedEffectClass;
		FTimerHandle TimerHandle;
	};

	bool ScheduleThunderDrop(
		const FPRPlayerSkillExecutionSnapshot& Snapshot,
		FVector ImpactPoint,
		UPRPlayerSkillDataAsset& SkillData,
		TSubclassOf<UGameplayEffect> StunnedEffectClass);
	void ResolveThunderDrop(TWeakObjectPtr<UPRPlayerSkillDataAsset> SkillData);
	void CleanupAllThunderDrops();
	bool ApplyStunnedEffect(
		AActor& Source,
		AActor& Target,
		UPRPlayerSkillDataAsset& SkillData,
		TSubclassOf<UGameplayEffect> StunnedEffectClass) const;

	TMap<FGuid, FActiveDisplacement> ActiveDisplacements;
	TMap<TWeakObjectPtr<AActor>, FActiveBurning> ActiveBurning;
	TMap<TWeakObjectPtr<UPRPlayerSkillDataAsset>, FActiveThunderDrop> ActiveThunderDrops;
	FPRAbilityDisplacementFinishedNative DisplacementFinishedEvent;
	FDelegateHandle WorldCleanupHandle;

	friend class UPRGA_ShadowThrust;
	friend class UPRGA_FireSlash;
	friend class UPRGA_ThunderDrop;
	friend class UPRGA_AfterimageDodge;
	friend class FPRPlayerSkillDamageDefenseTest;
	friend class FPRPlayerSkillLifecycleTest;
	friend class FPRPlayerSkillTargetingTest;
};
