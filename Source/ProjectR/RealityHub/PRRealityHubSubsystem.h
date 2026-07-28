// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RealityHub/PRRealityHubTypes.h"
#include "Roguelike/Account/PRAccountRuntimeTypes.h"
#include "Roguelike/Progression/PRProgressionTypes.h"
#include "Save/PRAccountSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRRealityHubSubsystem.generated.h"

/** Sole owner of transient Hub requests, state snapshots, fixed training travel, and cleanup. */
UCLASS()
class PROJECTR_API UPRRealityHubSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool GetSnapshot(FPRRealityHubSnapshot& OutSnapshot) const;
	FPRRealityHubForecast GetForecast() const;
	void GetGraveyardSnapshot(TArray<struct FPRAccountRecord>& OutRecords) const;
	bool GetProgressionSnapshot(struct FPRProgressionSnapshot& OutSnapshot) const;

	EPRRealityHubOperationResult RequestLoadDefaultProfile();
	EPRRealityHubOperationResult RequestCreateDefaultProfile();
	EPRRealityHubOperationResult RequestCreateFixedIdentityAccount(EPRRealityHubIdentity Identity);
	EPRRealityHubOperationResult RequestStartRun();
	EPRRealityHubOperationResult RetryPendingOperation();
	EPRRealityHubOperationResult RequestTrainingTravel();
	EPRRealityHubOperationResult RequestReturnToRealityHub();

	static FPrimaryAssetId GetFixedIdentityId(EPRRealityHubIdentity Identity);
	static int32 MakeFixedRunSeed(const FGuid& AccountId);
	DECLARE_MULTICAST_DELEGATE_OneParam(FPRRealityHubStateChangedNative, const FPRRealityHubSnapshot&);
	DECLARE_MULTICAST_DELEGATE_OneParam(FPRRealityHubOperationNative, const FPRRealityHubOperationEvent&);
	FPRRealityHubStateChangedNative& OnStateChanged();
	FPRRealityHubOperationNative& OnOperation();

private:
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleAccountOperation(const struct FPRAccountOperationEvent& Event);
	void HandlePostLoadMap(class UWorld* LoadedWorld);
	void ClearTrainingReturnWidget();
	void RebuildSnapshot();
	void PublishOperation(EPRRealityHubTerminal Terminal, EPRRealityHubOperationResult Result, const FText& Message);
	EPRRealityHubOperationResult FromAccountResult(EPRAccountOperationResult Result) const;
	const class UPRRealityHubTerminalRegistryDataAsset* LoadRegistry() const;

	TWeakObjectPtr<class UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<class UPRRunStateSubsystem> RunStateSubsystem;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle AccountOperationHandle;
	FDelegateHandle PostLoadMapHandle;
	TWeakObjectPtr<class UPRRealityHubTrainingReturnWidget> TrainingReturnWidget;
	bool bTrainingTravelActive = false;
	FPRRealityHubSnapshot Snapshot;
	int64 OperationSequence = 0;
	EPRRealityHubTerminal PendingTerminal = EPRRealityHubTerminal::None;
	FPRRealityHubStateChangedNative StateChanged;
	FPRRealityHubOperationNative Operation;
};
