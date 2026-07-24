// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Divergence/PRDivergenceTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRDivergenceSubsystem.generated.h"

class APRPlayerCharacter;
class UPRCombatSubsystem;
class UPRCompanionSubsystem;
class UPRDivergenceComponent;
class UPRDivergenceDataAsset;
class UPRSaveSubsystem;
class UPRCompanionDialogueSubsystem;

/**
 * Run-local, value-only divergence arbiter.  It observes the established
 * combat death fact and delegates revival, relationships and saving to their
 * existing owners.
 */
UCLASS()
class PROJECTR_API UPRDivergenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsDefinitionReady() const;
	FPRDivergenceRuntimeState GetRuntimeState() const;
	FPRDivergenceResult GetLastResult() const;
	bool SubmitChoice(EPRDivergenceChoice Choice);
	FPRDivergenceStateChangedNative& OnDivergenceStateChanged();
	FPRDivergenceResultNative& OnDivergenceResult();

#if WITH_DEV_AUTOMATION_TESTS
	/** Test-only in-memory relationship/profile gateway; it never reaches SaveSubsystem storage. */
	void ConfigureAutomationProfile(FGameplayTag CompanionId, const FPRRelationshipState& Relationship);
	void ResetAutomationProfile();
	void ResetAutomationRun();
#endif

private:
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void BindWorld(UWorld* World);
	void UnbindWorld(UWorld* World);
	void ReconcileInputBridge();
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleDialogueResult(const struct FPRDialogueResult& Result);
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandlePrimarySyncChanged(const struct FPRPrimaryCompanionSyncChangedEvent& Event);
	bool TryBeginFromDeath(const struct FPRCombatEvent& Event);
	bool BuildEligibility(FPRDivergenceEligibilityInput& OutInput, FPRCompanionRelationshipRecord& OutRelationship, FGameplayTag& OutPrimary) const;
	bool ApplyChoice(EPRDivergenceChoice Choice, FPRDivergenceResult& OutResult);
	bool ApplyRelationshipAndSave(FPRDivergenceResult& InOutResult);
	bool IsRelationshipStateDifferent(const FPRRelationshipState& Left, const FPRRelationshipState& Right) const;
	void HandleExpired();
	void BroadcastState();
	void PublishResult(const FPRDivergenceResult& Result);
	void ClearActive(bool bKeepRunConsumption);
	void LoadDefinition();
	double GetWorldTimeSeconds() const;

	TSoftObjectPtr<UPRDivergenceDataAsset> DefinitionAsset;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UPRCompanionSubsystem> CompanionSubsystem;
	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<UPRCompanionDialogueSubsystem> DialogueSubsystem;
	TWeakObjectPtr<UPRDivergenceComponent> InputBridge;
	TWeakObjectPtr<APRPlayerCharacter> BoundPlayerPawn;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle PrimarySyncHandle;
	FDelegateHandle DialogueResultHandle;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FTimerHandle InputReconcileTimer;
	FTimerHandle ExpiryTimer;
	FPRDivergenceRuntimeState RuntimeState;
	FPRDivergenceResult LastResult;
	FPRDivergenceStateChangedNative StateChanged;
	FPRDivergenceResultNative ResultPublished;
	bool bDefinitionReady = false;
	bool bRunProtectionConsumed = false;
	FGuid LastDialogueResultId;
	FName LastDialogueChoiceId;
	FGameplayTag LastDialogueCompanionId;

#if WITH_DEV_AUTOMATION_TESTS
	bool bUseAutomationProfile = false;
	FPRCompanionRelationshipRecord AutomationRelationship;
#endif
};
