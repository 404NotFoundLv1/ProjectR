// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "TripleResonance/PRTripleResonanceTypes.h"
#include "Save/PRTripleResonanceSaveTypes.h"
#include "GameplayAbilitySpec.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"

#include "PRTripleResonanceSubsystem.generated.h"

/** Sole owner of the frozen v0.7.2 Triple Resonance eligibility and lifecycle state. */
UCLASS()
class PROJECTR_API UPRTripleResonanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool GetSnapshot(FPRTripleResonanceSnapshot& OutSnapshot) const;
	bool GetLatestResult(FPRTripleResonanceExecutionResult& OutResult) const;
	bool GetLegacySnapshot(FPRTripleResonanceLegacySnapshot& OutSnapshot) const;
	EPRTripleResonanceOperationResult RetryPendingPersistence();
	FPRTripleResonanceStateChangedNative& OnStateChanged() { return StateChanged; }
	FPRTripleResonanceResultNative& OnResolved() { return Resolved; }
	FPRTripleResonanceOperationNative& OnOperation() { return Operation; }

private:
	void HandleChapterStateChanged(const struct FPRChapterSnapshot& InSnapshot);
	void HandleRunStateChanged(const struct FPRRunRuntimeState& InState);
	void HandleProgressionRunSnapshotChanged(const struct FPRProgressionRunSnapshotChangedEvent& InEvent);
	void HandleRelationshipChanged(const struct FPRRelationshipChangedEvent& InEvent);
	void HandleSemanticInput(struct FGameplayTag InputTag, double InputTimeSeconds);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void HandlePostWorldInitialization(class UWorld* World, const struct UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(class UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void HandleAccountDeleted(const struct FPRAccountDeletedEvent& Event);
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	bool BeginLegacyPersistence();
	bool HandleDeferredPersistence(float DeltaSeconds);
	void RefreshEligibility();
	void BindQTEBridge();
	bool StartExternalStep(EPRTripleResonanceStep Step);
	void FailSequence(FName Reason);
	bool CanExecuteGrantedAbility(const class AActor* Avatar) const;
	bool ExecuteGrantedAbility(class AActor* Avatar);
	bool GrantAndActivateTransientAbility();
	class APRHeadmindProjectionBoss* ResolveFrozenHeadmind() const;
	void PublishState();
	void PublishOperation(EPRTripleResonanceOperationResult Result, FName Reason);
	void EnsurePresentation();
	void ClearPresentation();
	void ClearRuntimeState();

	TWeakObjectPtr<class UPRChapterSubsystem> ChapterSubsystem;
	TWeakObjectPtr<class UPRRunStateSubsystem> RunStateSubsystem;
	TWeakObjectPtr<class UPRProgressionSubsystem> ProgressionSubsystem;
	TWeakObjectPtr<class UPRCompanionSubsystem> CompanionSubsystem;
	TWeakObjectPtr<class UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<class UPRQTESubsystem> QTESubsystem;
	FDelegateHandle ChapterStateHandle;
	FDelegateHandle RunStateHandle;
	FDelegateHandle ProgressionRunSnapshotHandle;
	FDelegateHandle RelationshipHandle;
	FDelegateHandle QTESemanticInputHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle AccountDeletedHandle;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FTSTicker::FDelegateHandle DeferredPersistenceHandle;
	FPRTripleResonanceSnapshot Snapshot;
	FPRTripleResonanceExecutionResult LatestResult;
	FPRTripleResonanceLegacySnapshot LegacySnapshot;
	bool bHasLatestResult = false;
	bool bHasTriggered = false;
	FGuid ActiveQTERequestId;
	struct FGameplayAbilitySpecHandle TransientAbilityHandle;
	FPRTripleResonancePersistenceData PendingExpectedPersistence;
	FPRTripleResonancePersistenceData PendingTargetPersistence;
	FGuid PendingSaveRequestId;
	bool bHasFrozenPersistence = false;
	bool bHighRiskCandidate = false;
	TWeakObjectPtr<class UPRTripleResonanceOverlayWidget> OverlayWidget;
	TWeakObjectPtr<class UPRTripleResonanceLegacyWidget> LegacyWidget;
	FPRTripleResonanceStateChangedNative StateChanged;
	FPRTripleResonanceResultNative Resolved;
	FPRTripleResonanceOperationNative Operation;

	friend class UPRGA_TripleResonance;
};
