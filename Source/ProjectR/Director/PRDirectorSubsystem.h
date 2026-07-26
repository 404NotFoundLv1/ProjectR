// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorProvider.h"
#include "Director/PRDirectorTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRDirectorSubsystem.generated.h"

class UPRDirectorRuleRegistryDataAsset;
class UPRDirectorRuleDataAsset;

/** Owns provider selection, validation, and non-gameplay applied-rule handles. */
UCLASS()
class PROJECTR_API UPRDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	EPRDirectorRequestStatus RequestEvaluation(FGuid& OutRequestId);
	bool GetAppliedRules(TArray<FPRAppliedDirectorRuleHandle>& OutRules) const;
	EPRDirectorRuleOperationResult RemoveAppliedRule(const FPRAppliedDirectorRuleHandle& Handle);
	FPRDirectorEvaluationCompletedNative& OnEvaluationCompleted();
	FPRDirectorAppliedRuleChangedNative& OnAppliedRuleChanged();
private:
	void CompleteProviderResponse(const FPRDirectorResponse& Response, bool bFallback);
	EPRDirectorRuleOperationResult ApplyValidatedResponse(const FPRDirectorResponse& Response);
	TObjectPtr<UPRDirectorRuleRegistryDataAsset> Registry = nullptr;
	TUniquePtr<IPRDirectorProvider> MockProvider;
	TUniquePtr<IPRDirectorProvider> HttpProvider;
	TArray<FPRAppliedDirectorRuleHandle> AppliedRules;
	FPRDirectorRequest ActiveRequest;
	FPRDirectorEvaluationCompletedNative EvaluationCompleted;
	FPRDirectorAppliedRuleChangedNative AppliedRuleChanged;
	int64 RequestSequence = 0;
	int64 ApplySequence = 0;
	bool bHasActiveRequest = false;
	bool bShuttingDown = false;
};
