// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorSubsystem.h"
#include "Core/PRDeveloperSettings.h"
#include "Director/PRDirectorResponseValidator.h"
#include "Director/PRDirectorRuleEffectTypes.h"
#include "Director/PRDirectorRuleDataAsset.h"
#include "Director/PRDirectorRuleRegistryDataAsset.h"
#include "Director/PRHttpDirectorProvider.h"
#include "Director/PRMockDirectorProvider.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "QTE/PRQTESubsystem.h"
#include "QTE/PRQTETypes.h"
#include "TimerManager.h"

void UPRDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MockProvider = MakeUnique<FPRMockDirectorProvider>();
	HttpProvider = MakeUnique<FPRHttpDirectorProvider>();
	RuleEffectExecutor = MakeUnique<FPRDirectorRuleEffectExecutor>();
	Registry = LoadObject<UPRDirectorRuleRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Director/DA_DirectorRuleRegistry.DA_DirectorRuleRegistry"));
	RebindRuntimeWorld();
}

void UPRDirectorSubsystem::Deinitialize()
{
	bShuttingDown = true;
	if (bHasActiveRequest && MockProvider) MockProvider->CancelRequest(ActiveRequest.RequestId);
	if (bHasActiveRequest && HttpProvider) HttpProvider->CancelRequest(ActiveRequest.RequestId);
	bHasActiveRequest = false;
	UnbindRuntimeWorld();
	AppliedRules.Empty();
	if (RuleEffectExecutor) RuleEffectExecutor->Reset();
	RuleEffectExecutor.Reset();
	MockProvider.Reset(); HttpProvider.Reset(); Registry = nullptr;
	Super::Deinitialize();
}

EPRDirectorRequestStatus UPRDirectorSubsystem::RequestEvaluation(FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	RebindRuntimeWorld();
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
bool UPRDirectorSubsystem::GetRuleRuntimeState(const FGameplayTag RuleId, FPRDirectorRuleRuntimeState& OutState) const
{
	OutState = FPRDirectorRuleRuntimeState();
	return RuleEffectExecutor && RuleEffectExecutor->GetState(RuleId, OutState);
}

void UPRDirectorSubsystem::GetRuleRuntimeStates(TArray<FPRDirectorRuleRuntimeState>& OutStates) const
{
	OutStates.Reset();
	if (RuleEffectExecutor) RuleEffectExecutor->GetStates(OutStates);
}
FPRDirectorEvaluationCompletedNative& UPRDirectorSubsystem::OnEvaluationCompleted() { return EvaluationCompleted; }
FPRDirectorAppliedRuleChangedNative& UPRDirectorSubsystem::OnAppliedRuleChanged() { return AppliedRuleChanged; }
FPRDirectorRuleRuntimeChangedNative& UPRDirectorSubsystem::OnRuleRuntimeChanged() { return RuleRuntimeChanged; }

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
	const FPRAppliedDirectorRuleHandle PreviousHandle = Index == INDEX_NONE ? FPRAppliedDirectorRuleHandle() : AppliedRules[Index];
	if (Index == INDEX_NONE) AppliedRules.Add(Handle); else AppliedRules[Index] = Handle;
	const UPRDirectorRuleDataAsset* Rule = Registry ? Registry->FindRule(Handle.RuleId) : nullptr;
	if (!Rule || !RuleEffectExecutor || !RuleEffectExecutor->Apply(Handle, Rule->DefaultVisibleReason, Rule->CounterDescription,
		GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0))
	{
		if (Index == INDEX_NONE) AppliedRules.RemoveAt(AppliedRules.Num() - 1); else AppliedRules[Index] = PreviousHandle;
		return EPRDirectorRuleOperationResult::Invalid;
	}
	BroadcastRuleRuntime(Handle.RuleId);
	if (Handle.RuleId.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"), false)))
	{
		ResourceEnergySpentSinceApply = 0.0f;
		EvaluateResourceBalance(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
	}
	EvaluateProfileDrivenRules(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
	AppliedRuleChanged.Broadcast(Result, Handle);
	return Result;
}

EPRDirectorRuleOperationResult UPRDirectorSubsystem::RemoveAppliedRule(const FPRAppliedDirectorRuleHandle& Handle)
{
	const int32 Index = AppliedRules.IndexOfByPredicate([&Handle](const FPRAppliedDirectorRuleHandle& Existing) { return Existing.HandleId == Handle.HandleId && Existing.RuleId == Handle.RuleId && Existing.Level == Handle.Level && Existing.ApplySequence == Handle.ApplySequence; });
	if (Index == INDEX_NONE) return EPRDirectorRuleOperationResult::NotFound;
	const FPRAppliedDirectorRuleHandle Removed = AppliedRules[Index];
	if (!RuleEffectExecutor || !RuleEffectExecutor->Remove(Removed, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0)) return EPRDirectorRuleOperationResult::Invalid;
	AppliedRules.RemoveAt(Index); AppliedRuleChanged.Broadcast(EPRDirectorRuleOperationResult::Removed, Removed);
	if (Removed.RuleId.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"), false)))
	{
		ResourceEnergySpentSinceApply = 0.0f;
		if (UWorld* CurrentWorld = GetWorld()) CurrentWorld->GetTimerManager().ClearTimer(ResourceBalanceHighEnergyTimer);
	}
	FPRDirectorRuleRuntimeState RemovedState;
	RemovedState.HandleId = Removed.HandleId;
	RemovedState.RuleId = Removed.RuleId;
	RemovedState.Level = Removed.Level;
	RemovedState.Status = EPRDirectorRuleRuntimeStatus::Inactive;
	RemovedState.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	RuleRuntimeChanged.Broadcast(RemovedState);
	return EPRDirectorRuleOperationResult::Removed;
}

#if !UE_BUILD_SHIPPING
EPRDirectorRuleOperationResult UPRDirectorSubsystem::ApplyWhitelistedRuleForDevelopment(const FGameplayTag RuleId, const int32 Level, FGuid& OutHandleId)
{
	OutHandleId.Invalidate();
	RebindRuntimeWorld();
	if (!Registry || !Registry->IsRegistryReady() || !FPRDirectorRuleEffectContract::IsRequiredRuleId(RuleId) || Level < 1 || Level > 3)
	{
		return EPRDirectorRuleOperationResult::Invalid;
	}
	const UPRDirectorRuleDataAsset* Rule = Registry->FindRule(RuleId);
	if (!Rule) return EPRDirectorRuleOperationResult::Invalid;
	FPRDirectorRequest Request;
	Request.RequestId = FGuid::NewGuid();
	for (const TSoftObjectPtr<UPRDirectorRuleDataAsset>& Reference : Registry->Rules)
	{
		if (const UPRDirectorRuleDataAsset* Loaded = Reference.LoadSynchronous()) Request.CandidateRuleIds.Add(Loaded->RuleId);
	}
	FPRDirectorResponse Response;
	Response.RequestId = Request.RequestId;
	Response.RuleId = RuleId;
	Response.Level = Level;
	Response.VisibleReason = Rule->DefaultVisibleReason.ToString();
	Response.ExpressionText = Rule->CounterDescription.ToString();
	FPRDirectorValidationResult Validation;
	if (!UPRDirectorResponseValidator::Validate(Request, Response, *Registry, TNumericLimits<double>::Max(), 0.0, Validation))
	{
		return EPRDirectorRuleOperationResult::Invalid;
	}
	const EPRDirectorRuleOperationResult Result = ApplyValidatedResponse(Validation.CanonicalResponse);
	if (Result == EPRDirectorRuleOperationResult::Applied || Result == EPRDirectorRuleOperationResult::AlreadyApplied || Result == EPRDirectorRuleOperationResult::Replaced)
	{
		if (const FPRAppliedDirectorRuleHandle* Handle = AppliedRules.FindByPredicate([RuleId](const FPRAppliedDirectorRuleHandle& Item) { return Item.RuleId == RuleId; })) OutHandleId = Handle->HandleId;
	}
	return Result;
}
#endif

void UPRDirectorSubsystem::RebindRuntimeWorld()
{
	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld || !CurrentWorld->IsGameWorld()) return;
	if (RuleEffectExecutor) RuleEffectExecutor->BindWorld(CurrentWorld);
	UPRCombatSubsystem* Combat = CurrentWorld->GetSubsystem<UPRCombatSubsystem>();
	if (BoundCombat.Get() != Combat)
	{
		if (UPRCombatSubsystem* Previous = BoundCombat.Get(); Previous && CombatEventHandle.IsValid()) Previous->OnCombatEvent().Remove(CombatEventHandle);
		CombatEventHandle.Reset();
		BoundCombat = Combat;
		if (Combat) CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRDirectorSubsystem::HandleCombatEvent);
	}
	UPRQTESubsystem* QTE = CurrentWorld->GetSubsystem<UPRQTESubsystem>();
	if (BoundQTE.Get() != QTE)
	{
		if (UPRQTESubsystem* Previous = BoundQTE.Get(); Previous && QTEResultHandle.IsValid()) Previous->OnQTEResult().Remove(QTEResultHandle);
		QTEResultHandle.Reset();
		BoundQTE = QTE;
		if (QTE) QTEResultHandle = QTE->OnQTEResult().AddUObject(this, &UPRDirectorSubsystem::HandleQTEResult);
	}
	APlayerController* Controller = CurrentWorld->GetFirstPlayerController();
	IAbilitySystemInterface* Interface = Controller ? Cast<IAbilitySystemInterface>(Controller->GetPawn()) : nullptr;
	UPRAbilitySystemComponent* ASC = Interface ? Cast<UPRAbilitySystemComponent>(Interface->GetAbilitySystemComponent()) : nullptr;
	if (BoundPlayerASC.Get() != ASC)
	{
		if (UPRAbilitySystemComponent* Previous = BoundPlayerASC.Get(); Previous)
		{
			if (AbilityLifecycleHandle.IsValid()) Previous->OnAbilityLifecycleEvent().Remove(AbilityLifecycleHandle);
			if (EnergyAttributeHandle.IsValid()) Previous->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetEnergyAttribute()).Remove(EnergyAttributeHandle);
			if (MaxEnergyAttributeHandle.IsValid()) Previous->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetMaxEnergyAttribute()).Remove(MaxEnergyAttributeHandle);
		}
		AbilityLifecycleHandle.Reset();
		EnergyAttributeHandle.Reset();
		MaxEnergyAttributeHandle.Reset();
		BoundPlayerASC = ASC;
		if (ASC)
		{
			AbilityLifecycleHandle = ASC->OnAbilityLifecycleEvent().AddUObject(this, &UPRDirectorSubsystem::HandleAbilityLifecycle);
			EnergyAttributeHandle = ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetEnergyAttribute()).AddUObject(this, &UPRDirectorSubsystem::HandleEnergyChanged);
			MaxEnergyAttributeHandle = ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetMaxEnergyAttribute()).AddUObject(this, &UPRDirectorSubsystem::HandleMaxEnergyChanged);
		}
	}
	if (RuleEffectExecutor) RuleEffectExecutor->RebindWorldEffects();
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	if (BoundCompanions.Get() != Companions)
	{
		if (UPRCompanionSubsystem* Previous = BoundCompanions.Get(); Previous && RelationshipChangedHandle.IsValid()) Previous->OnRelationshipChanged().Remove(RelationshipChangedHandle);
		RelationshipChangedHandle.Reset();
		BoundCompanions = Companions;
		if (Companions) RelationshipChangedHandle = Companions->OnRelationshipChanged().AddUObject(this, &UPRDirectorSubsystem::HandleRelationshipChanged);
	}
	EvaluateProfileDrivenRules(CurrentWorld->GetTimeSeconds());
}

void UPRDirectorSubsystem::UnbindRuntimeWorld()
{
	if (UPRCombatSubsystem* Combat = BoundCombat.Get(); Combat && CombatEventHandle.IsValid()) Combat->OnCombatEvent().Remove(CombatEventHandle);
	if (UPRQTESubsystem* QTE = BoundQTE.Get(); QTE && QTEResultHandle.IsValid()) QTE->OnQTEResult().Remove(QTEResultHandle);
	if (UPRAbilitySystemComponent* ASC = BoundPlayerASC.Get(); ASC)
	{
		if (AbilityLifecycleHandle.IsValid()) ASC->OnAbilityLifecycleEvent().Remove(AbilityLifecycleHandle);
		if (EnergyAttributeHandle.IsValid()) ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetEnergyAttribute()).Remove(EnergyAttributeHandle);
		if (MaxEnergyAttributeHandle.IsValid()) ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetMaxEnergyAttribute()).Remove(MaxEnergyAttributeHandle);
	}
	if (UWorld* CurrentWorld = GetWorld()) CurrentWorld->GetTimerManager().ClearTimer(ResourceBalanceHighEnergyTimer);
	if (UPRCompanionSubsystem* Companions = BoundCompanions.Get(); Companions && RelationshipChangedHandle.IsValid()) Companions->OnRelationshipChanged().Remove(RelationshipChangedHandle);
	CombatEventHandle.Reset(); QTEResultHandle.Reset(); AbilityLifecycleHandle.Reset(); EnergyAttributeHandle.Reset(); MaxEnergyAttributeHandle.Reset();
	RelationshipChangedHandle.Reset();
	BoundCombat.Reset(); BoundQTE.Reset(); BoundPlayerASC.Reset(); BoundCompanions.Reset();
}

void UPRDirectorSubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (!RuleEffectExecutor || !Event.EventId.IsValid()) return;
	RuleEffectExecutor->RebindWorldEffects();
	const auto Rule = [](const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name), false); };
	if (Event.bFatal && Event.TargetId == TEXT("Player"))
	{
		RuleEffectExecutor->SetRuleStatus(Rule(TEXT("Rule.DeleteEcho")), EPRDirectorRuleRuntimeStatus::Degraded, Event.WorldTimeSeconds);
		BroadcastRuleRuntime(Rule(TEXT("Rule.DeleteEcho")));
	}
	if (Event.TargetId == TEXT("Player") && Event.MaxHealth > 0.0f)
	{
		const EPRDirectorRuleRuntimeStatus Status = Event.RemainingHealth / Event.MaxHealth <= 0.35f
			? EPRDirectorRuleRuntimeStatus::Active : EPRDirectorRuleRuntimeStatus::Suspended;
		RuleEffectExecutor->SetRuleStatus(Rule(TEXT("Rule.SurvivalProtocol")), Status, Event.WorldTimeSeconds);
		BroadcastRuleRuntime(Rule(TEXT("Rule.SurvivalProtocol")));
	}
	if (AActor* Instigator = Event.Instigator.Get(); AActor* Target = Event.Target.Get())
	{
		const FVector Delta = Target->GetActorLocation() - Instigator->GetActorLocation();
		if (FVector(Delta.X, 0.0f, Delta.Z).Size() > 650.0f && Event.HealthDamage > 0.0f)
		{
			RuleEffectExecutor->SetRuleStatus(Rule(TEXT("Rule.DistanceCorrection")), EPRDirectorRuleRuntimeStatus::Active, Event.WorldTimeSeconds);
			++LongRangeSafeActionCount;
			if (LongRangeSafeActionCount >= 3) RuleEffectExecutor->SetRuleStatus(Rule(TEXT("Rule.OptimalPath")), EPRDirectorRuleRuntimeStatus::Degraded, Event.WorldTimeSeconds);
			BroadcastRuleRuntime(Rule(TEXT("Rule.DistanceCorrection")));
			BroadcastRuleRuntime(Rule(TEXT("Rule.OptimalPath")));
		}
	}
	if (Event.bFatal && Event.SourceId == TEXT("Player"))
	{
		RuleEffectExecutor->AdvanceCounter(FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"), false), 1, Event.WorldTimeSeconds);
		RuleEffectExecutor->AdvanceCounter(FGameplayTag::RequestGameplayTag(TEXT("Rule.OptimalPath"), false), 1, Event.WorldTimeSeconds);
		RuleEffectExecutor->AdvanceCounter(FGameplayTag::RequestGameplayTag(TEXT("Rule.RiskReward"), false), 1, Event.WorldTimeSeconds);
		BroadcastRuleRuntime(FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"), false));
		BroadcastRuleRuntime(FGameplayTag::RequestGameplayTag(TEXT("Rule.OptimalPath"), false));
		BroadcastRuleRuntime(FGameplayTag::RequestGameplayTag(TEXT("Rule.RiskReward"), false));
	}
	if (Event.TargetId == TEXT("Player") && Event.bFatal)
	{
		RuleEffectExecutor->SetRuleStatus(Rule(TEXT("Rule.RiskReward")), EPRDirectorRuleRuntimeStatus::Suspended, Event.WorldTimeSeconds);
		BroadcastRuleRuntime(Rule(TEXT("Rule.RiskReward")));
	}
}

void UPRDirectorSubsystem::HandleQTEResult(const FPRQTEResult& Result)
{
	if (!RuleEffectExecutor || !Result.ResultId.IsValid()) return;
	if (Result.ResultTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Success"), false)))
	{
		for (const FPRAppliedDirectorRuleHandle& Handle : AppliedRules)
		{
			RuleEffectExecutor->AdvanceCounter(Handle.RuleId, 1, Result.WorldTimeSeconds);
			BroadcastRuleRuntime(Handle.RuleId);
		}
	}
	else if (Result.ResultTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Failure"), false))
		|| Result.ResultTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Rejected"), false))
		|| Result.ResultTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Timeout"), false)))
	{
		const FGameplayTag Cooperation = FGameplayTag::RequestGameplayTag(TEXT("Rule.CooperationAudit"), false);
		RuleEffectExecutor->SetRuleStatus(Cooperation, EPRDirectorRuleRuntimeStatus::Active, Result.WorldTimeSeconds);
		BroadcastRuleRuntime(Cooperation);
	}
}

void UPRDirectorSubsystem::HandleAbilityLifecycle(const FPRAbilityLifecycleEvent& Event)
{
	if (!RuleEffectExecutor || Event.EventType != EPRAbilityLifecycleEventType::Committed) return;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const FGameplayTag SkillRoot = FGameplayTag::RequestGameplayTag(TEXT("Skill"), false);
	const FGameplayTag SkillTag = Event.AbilityState.AbilityTag;
	if (!SkillTag.IsValid() || !SkillTag.MatchesTag(SkillRoot)) return;
	const FGameplayTag Repetition = FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"), false);
	const FGameplayTag Prediction = FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false);
	RepetitionCommitStreak = LastCommittedSkillTag == SkillTag ? RepetitionCommitStreak + 1 : 1;
	LastCommittedSkillTag = SkillTag;
	PredictionDistinctSkillTags.Add(SkillTag);
	if (RepetitionCommitStreak >= 3) RuleEffectExecutor->SetRuleStatus(Repetition, EPRDirectorRuleRuntimeStatus::Active, Now);
	FPRPlayerProfileSnapshot Snapshot;
	FGameplayTag MostUsedSkill;
	int32 HighestCommitCount = 0;
	if (UPRPlayerProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRPlayerProfileSubsystem>() : nullptr; Profile && Profile->GetSnapshot(Snapshot))
	{
		for (const FPRPlayerProfileSkillMetric& Metric : Snapshot.SkillMetrics)
		{
			if (Metric.SkillTag.IsValid() && (Metric.CommitCount > HighestCommitCount || (Metric.CommitCount == HighestCommitCount && Metric.SkillTag.ToString() < MostUsedSkill.ToString())))
			{
				MostUsedSkill = Metric.SkillTag;
				HighestCommitCount = Metric.CommitCount;
			}
		}
	}
	if (MostUsedSkill.IsValid() && MostUsedSkill == SkillTag && RepetitionCommitStreak >= 2) RuleEffectExecutor->SetRuleStatus(Prediction, EPRDirectorRuleRuntimeStatus::Active, Now);
	if (PredictionDistinctSkillTags.Num() >= 3) RuleEffectExecutor->AdvanceCounter(Prediction, 1, Now);
	BroadcastRuleRuntime(Repetition);
	BroadcastRuleRuntime(Prediction);
}

void UPRDirectorSubsystem::HandleRelationshipChanged(const FPRRelationshipChangedEvent& Event)
{
	if (!RuleEffectExecutor) return;
	const FGameplayTag Emotional = FGameplayTag::RequestGameplayTag(TEXT("Rule.EmotionalInterference"), false);
	const EPRDirectorRuleRuntimeStatus Status = Event.CurrentState.Trust < 40 || Event.CurrentState.Affection < 40
		? EPRDirectorRuleRuntimeStatus::Active : EPRDirectorRuleRuntimeStatus::Suspended;
	RuleEffectExecutor->SetRuleStatus(Emotional, Status, Event.WorldTimeSeconds);
	BroadcastRuleRuntime(Emotional);
}

void UPRDirectorSubsystem::HandleEnergyChanged(const FOnAttributeChangeData& Data)
{
	if (Data.OldValue > Data.NewValue && FMath::IsFinite(Data.OldValue) && FMath::IsFinite(Data.NewValue))
	{
		ResourceEnergySpentSinceApply += Data.OldValue - Data.NewValue;
	}
	EvaluateResourceBalance(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
}

void UPRDirectorSubsystem::HandleMaxEnergyChanged(const FOnAttributeChangeData& Data)
{
	EvaluateResourceBalance(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
}

void UPRDirectorSubsystem::EvaluateResourceBalance(const double WorldTimeSeconds)
{
	if (!RuleEffectExecutor || !FMath::IsFinite(WorldTimeSeconds)) return;
	UPRAbilitySystemComponent* ASC = BoundPlayerASC.Get();
	if (!ASC) return;
	const FGameplayTag Resource = FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"), false);
	FPRDirectorRuleRuntimeState State;
	if (!RuleEffectExecutor->GetState(Resource, State)) return;
	if (State.Status == EPRDirectorRuleRuntimeStatus::Countered || State.Status == EPRDirectorRuleRuntimeStatus::Inactive) return;
	const float MaxEnergy = ASC->GetNumericAttribute(UPRAttributeSet::GetMaxEnergyAttribute());
	const float Energy = ASC->GetNumericAttribute(UPRAttributeSet::GetEnergyAttribute());
	if (MaxEnergy <= KINDA_SMALL_NUMBER || !FMath::IsFinite(Energy) || !FMath::IsFinite(MaxEnergy)) return;
	const float EnergyRatio = Energy / MaxEnergy;
	if (ResourceEnergySpentSinceApply >= (State.Level == 1 ? 30.0f : State.Level == 2 ? 40.0f : 50.0f))
	{
		RuleEffectExecutor->AdvanceCounter(Resource, State.CounterTarget, WorldTimeSeconds);
		BroadcastRuleRuntime(Resource);
		return;
	}
	if (EnergyRatio < 0.50f)
	{
		if (UWorld* CurrentWorld = GetWorld()) CurrentWorld->GetTimerManager().ClearTimer(ResourceBalanceHighEnergyTimer);
		RuleEffectExecutor->SetRuleStatus(Resource, EPRDirectorRuleRuntimeStatus::Suspended, WorldTimeSeconds);
		BroadcastRuleRuntime(Resource);
		return;
	}
	if (EnergyRatio >= 0.80f && State.Status == EPRDirectorRuleRuntimeStatus::Suspended)
	{
		if (UWorld* CurrentWorld = GetWorld(); CurrentWorld && !CurrentWorld->GetTimerManager().IsTimerActive(ResourceBalanceHighEnergyTimer))
		{
			CurrentWorld->GetTimerManager().SetTimer(ResourceBalanceHighEnergyTimer, this, &UPRDirectorSubsystem::ActivateResourceBalanceIfStillHigh, 5.0f, false);
		}
	}
}

void UPRDirectorSubsystem::ActivateResourceBalanceIfStillHigh()
{
	ResourceBalanceHighEnergyTimer.Invalidate();
	if (!RuleEffectExecutor) return;
	UPRAbilitySystemComponent* ASC = BoundPlayerASC.Get();
	if (!ASC) return;
	const float MaxEnergy = ASC->GetNumericAttribute(UPRAttributeSet::GetMaxEnergyAttribute());
	const float Energy = ASC->GetNumericAttribute(UPRAttributeSet::GetEnergyAttribute());
	if (MaxEnergy <= KINDA_SMALL_NUMBER || Energy / MaxEnergy < 0.80f) return;
	const FGameplayTag Resource = FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"), false);
	RuleEffectExecutor->SetRuleStatus(Resource, EPRDirectorRuleRuntimeStatus::Degraded, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
	BroadcastRuleRuntime(Resource);
}

void UPRDirectorSubsystem::EvaluateProfileDrivenRules(const double WorldTimeSeconds)
{
	if (!RuleEffectExecutor) return;
	UPRPlayerProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRPlayerProfileSubsystem>() : nullptr;
	FPRPlayerProfileSnapshot Snapshot;
	if (!Profile || !Profile->GetSnapshot(Snapshot)) return;
	const FGameplayTag Emotional = FGameplayTag::RequestGameplayTag(TEXT("Rule.EmotionalInterference"), false);
	bool bLowRelationship = false;
	for (const FPRCompanionRelationshipRecord& Record : Snapshot.Relationships)
	{
		if (Record.CompanionId == Snapshot.PrimaryCompanionId && (Record.State.Trust < 40 || Record.State.Affection < 40)) { bLowRelationship = true; break; }
	}
	RuleEffectExecutor->SetRuleStatus(Emotional, bLowRelationship ? EPRDirectorRuleRuntimeStatus::Active : EPRDirectorRuleRuntimeStatus::Suspended, WorldTimeSeconds);
	BroadcastRuleRuntime(Emotional);
	EvaluateResourceBalance(WorldTimeSeconds);
}

void UPRDirectorSubsystem::BroadcastRuleRuntime(const FGameplayTag& RuleId)
{
	FPRDirectorRuleRuntimeState State;
	if (RuleEffectExecutor && RuleEffectExecutor->GetState(RuleId, State)) RuleRuntimeChanged.Broadcast(State);
}
