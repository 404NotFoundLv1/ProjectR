// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Companions/PRCompanionTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRCompanionRuntimeSubsystem.generated.h"

class APRCompanionPawn;
class UPRCompanionRuntimeDataAsset;

/** World-owned runtime seam for the single active Companion Pawn and support facts. */
UCLASS()
class PROJECTR_API UPRCompanionRuntimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	static float CalculateEffectiveSupportInterval(const UPRCompanionRuntimeDataAsset& RuntimeData, int32 Overload);
	/** Narrow, source-keyed session policy seam. Director is intentionally not named here. */
	bool SetSupportPolicy(FName SourceId, float IntervalMultiplier, int32 SuppressionStride);
	bool ClearSupportPolicy(FName SourceId);
	bool GetSupportPolicy(float& OutIntervalMultiplier, int32& OutSuppressionStride) const;
	FPRCompanionSupportEventNative& OnSupportEvent();
	APRCompanionPawn* GetActiveCompanionPawn() const;

private:
	friend class UPRCompanionComponent;
	void HandlePrimarySyncChanged(const FPRPrimaryCompanionSyncChangedEvent& Event);
	void ReconcilePrimaryPawn();
	void RetryReconcilePrimaryPawn();
	void DestroyActiveCompanionPawn();
	void PublishSupportEvent(FPRCompanionSupportEvent&& Event);

	FPRCompanionSupportEventNative SupportEvent;
	TWeakObjectPtr<APRCompanionPawn> ActiveCompanionPawn;
	TWeakObjectPtr<class UPRCompanionSubsystem> CompanionSubsystem;
	FDelegateHandle PrimarySyncChangedHandle;
	FTimerHandle ReconcileRetryTimer;
	TMap<FName, float> SupportIntervalMultipliers;
	TMap<FName, int32> SupportSuppressionStrides;
};
