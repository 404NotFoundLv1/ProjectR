// Copyright Epic Games, Inc. All Rights Reserved.

#include "QTE/PRQTESubsystem.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Characters/PRPlayerCharacter.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/Bosses/PRBossAuditor.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "QTE/PRQTEComponent.h"
#include "QTE/PRQTEDataAsset.h"
#include "QTE/PRQTERegistryDataAsset.h"
#include "Save/PRSaveSubsystem.h"

namespace PRQTERegistry
{
const FSoftObjectPath RegistryPath(TEXT("/Game/ProjectR/QTE/DA_QTERegistry.DA_QTERegistry"));
constexpr float InputReconcileIntervalSeconds = 0.25f;
constexpr int32 MaximumRecentCombatEvents = 64;
constexpr int32 MaximumRecentSupportEvents = 16;
constexpr int32 MaximumRecentRequests = 128;
constexpr float RecentEventWindowSeconds = 2.0f;
constexpr float RecentRequestWindowSeconds = 30.0f;
constexpr float PlayerLowHealthFraction = 0.30f;
constexpr float NormalEnemyLowHealthFraction = 0.20f;
constexpr float NormalEnemyHealthFloorFraction = 0.20f;
constexpr float EliteEnemyHealthFloorFraction = 0.35f;
constexpr float BossHealthFloor = 1.0f;
const FSoftObjectPath AxiomShieldEffectPath(TEXT("/Game/ProjectR/Companions/Effects/GE_Companion_Axiom_Shield.GE_Companion_Axiom_Shield_C"));
const FSoftObjectPath StunnedEffectPath(TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C"));
const FSoftObjectPath InvulnerableEffectPath(TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Invulnerable.GE_State_Invulnerable_C"));
}

void UPRQTESubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	CombatSubsystem = Combat;
	if (Combat)
	{
		CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRQTESubsystem::HandleCombatEvent);
	}
	UPRCompanionRuntimeSubsystem* Runtime = GetWorld() ? GetWorld()->GetSubsystem<UPRCompanionRuntimeSubsystem>() : nullptr;
	CompanionRuntimeSubsystem = Runtime;
	if (Runtime)
	{
		SupportEventHandle = Runtime->OnSupportEvent().AddUObject(this, &UPRQTESubsystem::HandleSupportEvent);
	}
	LoadFixedRegistry();
	ReconcileInputBridge();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(InputReconcileTimer, this, &UPRQTESubsystem::ReconcileInputBridge,
			PRQTERegistry::InputReconcileIntervalSeconds, true);
	}
}

void UPRQTESubsystem::Deinitialize()
{
	CancelActiveQTE();
	ClearRuntimeEffectTimers();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputReconcileTimer);
		World->GetTimerManager().ClearTimer(CooldownTimer);
	}
	if (UPRCombatSubsystem* Combat = CombatSubsystem.Get(); Combat && CombatEventHandle.IsValid())
	{
		Combat->OnCombatEvent().Remove(CombatEventHandle);
	}
	if (UPRCompanionRuntimeSubsystem* Runtime = CompanionRuntimeSubsystem.Get(); Runtime && SupportEventHandle.IsValid())
	{
		Runtime->OnSupportEvent().Remove(SupportEventHandle);
	}
	CombatEventHandle.Reset();
	SupportEventHandle.Reset();
	if (UPRAbilitySystemComponent* PlayerASC = BoundPlayerASC.Get(); PlayerASC && PlayerDeadTagHandle.IsValid())
	{
		PlayerASC->RegisterGameplayTagEvent(UPRTagLibrary::GetStateDeadTag(), EGameplayTagEventType::NewOrRemoved).Remove(PlayerDeadTagHandle);
	}
	PlayerDeadTagHandle.Reset();
	BoundPlayerASC.Reset();
	if (UPRQTEComponent* Component = InputBridge.Get()) Component->DestroyComponent();
	InputBridge.Reset();
	CombatSubsystem.Reset();
	CompanionRuntimeSubsystem.Reset();
	LoadedAssets.Empty();
	RecentEvents.Empty();
	RecentRequests.Empty();
	FireSlashHits.Empty();
	ShieldBrokenTargetIds.Empty();
	AxiomShieldEffectClass = nullptr;
	StunnedEffectClass = nullptr;
	InvulnerableEffectClass = nullptr;
	bRegistryReady = false;
	StateChanged.Clear();
	ResultPublished.Clear();
	Super::Deinitialize();
}

bool UPRQTESubsystem::IsRegistryReady() const { return bRegistryReady; }
FPRQTERuntimeState UPRQTESubsystem::GetRuntimeState() const { return RuntimeState; }
FPRQTEStateChangedNative& UPRQTESubsystem::OnQTEStateChanged() { return StateChanged; }
FPRQTEResultNative& UPRQTESubsystem::OnQTEResult() { return ResultPublished; }

void UPRQTESubsystem::LoadFixedRegistry()
{
	RegistryAsset = TSoftObjectPtr<UPRQTERegistryDataAsset>(PRQTERegistry::RegistryPath);
	UPRQTERegistryDataAsset* Registry = RegistryAsset.LoadSynchronous();
	if (!Registry) return;
	for (const TSoftObjectPtr<UPRQTEDataAsset>& Entry : Registry->GetEntries())
	{
		UPRQTEDataAsset* Asset = Entry.LoadSynchronous();
		if (!Asset || !FPRQTEContract::IsExpectedQTEId(Asset->QTEId))
		{
			LoadedAssets.Empty();
			return;
		}
		LoadedAssets.Add(Asset);
	}
	AxiomShieldEffectClass = LoadClass<UGameplayEffect>(nullptr, *PRQTERegistry::AxiomShieldEffectPath.ToString());
	StunnedEffectClass = LoadClass<UGameplayEffect>(nullptr, *PRQTERegistry::StunnedEffectPath.ToString());
	InvulnerableEffectClass = LoadClass<UGameplayEffect>(nullptr, *PRQTERegistry::InvulnerableEffectPath.ToString());
	bRegistryReady = LoadedAssets.Num() == FPRQTEContract::GetExpectedQTEIds().Num();
}

void UPRQTESubsystem::ReconcileInputBridge()
{
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APRPlayerCharacter* Player = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
	if (!Controller || !Player) return;
	IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Player);
	UPRAbilitySystemComponent* PlayerASC = AbilityInterface ? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	if (BoundPlayerASC.Get() != PlayerASC)
	{
		if (UPRAbilitySystemComponent* PreviousASC = BoundPlayerASC.Get(); PreviousASC && PlayerDeadTagHandle.IsValid())
		{
			PreviousASC->RegisterGameplayTagEvent(UPRTagLibrary::GetStateDeadTag(), EGameplayTagEventType::NewOrRemoved).Remove(PlayerDeadTagHandle);
		}
		PlayerDeadTagHandle.Reset();
		BoundPlayerASC = PlayerASC;
		if (PlayerASC)
		{
			PlayerDeadTagHandle = PlayerASC->RegisterGameplayTagEvent(UPRTagLibrary::GetStateDeadTag(), EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UPRQTESubsystem::HandlePlayerDeathTagChanged);
		}
		if (RuntimeState.State == EPRQTEState::Active || RuntimeState.State == EPRQTEState::Resolving) CancelActiveQTE();
	}
	UPRQTEComponent* Component = InputBridge.Get();
	if (!Component)
	{
		Component = NewObject<UPRQTEComponent>(Controller, TEXT("QTEInputBridge"));
		if (!Component) return;
		Component->RegisterComponent();
		Component->InitializeForSubsystem(this);
		InputBridge = Component;
	}
	if (UPRQTERegistryDataAsset* Registry = RegistryAsset.Get())
	{
		Component->SetPromptWidgetClass(Registry->PromptWidgetClass.LoadSynchronous());
	}
	Component->RebindPlayerPawn(Player);
}

void UPRQTESubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (!Event.EventId.IsValid()) return;
	if (Event.AbilityTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"))) && Event.HealthDamage > 0.0f)
	{
		FFireSlashHit& Hit = FireSlashHits.AddDefaulted_GetRef();
		Hit.WorldTimeSeconds = Event.WorldTimeSeconds;
		Hit.TargetId = Event.TargetId;
		Hit.Target = Event.Target;
		FireSlashHits.RemoveAll([&Event](const FFireSlashHit& Existing) { return Event.WorldTimeSeconds - Existing.WorldTimeSeconds > 0.25; });
	}
	if (ActiveQTE.bAwaitingBasicAttackKill && IsBasicAttackKillForActiveQTE(Event))
	{
		ResolveActiveQTE(UPRTagLibrary::GetQTEResultSuccessTag(), UPRTagLibrary::GetInputAttackTag(), EPRQTETimingGrade::Standard);
		return;
	}
	if (Event.bFatal && Event.TargetId == TEXT("Player"))
	{
		CancelActiveQTE();
		return;
	}
	TryStartFromCombatEvent(Event);
	if (Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()) && !Event.TargetId.IsNone()) ShieldBrokenTargetIds.Add(Event.TargetId);
}

void UPRQTESubsystem::HandleSupportEvent(const FPRCompanionSupportEvent& Event)
{
	TryStartFromSupportEvent(Event);
}

bool UPRQTESubsystem::TryStartFromCombatEvent(const FPRCombatEvent& Event)
{
	if (!bRegistryReady || RuntimeState.State != EPRQTEState::Idle || !Event.EventId.IsValid()
		|| IsDuplicateEvent(Event.EventId, EPRQTETriggerSource::CombatEvent, Event.WorldTimeSeconds)) return false;
	UPRQTEDataAsset* Candidate = nullptr;
	for (UPRQTEDataAsset* Asset : LoadedAssets)
	{
		if (Asset && IsCandidateEligible(*Asset) && DoesCombatEventMatch(*Asset, Event)
			&& (!Candidate || FPRQTEContract::GetTypePriority(Asset->QTEType) > FPRQTEContract::GetTypePriority(Candidate->QTEType)
				|| (FPRQTEContract::GetTypePriority(Asset->QTEType) == FPRQTEContract::GetTypePriority(Candidate->QTEType)
					&& (Asset->Priority > Candidate->Priority || (Asset->Priority == Candidate->Priority && Asset->QTEId.LexicalLess(Candidate->QTEId))))))
		{
			Candidate = Asset;
		}
	}
	if (!Candidate) return false;
	FPRQTERequest Request;
	Request.RequestId = FGuid::NewGuid(); Request.QTEId = Candidate->GetPrimaryAssetId(); Request.CompanionId = Candidate->CompanionId;
	Request.TriggerEventId = Event.EventId; Request.TriggerSource = EPRQTETriggerSource::CombatEvent;
	Request.SourceId = Event.SourceId; Request.TargetId = Event.TargetId; Request.AbilityTag = Event.AbilityTag;
	Request.ResponseTags = Event.ResponseTags; Request.WorldTimeSeconds = Event.WorldTimeSeconds;
	return StartQTE(Candidate, Request, Event.Instigator.Get(), Event.Target.Get());
}

bool UPRQTESubsystem::TryStartFromSupportEvent(const FPRCompanionSupportEvent& Event)
{
	if (!bRegistryReady || RuntimeState.State != EPRQTEState::Idle || !Event.EventId.IsValid()
		|| IsDuplicateEvent(Event.EventId, EPRQTETriggerSource::CompanionSupportEvent, Event.WorldTimeSeconds)) return false;
	UPRQTEDataAsset* Candidate = nullptr;
	for (UPRQTEDataAsset* Asset : LoadedAssets)
	{
		if (Asset && IsCandidateEligible(*Asset) && DoesSupportEventMatch(*Asset, Event)
			&& (!Candidate || FPRQTEContract::GetTypePriority(Asset->QTEType) > FPRQTEContract::GetTypePriority(Candidate->QTEType)
				|| (FPRQTEContract::GetTypePriority(Asset->QTEType) == FPRQTEContract::GetTypePriority(Candidate->QTEType)
					&& (Asset->Priority > Candidate->Priority || (Asset->Priority == Candidate->Priority && Asset->QTEId.LexicalLess(Candidate->QTEId))))))
		{
			Candidate = Asset;
		}
	}
	if (!Candidate) return false;
	FPRQTERequest Request;
	Request.RequestId = FGuid::NewGuid(); Request.QTEId = Candidate->GetPrimaryAssetId(); Request.CompanionId = Candidate->CompanionId;
	Request.TriggerEventId = Event.EventId; Request.TriggerSource = EPRQTETriggerSource::CompanionSupportEvent;
	Request.SourceId = Event.SourceId; Request.TargetId = Event.TargetId; Request.WorldTimeSeconds = Event.WorldTimeSeconds;
	AActor* TargetActor = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APREnemyCharacter> It(World); It; ++It)
		{
			if ((*It)->GetCombatantId() == Event.TargetId) { TargetActor = *It; break; }
		}
	}
	return StartQTE(Candidate, Request, nullptr, TargetActor);
}

bool UPRQTESubsystem::IsDuplicateEvent(const FGuid& EventId, const EPRQTETriggerSource Source, const double EventTimeSeconds)
{
	const double Now = FMath::IsFinite(EventTimeSeconds) ? EventTimeSeconds : GetWorldTimeSeconds();
	RecentEvents.RemoveAll([Now](const FRecentEventRecord& Existing)
	{
		return Now - Existing.WorldTimeSeconds > PRQTERegistry::RecentEventWindowSeconds;
	});
	if (RecentEvents.ContainsByPredicate([EventId](const FRecentEventRecord& Existing) { return Existing.EventId == EventId; })) return true;
	const int32 Capacity = Source == EPRQTETriggerSource::CombatEvent
		? PRQTERegistry::MaximumRecentCombatEvents : PRQTERegistry::MaximumRecentSupportEvents;
	int32 SourceCount = 0;
	for (const FRecentEventRecord& Existing : RecentEvents)
	{
		if (Existing.Source == Source) ++SourceCount;
	}
	if (SourceCount >= Capacity) return true;
	FRecentEventRecord& Record = RecentEvents.AddDefaulted_GetRef();
	Record.EventId = EventId;
	Record.WorldTimeSeconds = Now;
	Record.Source = Source;
	return false;
}

bool UPRQTESubsystem::IsCandidateEligible(const UPRQTEDataAsset& Asset) const
{
	UPRAbilitySystemComponent* PlayerASC = BoundPlayerASC.Get();
	if (!PlayerASC || PlayerASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())) return false;
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!Controller || !IsValid(Controller->GetPawn())) return false;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	if (!Companions || !Companions->GetSyncState().PrimaryCompanionId.MatchesTagExact(Asset.CompanionId)) return false;
	FPRCompanionRelationshipRecord Relationship;
	return Companions->GetRelationshipSnapshot(Asset.CompanionId, Relationship)
		&& Relationship.State.Trust >= Asset.MinimumTrust && Relationship.State.Overload <= Asset.MaximumOverload;
}

bool UPRQTESubsystem::DoesCombatEventMatch(const UPRQTEDataAsset& Asset, const FPRCombatEvent& Event) const
{
	if (Asset.TriggerSource != EPRQTETriggerSource::CombatEvent) return false;
	if (Asset.TriggerTargetScope != EPRQTETargetScope::AnyEnemy && !DoesTargetMatchScope(Event.Target.Get(), Asset.TriggerTargetScope)) return false;
	if (Asset.RequiredAbilityTag.IsValid() && !Event.AbilityTag.MatchesTagExact(Asset.RequiredAbilityTag)) return false;
	if (Asset.RequiredResponseTag.IsValid() && !Event.ResponseTags.HasTagExact(Asset.RequiredResponseTag)) return false;
	switch (Asset.TriggerKind)
	{
	case EPRQTETriggerKind::ShieldBroken: return Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag())
		&& (!Asset.bRequireFirstTargetShieldBreak || !ShieldBrokenTargetIds.Contains(Event.TargetId));
	case EPRQTETriggerKind::PredictionBlocked: return Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePredictionBlockedTag());
	case EPRQTETriggerKind::ShadowThrustHit: return Event.AbilityTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"))) && Event.HealthDamage > 0.0f;
	case EPRQTETriggerKind::DecoyConsumed: return Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseDecoyConsumedTag()) && !Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	case EPRQTETriggerKind::PerfectDecoyConsumed: return Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseDecoyConsumedTag()) && Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	case EPRQTETriggerKind::LowHealthDamage: return IsPlayerLowHealthDamage(Event, Asset.TriggerHealthFraction);
	case EPRQTETriggerKind::FireSlashThreeHits: return GetRecentFireSlashTargets().Num() >= Asset.RequiredDistinctTargetCount
		&& Event.AbilityTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash")));
	case EPRQTETriggerKind::NormalLowHealth: return IsNormalEnemyLowHealth(Event, Asset.TriggerHealthFraction);
	default: return false;
	}
}

bool UPRQTESubsystem::DoesSupportEventMatch(const UPRQTEDataAsset& Asset, const FPRCompanionSupportEvent& Event) const
{
	if (Asset.TriggerSource != EPRQTETriggerSource::CompanionSupportEvent
		|| !Asset.CompanionId.MatchesTagExact(Event.CompanionId)
		|| Event.Result != EPRCompanionSupportResult::Applied) return false;
	if (Asset.TriggerKind == EPRQTETriggerKind::KindleCrowdSupport)
	{
		return Event.SupportType == EPRCompanionSupportType::LightDamage
			&& CountEffectTargets(260.0f, EPRQTETargetScope::LightEnemy) >= 3;
	}
	if (Asset.TriggerKind == EPRQTETriggerKind::SupportDuringAuditorWindup)
	{
		FPRBossRuntimeState BossState;
		return Event.SupportType == EPRCompanionSupportType::Shield && GetWorld() && GetWorld()->GetSubsystem<UPRBossSubsystem>()
			&& GetWorld()->GetSubsystem<UPRBossSubsystem>()->GetActiveBossState(BossState) && BossState.AttackPhase == 1;
	}
	return false;
}

bool UPRQTESubsystem::StartQTE(UPRQTEDataAsset* Asset, const FPRQTERequest& Request, AActor* SourceActor, AActor* TargetActor)
{
	if (!Asset || RuntimeState.State != EPRQTEState::Idle) return false;
	const double Now = GetWorldTimeSeconds();
	RecentRequests.RemoveAll([Now](const FRecentRequestRecord& Existing)
	{
		return Now - Existing.WorldTimeSeconds > PRQTERegistry::RecentRequestWindowSeconds;
	});
	if (RecentRequests.ContainsByPredicate([&Request](const FRecentRequestRecord& Existing) { return Existing.RequestId == Request.RequestId; })
		|| RecentRequests.Num() >= PRQTERegistry::MaximumRecentRequests) return false;
	FRecentRequestRecord& RecentRequest = RecentRequests.AddDefaulted_GetRef();
	RecentRequest.RequestId = Request.RequestId;
	RecentRequest.WorldTimeSeconds = Now;
	ActiveQTE.Asset = Asset; ActiveQTE.SourceActor = SourceActor; ActiveQTE.TargetActor = TargetActor; ActiveQTE.Request = Request;
	if (Asset->TriggerKind == EPRQTETriggerKind::FireSlashThreeHits)
	{
		ActiveQTE.EffectTargets = GetRecentFireSlashTargets();
	}
	RuntimeState.RequestId = Request.RequestId; RuntimeState.QTEId = Request.QTEId; RuntimeState.CompanionId = Asset->CompanionId;
	RuntimeState.QTEType = Asset->QTEType; RuntimeState.DisplayName = Asset->DisplayName; RuntimeState.PromptText = Asset->PromptText;
	RuntimeState.TargetId = Request.TargetId; RuntimeState.AcceptedInputTags = Asset->AcceptedInputTags;
	RuntimeState.StartTimeSeconds = GetWorldTimeSeconds(); RuntimeState.DeadlineTimeSeconds = RuntimeState.StartTimeSeconds + Asset->WindowSeconds;
	RuntimeState.RemainingSeconds = Asset->WindowSeconds; BroadcastState(EPRQTEState::Active);
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(ActiveQTE.TimeoutTimer, this, &UPRQTESubsystem::HandleTimeout, Asset->WindowSeconds, false);
	return true;
}

bool UPRQTESubsystem::SubmitSemanticInput(const FGameplayTag InputTag, const double InputTimeSeconds)
{
	if (RuntimeState.State != EPRQTEState::Active) return false;
	if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputQTERejectTag()))
	{
		ResolveActiveQTE(UPRTagLibrary::GetQTEResultRejectedTag(), InputTag, EPRQTETimingGrade::None);
		return true;
	}
	if (!RuntimeState.AcceptedInputTags.HasTagExact(InputTag)) return false;
	const double PerfectDeadline = RuntimeState.StartTimeSeconds + (RuntimeState.DeadlineTimeSeconds - RuntimeState.StartTimeSeconds) * 0.35;
	if (UPRQTEDataAsset* Asset = ActiveQTE.Asset.Get(); Asset && !ActiveQTE.bAwaitingBasicAttackKill)
	{
		const FPRQTEEffectDefinition* Confirmation = Asset->SuccessEffects.FindByPredicate([](const FPRQTEEffectDefinition& Effect) { return Effect.EffectKind == EPRQTEEffectKind::ConfirmBasicAttackKill; });
		if (Confirmation)
		{
			if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(ActiveQTE.TimeoutTimer);
			ActiveQTE.bAwaitingBasicAttackKill = true;
			BroadcastState(EPRQTEState::Resolving);
			if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(ActiveQTE.ConfirmationTimer, this, &UPRQTESubsystem::HandleConfirmationTimeout, Confirmation->Magnitude, false);
			return true;
		}
	}
	ResolveActiveQTE(UPRTagLibrary::GetQTEResultSuccessTag(), InputTag,
		InputTimeSeconds <= PerfectDeadline ? EPRQTETimingGrade::Perfect : EPRQTETimingGrade::Standard);
	return true;
}

void UPRQTESubsystem::HandleTimeout()
{
	if (RuntimeState.State == EPRQTEState::Active) ResolveActiveQTE(UPRTagLibrary::GetQTEResultTimeoutTag(), FGameplayTag(), EPRQTETimingGrade::None);
}

void UPRQTESubsystem::HandleConfirmationTimeout()
{
	if (ActiveQTE.bAwaitingBasicAttackKill) ResolveActiveQTE(UPRTagLibrary::GetQTEResultFailureTag(), FGameplayTag(), EPRQTETimingGrade::None);
}

void UPRQTESubsystem::CancelActiveQTE()
{
	if (RuntimeState.State == EPRQTEState::Active || RuntimeState.State == EPRQTEState::Resolving)
	{
		ClearRuntimeEffectTimers();
		ResolveActiveQTE(UPRTagLibrary::GetQTEResultCancelledTag(), FGameplayTag(), EPRQTETimingGrade::None);
	}
}

void UPRQTESubsystem::ResolveActiveQTE(const FGameplayTag ResultTag, const FGameplayTag SubmittedInput, const EPRQTETimingGrade TimingGrade)
{
	UPRQTEDataAsset* Asset = ActiveQTE.Asset.Get();
	if (!Asset || RuntimeState.State == EPRQTEState::Idle) return;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveQTE.TimeoutTimer);
		World->GetTimerManager().ClearTimer(ActiveQTE.ConfirmationTimer);
	}
	BroadcastState(EPRQTEState::Resolving);
	FPRQTEResult Result;
	Result.ResultId = FGuid::NewGuid(); Result.RequestId = ActiveQTE.Request.RequestId; Result.QTEId = Asset->GetPrimaryAssetId();
	Result.CompanionId = Asset->CompanionId; Result.ResultTag = ResultTag; Result.TriggerEventId = ActiveQTE.Request.TriggerEventId;
	Result.TargetId = ActiveQTE.Request.TargetId; Result.SubmittedInput = SubmittedInput; Result.TimingGrade = TimingGrade;
	Result.WorldTimeSeconds = GetWorldTimeSeconds();
	if (ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag()))
	{
		ApplySuccessEffects(*Asset, Result);
	}
	if (!ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultCancelledTag()))
	{
		FPRRelationshipDelta Delta = ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag()) ? Asset->RelationshipDeltas.Success
			: ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultRejectedTag()) ? Asset->RelationshipDeltas.Rejected
			: ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultTimeoutTag()) ? Asset->RelationshipDeltas.Timeout : Asset->RelationshipDeltas.Failure;
		Delta.CompanionId = Asset->CompanionId; Delta.SourceId = Asset->QTEId;
		Result.RelationshipDeltaRequest = Delta;
		UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
		if (UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr)
		{
			Result.RelationshipApplyResult = static_cast<uint8>(Companions->ApplyRelationshipDelta(Delta));
			if (Result.RelationshipApplyResult == static_cast<uint8>(EPRRelationshipApplyResult::Applied)
				|| Result.RelationshipApplyResult == static_cast<uint8>(EPRRelationshipApplyResult::AppliedClamped))
			{
				if (UPRSaveSubsystem* Save = GameInstance->GetSubsystem<UPRSaveSubsystem>())
				{
					Save->RequestSaveCurrentProfile(Result.SaveRequestId);
				}
			}
		}
	}
	CompleteResolvedQTE(*Asset, MoveTemp(Result));
}

void UPRQTESubsystem::CompleteResolvedQTE(UPRQTEDataAsset& Asset, FPRQTEResult&& Result)
{
	ResultPublished.Broadcast(Result);
	const float Cooldown = CalculateEffectiveCooldown(Asset);
	ClearActiveQTE();
	ScheduleCooldown(Cooldown);
}

bool UPRQTESubsystem::ApplySuccessEffects(const UPRQTEDataAsset& Asset, FPRQTEResult& InOutResult)
{
	bool bAllApplied = true;
	for (const FPRQTEEffectDefinition& Effect : Asset.SuccessEffects)
	{
		bAllApplied = ApplyEffect(Asset, Effect, InOutResult) && bAllApplied;
	}
	return bAllApplied;
}

bool UPRQTESubsystem::ApplyEffect(const UPRQTEDataAsset& Asset, const FPRQTEEffectDefinition& Effect, FPRQTEResult& InOutResult)
{
	const FName EffectId(*StaticEnum<EPRQTEEffectKind>()->GetNameStringByValue(static_cast<int64>(Effect.EffectKind)));
	switch (Effect.EffectKind)
	{
	case EPRQTEEffectKind::Damage:
		if (AActor* Target = ResolveEffectTarget(Effect.Range, false, Effect.TargetScope)) return ApplyDamageToTarget(*Target, Effect.Magnitude, Asset, EffectId, InOutResult);
		return false;
	case EPRQTEEffectKind::PulseDamage:
	{
		TArray<AActor*> Targets;
		for (const TWeakObjectPtr<AActor>& SnapshotTarget : ActiveQTE.EffectTargets)
		{
			if (AActor* Target = SnapshotTarget.Get(); Target && DoesTargetMatchScope(Target, Effect.TargetScope)) Targets.AddUnique(Target);
		}
		if (Targets.IsEmpty()) Targets = ResolveEffectTargets(Effect.Range, Effect.MaxTargets, Effect.TargetScope);
		if (Targets.IsEmpty() || Effect.PulseCount <= 0) return false;
		for (AActor* Target : Targets)
		{
			for (int32 Index = 0; Index < Effect.PulseCount; ++Index)
			{
				if (Index == 0)
				{
					ApplyPulseDamage(const_cast<UPRQTEDataAsset*>(&Asset), Target, Effect.Magnitude, EffectId);
				}
				else if (UWorld* World = GetWorld())
				{
					FTimerHandle Handle;
					World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UPRQTESubsystem::ApplyPulseDamage, TWeakObjectPtr<UPRQTEDataAsset>(const_cast<UPRQTEDataAsset*>(&Asset)), TWeakObjectPtr<AActor>(Target), Effect.Magnitude, EffectId), Effect.PulseIntervalSeconds * Index, false);
					RuntimeEffectTimers.Add(Handle);
				}
			}
		}
		InOutResult.AppliedEffectIds.Add(EffectId);
		return true;
	}
	case EPRQTEEffectKind::ShieldAndInvulnerable:
		return ApplyPlayerDefense(Effect.Magnitude, EffectId, InOutResult);
	case EPRQTEEffectKind::Stun:
	{
		bool bApplied = false;
		for (AActor* Target : ResolveEffectTargets(Effect.Range, FMath::Max(1, Effect.MaxTargets), Effect.TargetScope))
		{
			bApplied = ApplyStunToTarget(*Target, EffectId, InOutResult) || bApplied;
		}
		return bApplied;
	}
	case EPRQTEEffectKind::FutureSample:
		InOutResult.AppliedEffectIds.Add(Effect.FutureSampleId.IsNone() ? EffectId : Effect.FutureSampleId);
		return true;
	case EPRQTEEffectKind::ConfirmBasicAttackKill:
		InOutResult.AppliedEffectIds.Add(EffectId);
		return true;
	default:
		return false;
	}
}

void UPRQTESubsystem::ApplyPulseDamage(TWeakObjectPtr<UPRQTEDataAsset> Asset, TWeakObjectPtr<AActor> Target, const float Damage, const FName EffectId)
{
	if (!Asset.IsValid() || !Target.IsValid() || !IsValid(Target.Get())) return;
	FPRQTEResult DiscardedResult;
	ApplyDamageToTarget(*Target.Get(), Damage, *Asset.Get(), EffectId, DiscardedResult);
}

AActor* UPRQTESubsystem::ResolveEffectTarget(const float Range, const bool bAllowPlayerTarget, const EPRQTETargetScope TargetScope) const
{
	if (AActor* ActiveTarget = ActiveQTE.TargetActor.Get(); IsValid(ActiveTarget) && (bAllowPlayerTarget || ActiveTarget->IsA<APREnemyCharacter>()) && DoesTargetMatchScope(ActiveTarget, TargetScope)) return ActiveTarget;
	APRPlayerCharacter* Player = GetWorld() ? Cast<APRPlayerCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	if (!GetWorld() || !Player) return nullptr;
	APREnemyCharacter* Best = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (TActorIterator<APREnemyCharacter> It(GetWorld()); It; ++It)
	{
		APREnemyCharacter* Candidate = *It;
		if (!Candidate || !Candidate->IsEnemyInitialized() || Candidate->IsEnemyDead()) continue;
		const float DistanceSq = FVector::DistSquared2D(Player->GetActorLocation(), Candidate->GetActorLocation());
		if (DoesTargetMatchScope(Candidate, TargetScope) && (Range <= 0.0f || DistanceSq <= FMath::Square(Range)) && DistanceSq < BestDistanceSq)
		{
			Best = Candidate;
			BestDistanceSq = DistanceSq;
		}
	}
	return Best;
}

TArray<AActor*> UPRQTESubsystem::ResolveEffectTargets(const float Range, const int32 MaximumTargets, const EPRQTETargetScope TargetScope) const
{
	TArray<AActor*> Result;
	APRPlayerCharacter* Player = GetWorld() ? Cast<APRPlayerCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	if (!GetWorld() || !Player || MaximumTargets <= 0) return Result;
	TArray<TPair<float, APREnemyCharacter*>> Candidates;
	for (TActorIterator<APREnemyCharacter> It(GetWorld()); It; ++It)
	{
		APREnemyCharacter* Candidate = *It;
		if (!Candidate || !Candidate->IsEnemyInitialized() || Candidate->IsEnemyDead()) continue;
		const float DistanceSq = FVector::DistSquared2D(Player->GetActorLocation(), Candidate->GetActorLocation());
		if (DoesTargetMatchScope(Candidate, TargetScope) && (Range <= 0.0f || DistanceSq <= FMath::Square(Range))) Candidates.Emplace(DistanceSq, Candidate);
	}
	Candidates.Sort([](const TPair<float, APREnemyCharacter*>& A, const TPair<float, APREnemyCharacter*>& B)
	{
		return FMath::IsNearlyEqual(A.Key, B.Key)
			? A.Value->GetAbilityTargetId().LexicalLess(B.Value->GetAbilityTargetId())
			: A.Key < B.Key;
	});
	for (const TPair<float, APREnemyCharacter*>& Candidate : Candidates)
	{
		Result.Add(Candidate.Value);
		if (Result.Num() >= MaximumTargets) break;
	}
	return Result;
}

int32 UPRQTESubsystem::CountEffectTargets(const float Range, const EPRQTETargetScope TargetScope) const
{
	return ResolveEffectTargets(Range, TNumericLimits<int32>::Max(), TargetScope).Num();
}

bool UPRQTESubsystem::DoesTargetMatchScope(const AActor* Target, const EPRQTETargetScope TargetScope) const
{
	const APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(Target);
	if (!Enemy || !Enemy->IsEnemyInitialized() || Enemy->IsEnemyDead()) return false;
	if (TargetScope == EPRQTETargetScope::AnyEnemy) return true;
	if (TargetScope == EPRQTETargetScope::NonBossEnemy) return !Enemy->IsA<APRBossAuditor>();
	if (TargetScope == EPRQTETargetScope::EliteOrBoss)
	{
		const UPREnemyPrototypeDataAsset* Prototype = Enemy->GetEnemyPrototype();
		return Enemy->IsA<APRBossAuditor>() || (Prototype && Prototype->PrototypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false)));
	}
	if (TargetScope == EPRQTETargetScope::NormalEnemy)
	{
		const UPREnemyPrototypeDataAsset* Prototype = Enemy->GetEnemyPrototype();
		return !Enemy->IsA<APRBossAuditor>() && (!Prototype || !Prototype->PrototypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false)));
	}
	return Enemy->GetAbilityTargetMobility() == EPRAbilityTargetMobility::Light;
}

TArray<TWeakObjectPtr<AActor>> UPRQTESubsystem::GetRecentFireSlashTargets() const
{
	TArray<TWeakObjectPtr<AActor>> Targets;
	for (const FFireSlashHit& Hit : FireSlashHits)
	{
		if (AActor* Target = Hit.Target.Get(); Target && DoesTargetMatchScope(Target, EPRQTETargetScope::AnyEnemy)) Targets.AddUnique(Target);
	}
	return Targets;
}

bool UPRQTESubsystem::ApplyDamageToTarget(AActor& Target, const float RequestedDamage, const UPRQTEDataAsset& Asset, const FName EffectId, FPRQTEResult& InOutResult)
{
	APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(&Target);
	const UPRAttributeSet* Attributes = Enemy ? Enemy->GetAttributeSet() : nullptr;
	if (!Enemy || !Attributes || Enemy->IsEnemyDead() || RequestedDamage <= 0.0f) return false;
	float Floor = PRQTERegistry::NormalEnemyHealthFloorFraction * Attributes->GetMaxHealth();
	if (Enemy->IsA<APRBossAuditor>()) Floor = PRQTERegistry::BossHealthFloor;
	else if (const UPREnemyPrototypeDataAsset* Prototype = Enemy->GetEnemyPrototype(); Prototype && Prototype->PrototypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false))) Floor = PRQTERegistry::EliteEnemyHealthFloorFraction * Attributes->GetMaxHealth();
	const float AllowedDamage = FMath::Max(0.0f, Attributes->GetShield() + Attributes->GetHealth() - Floor);
	const float Damage = FMath::Min(RequestedDamage, AllowedDamage);
	APRPlayerCharacter* Player = GetWorld() ? Cast<APRPlayerCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	UPRCombatSubsystem* Combat = CombatSubsystem.Get();
	if (!Combat || !Player || Damage <= 0.0f) return false;
	FPRDamageRequest Request;
	Request.SourceId = FName(*FString::Printf(TEXT("QTE.%s"), *Asset.QTEId.ToString()));
	Request.DamageSource = this;
	Request.Instigator = Player;
	Request.Target = Enemy;
	Request.AbilityTag = FGameplayTag();
	Request.RawDamage = Damage;
	Request.DamageTags.AddTag(Asset.CompanionId);
	Request.ImpactOrigin = Player->GetActorLocation();
	Request.IncomingDirection = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
	if (Combat->ApplyDamage(Request) != EPRCombatRequestStatus::Applied) return false;
	InOutResult.AppliedEffectIds.Add(EffectId);
	return true;
}

bool UPRQTESubsystem::ApplyStunToTarget(AActor& Target, const FName EffectId, FPRQTEResult& InOutResult)
{
	APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(&Target);
	UPRAbilitySystemComponent* ASC = Enemy ? Enemy->GetProjectRAbilitySystemComponent() : nullptr;
	if (!ASC || !StunnedEffectClass || ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())) return false;
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(StunnedEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
	if (!Handle.IsValid()) return false;
	InOutResult.AppliedEffectIds.Add(EffectId);
	return true;
}

bool UPRQTESubsystem::ApplyPlayerDefense(const float ShieldAmount, const FName EffectId, FPRQTEResult& InOutResult)
{
	(void)ShieldAmount; // The frozen GE_Companion_Axiom_Shield owns the fixed +20 modifier.
	APRPlayerCharacter* Player = GetWorld() ? Cast<APRPlayerCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	IAbilitySystemInterface* AbilityInterface = Player ? Cast<IAbilitySystemInterface>(Player) : nullptr;
	UPRAbilitySystemComponent* ASC = AbilityInterface ? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	if (!ASC || !AxiomShieldEffectClass || !InvulnerableEffectClass) return false;
	const FGameplayEffectSpecHandle ShieldSpec = ASC->MakeOutgoingSpec(AxiomShieldEffectClass, 1.0f, ASC->MakeEffectContext());
	if (!ShieldSpec.IsValid() || !ShieldSpec.Data.IsValid()) return false;
	ASC->ApplyGameplayEffectSpecToSelf(*ShieldSpec.Data.Get());
	const FActiveGameplayEffectHandle InvulnerableHandle = ASC->ApplyGameplayEffectToSelf(InvulnerableEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
	if (!InvulnerableHandle.IsValid()) return false;
	InOutResult.AppliedEffectIds.Add(EffectId);
	return true;
}

bool UPRQTESubsystem::IsPlayerLowHealthDamage(const FPRCombatEvent& Event, const float RequiredHealthFraction) const
{
	APRPlayerCharacter* Player = GetWorld() ? Cast<APRPlayerCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	if (!Player || Event.Target.Get() != Player || Event.HealthDamage <= 0.0f || Event.bFatal) return false;
	IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Player);
	const UPRAbilitySystemComponent* ASC = AbilityInterface ? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	const UPRAttributeSet* Attributes = ASC ? Cast<UPRAttributeSet>(ASC->GetAttributeSet(UPRAttributeSet::StaticClass())) : nullptr;
	const float Threshold = RequiredHealthFraction > 0.0f ? RequiredHealthFraction : PRQTERegistry::PlayerLowHealthFraction;
	return Attributes && Attributes->GetMaxHealth() > 0.0f && Attributes->GetHealth() / Attributes->GetMaxHealth() <= Threshold;
}

bool UPRQTESubsystem::IsNormalEnemyLowHealth(const FPRCombatEvent& Event, const float RequiredHealthFraction) const
{
	APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(Event.Target.Get());
	const UPRAttributeSet* Attributes = Enemy ? Enemy->GetAttributeSet() : nullptr;
	const UPREnemyPrototypeDataAsset* Prototype = Enemy ? Enemy->GetEnemyPrototype() : nullptr;
	return Enemy && Attributes && Prototype && !Enemy->IsA<APRBossAuditor>()
		&& !Prototype->PrototypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false))
		&& !Event.bFatal && Attributes->GetMaxHealth() > 0.0f
		&& Attributes->GetHealth() / Attributes->GetMaxHealth() <= (RequiredHealthFraction > 0.0f ? RequiredHealthFraction : PRQTERegistry::NormalEnemyLowHealthFraction);
}

bool UPRQTESubsystem::IsBasicAttackKillForActiveQTE(const FPRCombatEvent& Event) const
{
	return ActiveQTE.bAwaitingBasicAttackKill && Event.bFatal
		&& Event.TargetId == ActiveQTE.Request.TargetId
		&& Event.AbilityTag.MatchesTagExact(UPRTagLibrary::GetSkillBasicAttackTag());
}

void UPRQTESubsystem::HandlePlayerDeathTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	if (Tag.MatchesTagExact(UPRTagLibrary::GetStateDeadTag()) && NewCount > 0) CancelActiveQTE();
}

void UPRQTESubsystem::ClearRuntimeEffectTimers()
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : RuntimeEffectTimers) World->GetTimerManager().ClearTimer(Handle);
	}
	RuntimeEffectTimers.Empty();
}

void UPRQTESubsystem::BroadcastState(const EPRQTEState NewState)
{
	RuntimeState.State = NewState;
	RuntimeState.RemainingSeconds = NewState == EPRQTEState::Active ? FMath::Max(0.0, RuntimeState.DeadlineTimeSeconds - GetWorldTimeSeconds()) : 0.0f;
	StateChanged.Broadcast(RuntimeState);
}

void UPRQTESubsystem::ClearActiveQTE()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveQTE.TimeoutTimer);
		World->GetTimerManager().ClearTimer(ActiveQTE.ConfirmationTimer);
	}
	ActiveQTE = FActiveQTE();
	RuntimeState = FPRQTERuntimeState();
}

void UPRQTESubsystem::ScheduleCooldown(const float CooldownSeconds)
{
	BroadcastState(EPRQTEState::Cooldown);
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(CooldownTimer, this, &UPRQTESubsystem::EndCooldown, FMath::Max(0.01f, CooldownSeconds), false);
}

void UPRQTESubsystem::EndCooldown()
{
	RuntimeState = FPRQTERuntimeState();
	BroadcastState(EPRQTEState::Idle);
}

float UPRQTESubsystem::CalculateEffectiveCooldown(const UPRQTEDataAsset& Asset) const
{
	FPRCompanionRelationshipRecord Relationship;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		Companions && Companions->GetRelationshipSnapshot(Asset.CompanionId, Relationship))
	{
		return FPRQTEContract::CalculateEffectiveCooldown(Asset.BaseCooldownSeconds, Relationship.State.Overload);
	}
	return FPRQTEContract::CalculateEffectiveCooldown(Asset.BaseCooldownSeconds, 0);
}

double UPRQTESubsystem::GetWorldTimeSeconds() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
