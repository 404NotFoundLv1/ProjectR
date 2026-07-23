// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "QTE/PRQTETypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRQTESubsystem.generated.h"

class AActor;
class APRPlayerCharacter;
class UPRCombatSubsystem;
class UPRCompanionRuntimeSubsystem;
class UPRQTEComponent;
class UPRQTEDataAsset;
class UPRQTERegistryDataAsset;
class UPRAbilitySystemComponent;
class UGameplayEffect;
struct FPRCombatEvent;
struct FPRCompanionSupportEvent;

/** The sole runtime owner of deterministic QTE selection, resolution, cooldown and cleanup. */
UCLASS()
class PROJECTR_API UPRQTESubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsRegistryReady() const;
	FPRQTERuntimeState GetRuntimeState() const;
	bool SubmitSemanticInput(FGameplayTag InputTag, double InputTimeSeconds);
	void CancelActiveQTE();
	FPRQTEStateChangedNative& OnQTEStateChanged();
	FPRQTEResultNative& OnQTEResult();

private:
	struct FActiveQTE
	{
		TWeakObjectPtr<UPRQTEDataAsset> Asset;
		TWeakObjectPtr<AActor> SourceActor;
		TWeakObjectPtr<AActor> TargetActor;
		FPRQTERequest Request;
		FTimerHandle TimeoutTimer;
		FTimerHandle ConfirmationTimer;
		TArray<TWeakObjectPtr<AActor>> EffectTargets;
		bool bAwaitingBasicAttackKill = false;
	};
	struct FFireSlashHit
	{
		double WorldTimeSeconds = 0.0;
		FName TargetId;
		TWeakObjectPtr<AActor> Target;
	};
	struct FRecentEventRecord
	{
		FGuid EventId;
		double WorldTimeSeconds = 0.0;
		EPRQTETriggerSource Source = EPRQTETriggerSource::CombatEvent;
	};
	struct FRecentRequestRecord
	{
		FGuid RequestId;
		double WorldTimeSeconds = 0.0;
	};

	void LoadFixedRegistry();
	void ReconcileInputBridge();
	void HandleCombatEvent(const FPRCombatEvent& Event);
	void HandleSupportEvent(const FPRCompanionSupportEvent& Event);
	bool TryStartFromCombatEvent(const FPRCombatEvent& Event);
	bool TryStartFromSupportEvent(const FPRCompanionSupportEvent& Event);
	bool IsCandidateEligible(const UPRQTEDataAsset& Asset) const;
	bool DoesCombatEventMatch(const UPRQTEDataAsset& Asset, const FPRCombatEvent& Event) const;
	bool DoesSupportEventMatch(const UPRQTEDataAsset& Asset, const FPRCompanionSupportEvent& Event) const;
	bool IsDuplicateEvent(const FGuid& EventId, EPRQTETriggerSource Source, double EventTimeSeconds);
	bool StartQTE(UPRQTEDataAsset* Asset, const FPRQTERequest& Request, AActor* SourceActor, AActor* TargetActor);
	void HandleTimeout();
	void HandleConfirmationTimeout();
	void ResolveActiveQTE(FGameplayTag ResultTag, FGameplayTag SubmittedInput, EPRQTETimingGrade TimingGrade);
	void CompleteResolvedQTE(UPRQTEDataAsset& Asset, FPRQTEResult&& Result);
	bool ApplySuccessEffects(const UPRQTEDataAsset& Asset, FPRQTEResult& InOutResult);
	bool ApplyEffect(const UPRQTEDataAsset& Asset, const FPRQTEEffectDefinition& Effect, FPRQTEResult& InOutResult);
	void ApplyPulseDamage(TWeakObjectPtr<UPRQTEDataAsset> Asset, TWeakObjectPtr<AActor> Target, float Damage, FName EffectId);
	AActor* ResolveEffectTarget(float Range, bool bAllowPlayerTarget, EPRQTETargetScope TargetScope) const;
	TArray<AActor*> ResolveEffectTargets(float Range, int32 MaximumTargets, EPRQTETargetScope TargetScope) const;
	int32 CountEffectTargets(float Range, EPRQTETargetScope TargetScope) const;
	bool DoesTargetMatchScope(const AActor* Target, EPRQTETargetScope TargetScope) const;
	TArray<TWeakObjectPtr<AActor>> GetRecentFireSlashTargets() const;
	bool ApplyDamageToTarget(AActor& Target, float RequestedDamage, const UPRQTEDataAsset& Asset, FName EffectId, FPRQTEResult& InOutResult);
	bool ApplyStunToTarget(AActor& Target, FName EffectId, FPRQTEResult& InOutResult);
	bool ApplyPlayerDefense(float ShieldAmount, FName EffectId, FPRQTEResult& InOutResult);
	bool IsPlayerLowHealthDamage(const FPRCombatEvent& Event, float RequiredHealthFraction) const;
	bool IsNormalEnemyLowHealth(const FPRCombatEvent& Event, float RequiredHealthFraction) const;
	bool IsBasicAttackKillForActiveQTE(const FPRCombatEvent& Event) const;
	void HandlePlayerDeathTagChanged(FGameplayTag Tag, int32 NewCount);
	void ClearRuntimeEffectTimers();
	void BroadcastState(EPRQTEState NewState);
	void ClearActiveQTE();
	void ScheduleCooldown(float CooldownSeconds);
	void EndCooldown();
	void ReconcilePlayerBinding();
	float CalculateEffectiveCooldown(const UPRQTEDataAsset& Asset) const;
	double GetWorldTimeSeconds() const;

	TSoftObjectPtr<UPRQTERegistryDataAsset> RegistryAsset;
	TArray<TObjectPtr<UPRQTEDataAsset>> LoadedAssets;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UPRCompanionRuntimeSubsystem> CompanionRuntimeSubsystem;
	TWeakObjectPtr<UPRQTEComponent> InputBridge;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle SupportEventHandle;
	FTimerHandle InputReconcileTimer;
	FTimerHandle CooldownTimer;
	TArray<FTimerHandle> RuntimeEffectTimers;
	TArray<FFireSlashHit> FireSlashHits;
	TSet<FName> ShieldBrokenTargetIds;
	TArray<FRecentEventRecord> RecentEvents;
	TArray<FRecentRequestRecord> RecentRequests;
	TWeakObjectPtr<UPRAbilitySystemComponent> BoundPlayerASC;
	FDelegateHandle PlayerDeadTagHandle;
	TSubclassOf<UGameplayEffect> AxiomShieldEffectClass;
	TSubclassOf<UGameplayEffect> StunnedEffectClass;
	TSubclassOf<UGameplayEffect> InvulnerableEffectClass;
	FActiveQTE ActiveQTE;
	FPRQTERuntimeState RuntimeState;
	FPRQTEStateChangedNative StateChanged;
	FPRQTEResultNative ResultPublished;
	bool bRegistryReady = false;
};
