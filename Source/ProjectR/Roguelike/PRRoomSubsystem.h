// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Roguelike/PRRewardTypes.h"
#include "Roguelike/PRRoomTypes.h"
#include "GameplayEffectTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRRoomSubsystem.generated.h"

class UPRRoguelikeContentRegistryDataAsset;

/** Sole owner of a bounded, non-persistent roguelike room session. */
UCLASS()
class PROJECTR_API UPRRoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Pure bounded contract helper used by deterministic path construction and native automation. */
	static int32 GetRoomPathLengthForSeed(int32 Seed);
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EPRRoomOperationResult StartRoomSequence(int32 Seed, FGuid& OutSessionId);
	EPRRoomOperationResult SelectRoom(FPrimaryAssetId RoomId);
	EPRRoomOperationResult SelectEventChoice(FName ChoiceId);
	EPRRoomOperationResult ConfirmSafeRoomExit();
	EPRRoomOperationResult SelectReward(FPrimaryAssetId RewardId, FGuid& OutHandleId);
	bool GetRoomRuntimeState(FPRRoomRuntimeState& OutState) const;
	/** Returns only the active encounter's registered runtime spawn handles; callers cannot mutate session ownership. */
	void GetActiveEncounterSpawnIds(TArray<FGuid>& OutSpawnIds) const;
	void GetAppliedRewards(TArray<FPRRewardApplicationHandle>& OutHandles) const;
	FPRRoomStateChangedNative& OnRoomStateChanged();
	FPRRewardOfferChangedNative& OnRewardOfferChanged();
	FPRRoomEventResolvedNative& OnRoomEventResolved();
	FPRRoomSequenceCompletedNative& OnRoomSequenceCompleted();

private:
	bool BuildPath();
	bool IsRoomEligible(const class UPRRoomDataAsset& Room) const;
	bool IsRewardEligible(const class UPRRewardDataAsset& Reward) const;
	bool IsRelationshipDeltaEmpty(const struct FPRRelationshipDelta& Delta) const;
	int32 GetRoomWeight(const class UPRRoomDataAsset& Room) const;
	bool DoesConditionPass(const FPRRoomCondition& Condition) const;
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void StartEncounter();
	void CheckEncounterCompletion();
	void HandleEnemyStateChanged(const struct FPREnemyRuntimeState& State);
	void HandleBossCompleted(const struct FPRPrototypeRunResult& Result);
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void CompleteEncounter();
	void CreateRewardOffer();
	void CompleteSequence();
	class UAbilitySystemComponent* ResolvePlayerAbilitySystem() const;
	bool RebindSessionGameplayEffects();
	void ClearSessionGameplayEffects();
	void ClearWorldBindings();
	void BroadcastState();
	void ResetSession();
	TSoftObjectPtr<UPRRoguelikeContentRegistryDataAsset> RegistryAsset;
	TObjectPtr<UPRRoguelikeContentRegistryDataAsset> Registry = nullptr;
	FPRRoomRuntimeState RuntimeState;
	FPRRewardOffer ActiveOffer;
	TArray<FPRRewardApplicationHandle> AppliedRewards;
	TMap<FGuid, struct FActiveGameplayEffectHandle> GameplayEffectHandles;
	TArray<FGuid> ActiveEncounterSpawnIds;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle EnemyStateChangedHandle;
	FDelegateHandle BossCompletedHandle;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FTimerHandle EncounterRetryTimer;
	FTimerHandle EncounterCompletionTimer;
	FGuid LastCombatEventId;
	FGameplayTag LastCombatEventTag;
	FGameplayTag LastQTEResultTag;
	bool bCurrentOfferEpicWeightBoosted = false;
	FPRRoomStateChangedNative StateChanged;
	FPRRewardOfferChangedNative RewardOfferChanged;
	FPRRoomEventResolvedNative EventResolved;
	FPRRoomSequenceCompletedNative SequenceCompleted;
};
