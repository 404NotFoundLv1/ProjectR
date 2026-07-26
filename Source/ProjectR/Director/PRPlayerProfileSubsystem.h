// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRPlayerProfileTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRPlayerProfileSubsystem.generated.h"

/** Sole owner of a bounded, current-session player profile. */
UCLASS()
class PROJECTR_API UPRPlayerProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool GetSnapshot(FPRPlayerProfileSnapshot& OutSnapshot) const;
	FPRPlayerProfileChangedNative& OnPlayerProfileChanged();
#if WITH_DEV_AUTOMATION_TESTS
	void BeginProfileSessionForAutomation();
	/** Test-only value injection used by the fixed Director smoke; unavailable in non-automation builds. */
	void InjectAbilityLifecycleForAutomation(const struct FPRAbilityLifecycleEvent& Event);
	void InjectCombatEventForAutomation(const struct FPRCombatEvent& Event);
	void InjectQTEResultForAutomation(const struct FPRQTEResult& Result);
	void InjectRelationshipChangedForAutomation(const struct FPRRelationshipChangedEvent& Event);
	void InjectDivergenceResultForAutomation(const struct FPRDivergenceResult& Result);
#endif
private:
	void BeginProfileSession();
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleRelationshipChanged(const struct FPRRelationshipChangedEvent& Event);
	void HandlePrimarySyncChanged(const struct FPRPrimaryCompanionSyncChangedEvent& Event);
	void HandleDivergenceResult(const struct FPRDivergenceResult& Result);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldBeginPlay();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void BindPlayerSources(UWorld* World);
	void UnbindPlayerSources();
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void HandleAbilityLifecycle(const struct FPRAbilityLifecycleEvent& Event);
	void HandleAttributeChanged(const struct FPRAttributeChange& Change);
	void AddTaggedCount(TArray<FPRPlayerProfileTaggedCount>& Counts, FGameplayTag Tag, int32 Amount);
	bool ConsumeUniqueId(const FGuid& Id);
	void PublishProfileChange();
	mutable FPRPlayerProfileSnapshot Snapshot;
	FPRPlayerProfileChangedNative ProfileChanged;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<class APRPlayerState> BoundPlayerState;
	TWeakObjectPtr<class UPRAbilitySystemComponent> BoundAbilitySystem;
	TArray<FGuid> RecentIds;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle RelationshipChangedHandle;
	FDelegateHandle PrimarySyncHandle;
	FDelegateHandle DivergenceResultHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldBeginPlayHandle;
	FDelegateHandle WorldCleanupHandle;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle AbilityLifecycleHandle;
	FDelegateHandle AttributeChangedHandle;
	bool bHasSession = false;
};
