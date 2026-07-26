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
#endif
private:
	void BeginProfileSession();
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleRelationshipChanged(const struct FPRRelationshipChangedEvent& Event);
	void HandlePrimarySyncChanged(const struct FPRPrimaryCompanionSyncChangedEvent& Event);
	void HandleDivergenceResult(const struct FPRDivergenceResult& Result);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void HandleAbilityLifecycle(const struct FPRAbilityLifecycleEvent& Event);
	void AddTaggedCount(TArray<FPRPlayerProfileTaggedCount>& Counts, FGameplayTag Tag, int32 Amount);
	bool ConsumeUniqueId(const FGuid& Id);
	void PublishProfileChange();
	mutable FPRPlayerProfileSnapshot Snapshot;
	FPRPlayerProfileChangedNative ProfileChanged;
	TWeakObjectPtr<UWorld> BoundWorld;
	TArray<FGuid> RecentIds;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle RelationshipChangedHandle;
	FDelegateHandle PrimarySyncHandle;
	FDelegateHandle DivergenceResultHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle AbilityLifecycleHandle;
	bool bHasSession = false;
};
