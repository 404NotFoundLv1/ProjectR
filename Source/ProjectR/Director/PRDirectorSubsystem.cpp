// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorSubsystem.h"
#include "Core/PRDeveloperSettings.h"
#include "Director/PRDirectorResponseValidator.h"
#include "Director/PRDirectorRuleRegistryDataAsset.h"
#include "Director/PRHttpDirectorProvider.h"
#include "Director/PRMockDirectorProvider.h"
#include "Director/PRPlayerProfileSubsystem.h"

void UPRDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MockProvider = MakeUnique<FPRMockDirectorProvider>();
	HttpProvider = MakeUnique<FPRHttpDirectorProvider>();
	Registry = LoadObject<UPRDirectorRuleRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Director/DA_DirectorRuleRegistry.DA_DirectorRuleRegistry"));
}

void UPRDirectorSubsystem::Deinitialize()
{
	bShuttingDown = true;
	if (bHasActiveRequest && MockProvider) MockProvider->CancelRequest(ActiveRequest.RequestId);
	if (bHasActiveRequest && HttpProvider) HttpProvider->CancelRequest(ActiveRequest.RequestId);
	bHasActiveRequest = false;
	AppliedRules.Empty();
	MockProvider.Reset(); HttpProvider.Reset(); Registry = nullptr;
	Super::Deinitialize();
}

EPRDirectorRequestStatus UPRDirectorSubsystem::RequestEvaluation(FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (bShuttingDown) return EPRDirectorRequestStatus::ShuttingDown;
	if (bHasActiveRequest) return EPRDirectorRequestStatus::Busy;
	if (!Registry || !Registry->IsRegistryReady()) return EPRDirectorRequestStatus::Invalid;
	UPRPlayerProfileSubsystem* Profile = GetGameInstance()->GetSubsystem<UPRPlayerProfileSubsystem>();
	if (!Profile || !Profile->GetSnapshot(ActiveRequest.Profile)) return EPRDirectorRequestStatus::Invalid;
	ActiveRequest = FPRDirectorRequest();
	Profile->GetSnapshot(ActiveRequest.Profile);
	ActiveRequest.RequestId = FGuid::NewGuid();
	ActiveRequest.RequestSequence = ++RequestSequence;
	for (const TSoftObjectPtr<UPRDirectorRuleDataAsset>& Rule : Registry->Rules) if (const UPRDirectorRuleDataAsset* Loaded = Rule.LoadSynchronous()) ActiveRequest.CandidateRuleIds.Add(Loaded->RuleId);
	bHasActiveRequest = true; OutRequestId = ActiveRequest.RequestId;
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings && Settings->bUseMockDirector)
	{
		MockProvider->RequestRule(ActiveRequest, FPRDirectorProviderCompletion::CreateUObject(this, &UPRDirectorSubsystem::CompleteProviderResponse, false));
		return EPRDirectorRequestStatus::Started;
	}
	if (!HttpProvider || !HttpProvider->IsAvailable())
	{
		MockProvider->RequestRule(ActiveRequest, FPRDirectorProviderCompletion::CreateUObject(this, &UPRDirectorSubsystem::CompleteProviderResponse, true));
		return EPRDirectorRequestStatus::ProviderUnavailable;
	}
	HttpProvider->RequestRule(ActiveRequest, FPRDirectorProviderCompletion::CreateUObject(this, &UPRDirectorSubsystem::CompleteProviderResponse, false));
	return EPRDirectorRequestStatus::Started;
}

bool UPRDirectorSubsystem::GetAppliedRules(TArray<FPRAppliedDirectorRuleHandle>& OutRules) const { OutRules = AppliedRules; return true; }
FPRDirectorEvaluationCompletedNative& UPRDirectorSubsystem::OnEvaluationCompleted() { return EvaluationCompleted; }
FPRDirectorAppliedRuleChangedNative& UPRDirectorSubsystem::OnAppliedRuleChanged() { return AppliedRuleChanged; }

void UPRDirectorSubsystem::CompleteProviderResponse(const FPRDirectorResponse& Response, const bool bFallback)
{
	if (bShuttingDown || !bHasActiveRequest || Response.RequestId != ActiveRequest.RequestId) return;
	FPRDirectorValidationResult Validation;
	const bool bValid = UPRDirectorResponseValidator::Validate(ActiveRequest, Response, *Registry, TNumericLimits<double>::Max(), 0.0, Validation);
	bHasActiveRequest = false;
	const EPRDirectorRuleOperationResult Operation = bValid ? ApplyValidatedResponse(Validation.CanonicalResponse) : EPRDirectorRuleOperationResult::Invalid;
	EvaluationCompleted.Broadcast(Response.RequestId, bValid ? (bFallback ? EPRDirectorEvaluationResult::FallbackApplied : EPRDirectorEvaluationResult::Applied) : Validation.Result, Operation, Response);
}

EPRDirectorRuleOperationResult UPRDirectorSubsystem::ApplyValidatedResponse(const FPRDirectorResponse& Response)
{
	if (!Response.RuleId.IsValid()) return EPRDirectorRuleOperationResult::Invalid;
	const int32 Index = AppliedRules.IndexOfByPredicate([&Response](const FPRAppliedDirectorRuleHandle& Handle) { return Handle.RuleId == Response.RuleId; });
	if (Index != INDEX_NONE && AppliedRules[Index].Level == Response.Level && AppliedRules[Index].Parameters == Response.Parameters) return EPRDirectorRuleOperationResult::AlreadyApplied;
	FPRAppliedDirectorRuleHandle Handle; Handle.HandleId = FGuid::NewGuid(); Handle.RuleId = Response.RuleId; Handle.Level = Response.Level; Handle.Parameters = Response.Parameters; Handle.ApplySequence = ++ApplySequence;
	const EPRDirectorRuleOperationResult Result = Index == INDEX_NONE ? EPRDirectorRuleOperationResult::Applied : EPRDirectorRuleOperationResult::Replaced;
	if (Index == INDEX_NONE) AppliedRules.Add(Handle); else AppliedRules[Index] = Handle;
	AppliedRuleChanged.Broadcast(Result, Handle);
	return Result;
}

EPRDirectorRuleOperationResult UPRDirectorSubsystem::RemoveAppliedRule(const FPRAppliedDirectorRuleHandle& Handle)
{
	const int32 Index = AppliedRules.IndexOfByPredicate([&Handle](const FPRAppliedDirectorRuleHandle& Existing) { return Existing.HandleId == Handle.HandleId && Existing.RuleId == Handle.RuleId && Existing.Level == Handle.Level && Existing.ApplySequence == Handle.ApplySequence; });
	if (Index == INDEX_NONE) return EPRDirectorRuleOperationResult::NotFound;
	const FPRAppliedDirectorRuleHandle Removed = AppliedRules[Index]; AppliedRules.RemoveAt(Index); AppliedRuleChanged.Broadcast(EPRDirectorRuleOperationResult::Removed, Removed);
	return EPRDirectorRuleOperationResult::Removed;
}
