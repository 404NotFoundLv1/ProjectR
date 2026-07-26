// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRPlayerProfileSubsystem.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRPlayerState.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Engine/World.h"
#include "QTE/PRQTESubsystem.h"
#include "Save/PRSaveSubsystem.h"

void UPRPlayerProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UPRSaveSubsystem* Save = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>()) SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRPlayerProfileSubsystem::HandleSaveOperation);
	if (UPRCompanionSubsystem* Companions = GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>())
	{
		RelationshipChangedHandle = Companions->OnRelationshipChanged().AddUObject(this, &UPRPlayerProfileSubsystem::HandleRelationshipChanged);
		PrimarySyncHandle = Companions->OnPrimarySyncChanged().AddUObject(this, &UPRPlayerProfileSubsystem::HandlePrimarySyncChanged);
	}
	if (UPRDivergenceSubsystem* Divergence = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>()) DivergenceResultHandle = Divergence->OnDivergenceResult().AddUObject(this, &UPRPlayerProfileSubsystem::HandleDivergenceResult);
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRPlayerProfileSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRPlayerProfileSubsystem::HandleWorldCleanup);
}

void UPRPlayerProfileSubsystem::Deinitialize()
{
	if (UPRSaveSubsystem* Save = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>()) Save->OnSaveOperation().Remove(SaveOperationHandle);
	if (UPRCompanionSubsystem* Companions = GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>()) { Companions->OnRelationshipChanged().Remove(RelationshipChangedHandle); Companions->OnPrimarySyncChanged().Remove(PrimarySyncHandle); }
	if (UPRDivergenceSubsystem* Divergence = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>()) Divergence->OnDivergenceResult().Remove(DivergenceResultHandle);
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle); FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	UnbindPlayerSources();
	HandleWorldCleanup(BoundWorld.Get(), false, true);
	bHasSession = false;
	Snapshot = FPRPlayerProfileSnapshot();
	ProfileChanged.Clear();
	Super::Deinitialize();
}

bool UPRPlayerProfileSubsystem::GetSnapshot(FPRPlayerProfileSnapshot& OutSnapshot) const
{
	if (!bHasSession) return false;
	Snapshot.SnapshotId = FGuid::NewGuid();
	++Snapshot.Sequence;
	OutSnapshot = Snapshot;
	return true;
}

FPRPlayerProfileChangedNative& UPRPlayerProfileSubsystem::OnPlayerProfileChanged() { return ProfileChanged; }

void UPRPlayerProfileSubsystem::BeginProfileSession()
{
	Snapshot = FPRPlayerProfileSnapshot();
	Snapshot.ProfileSessionId = FGuid::NewGuid();
	bHasSession = true;
	ProfileChanged.Broadcast(Snapshot);
}

void UPRPlayerProfileSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if ((Event.Operation == EPRSaveOperationType::Create || Event.Operation == EPRSaveOperationType::Load) && (Event.Result == EPRSaveResult::Success || Event.Result == EPRSaveResult::RecoveredFromAlternate)) BeginProfileSession();
}

void UPRPlayerProfileSubsystem::HandleRelationshipChanged(const FPRRelationshipChangedEvent& Event)
{
	if (!bHasSession || !Event.CompanionId.IsValid()) return;
	FPRCompanionRelationshipRecord* Existing = Snapshot.Relationships.FindByPredicate([&Event](const FPRCompanionRelationshipRecord& Item) { return Item.CompanionId == Event.CompanionId; });
	if (!Existing) { if (Snapshot.Relationships.Num() >= 3) return; Existing = &Snapshot.Relationships.AddDefaulted_GetRef(); Existing->CompanionId = Event.CompanionId; }
	Existing->State = Event.CurrentState;
	Snapshot.Relationships.Sort([](const FPRCompanionRelationshipRecord& Left, const FPRCompanionRelationshipRecord& Right) { return Left.CompanionId.ToString() < Right.CompanionId.ToString(); }); PublishProfileChange();
}

void UPRPlayerProfileSubsystem::HandlePrimarySyncChanged(const FPRPrimaryCompanionSyncChangedEvent& Event) { if (bHasSession) { Snapshot.PrimaryCompanionId = Event.CurrentState.PrimaryCompanionId; PublishProfileChange(); } }
void UPRPlayerProfileSubsystem::HandleDivergenceResult(const FPRDivergenceResult& Result) { if (bHasSession && ConsumeUniqueId(Result.ResultId)) { Snapshot.LastDivergence = { Result.ResultId, Result.CompanionId, Result.Choice, Result.Resolution, Result.FutureDisposition }; PublishProfileChange(); } }

void UPRPlayerProfileSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	if (!World || World->GetGameInstance() != GetGameInstance()) return;
	HandleWorldCleanup(BoundWorld.Get(), false, true); BoundWorld = World;
	WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UPRPlayerProfileSubsystem::HandleWorldBeginPlay);
	if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>()) CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRPlayerProfileSubsystem::HandleCombatEvent);
	if (UPRQTESubsystem* QTE = World->GetSubsystem<UPRQTESubsystem>()) QTEResultHandle = QTE->OnQTEResult().AddUObject(this, &UPRPlayerProfileSubsystem::HandleQTEResult);
	BindPlayerSources(World);
}

void UPRPlayerProfileSubsystem::HandleWorldBeginPlay()
{
	if (UWorld* World = BoundWorld.Get(); World && World->GetGameInstance() == GetGameInstance()) BindPlayerSources(World);
}

void UPRPlayerProfileSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!World || World != BoundWorld.Get()) return;
	UnbindPlayerSources();
	if (WorldBeginPlayHandle.IsValid()) World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>()) Combat->OnCombatEvent().Remove(CombatEventHandle);
	if (UPRQTESubsystem* QTE = World->GetSubsystem<UPRQTESubsystem>()) QTE->OnQTEResult().Remove(QTEResultHandle);
	BoundWorld = nullptr; WorldBeginPlayHandle.Reset(); CombatEventHandle.Reset(); QTEResultHandle.Reset();
}

void UPRPlayerProfileSubsystem::BindPlayerSources(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance()) return;
	APlayerController* Controller = World->GetFirstPlayerController();
	APRPlayerState* PlayerState = Controller ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
	UPRAbilitySystemComponent* AbilitySystem = PlayerState ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
	if (!IsValid(PlayerState) || !IsValid(AbilitySystem)) return;
	if (BoundPlayerState.Get() == PlayerState && BoundAbilitySystem.Get() == AbilitySystem) return;
	UnbindPlayerSources();
	BoundPlayerState = PlayerState;
	BoundAbilitySystem = AbilitySystem;
	AbilityLifecycleHandle = AbilitySystem->OnAbilityLifecycleEvent().AddUObject(this, &UPRPlayerProfileSubsystem::HandleAbilityLifecycle);
	AttributeChangedHandle = PlayerState->OnAttributeChanged().AddUObject(this, &UPRPlayerProfileSubsystem::HandleAttributeChanged);
}

void UPRPlayerProfileSubsystem::UnbindPlayerSources()
{
	if (UPRAbilitySystemComponent* AbilitySystem = BoundAbilitySystem.Get(); AbilitySystem && AbilityLifecycleHandle.IsValid()) AbilitySystem->OnAbilityLifecycleEvent().Remove(AbilityLifecycleHandle);
	if (APRPlayerState* PlayerState = BoundPlayerState.Get(); PlayerState && AttributeChangedHandle.IsValid()) PlayerState->OnAttributeChanged().Remove(AttributeChangedHandle);
	BoundPlayerState.Reset(); BoundAbilitySystem.Reset(); AbilityLifecycleHandle.Reset(); AttributeChangedHandle.Reset();
}

void UPRPlayerProfileSubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (!bHasSession || !ConsumeUniqueId(Event.EventId)) return;
	Snapshot.Resources.DamageDealt = FMath::Max(0.0f, Snapshot.Resources.DamageDealt + (Event.SourceId == TEXT("Player") ? Event.HealthDamage : 0.0f));
	Snapshot.Resources.DamageTaken = FMath::Max(0.0f, Snapshot.Resources.DamageTaken + (Event.TargetId == TEXT("Player") ? Event.HealthDamage : 0.0f));
	Snapshot.Resources.ShieldAbsorbed = FMath::Max(0.0f, Snapshot.Resources.ShieldAbsorbed + Event.ShieldAbsorbed);
	if (Event.bFatal && Event.TargetId == TEXT("Player")) ++Snapshot.DeathCount;
	if (Event.MaxHealth > 0.0f && Event.TargetId == TEXT("Player")) Snapshot.Resources.MinimumHealthRatio = FMath::Min(Snapshot.Resources.MinimumHealthRatio, FMath::Clamp(Event.RemainingHealth / Event.MaxHealth, 0.0f, 1.0f));
	if (const AActor* Instigator = Event.Instigator.Get()) if (const AActor* Target = Event.Target.Get()) { const float Distance = FVector::Dist2D(Instigator->GetActorLocation(), Target->GetActorLocation()); FPRPlayerProfileDistanceMetric& Metric = Snapshot.CombatDistance; Metric.MinimumDistanceCm = Metric.SampleCount == 0 ? Distance : FMath::Min(Metric.MinimumDistanceCm, Distance); Metric.MaximumDistanceCm = FMath::Max(Metric.MaximumDistanceCm, Distance); Metric.AverageDistanceCm = (Metric.AverageDistanceCm * Metric.SampleCount + Distance) / (Metric.SampleCount + 1); ++Metric.SampleCount; }
	Snapshot.WorldTimeSeconds = Event.WorldTimeSeconds; PublishProfileChange();
}

void UPRPlayerProfileSubsystem::HandleQTEResult(const FPRQTEResult& Result)
{
	if (!bHasSession || !ConsumeUniqueId(Result.ResultId)) return;
	AddTaggedCount(Snapshot.QTEResultCounts, Result.ResultTag, 1); for (const FGameplayTag& Tag : Result.ProfileSampleTags) AddTaggedCount(Snapshot.QTEProfileSampleCounts, Tag, 1); if (Result.TimingGrade == EPRQTETimingGrade::Perfect) ++Snapshot.QTEPerfectTimingCount; Snapshot.WorldTimeSeconds = Result.WorldTimeSeconds; PublishProfileChange();
}

void UPRPlayerProfileSubsystem::HandleAbilityLifecycle(const FPRAbilityLifecycleEvent& Event)
{
	if (!bHasSession || !Event.AbilityState.AbilityTag.IsValid()) return;
	FPRPlayerProfileSkillMetric* Metric = Snapshot.SkillMetrics.FindByPredicate([&Event](const FPRPlayerProfileSkillMetric& Item) { return Item.SkillTag == Event.AbilityState.AbilityTag; });
	if (!Metric) { if (Snapshot.SkillMetrics.Num() >= 16) return; Metric = &Snapshot.SkillMetrics.AddDefaulted_GetRef(); Metric->SkillTag = Event.AbilityState.AbilityTag; }
	if (Event.EventType == EPRAbilityLifecycleEventType::Activated) ++Metric->UseCount; else if (Event.EventType == EPRAbilityLifecycleEventType::Committed) ++Metric->CommitCount; else if (Event.EventType == EPRAbilityLifecycleEventType::CommitFailed || Event.EventType == EPRAbilityLifecycleEventType::ActivationFailed) ++Metric->FailureCount;
	Snapshot.SkillMetrics.Sort([](const FPRPlayerProfileSkillMetric& Left, const FPRPlayerProfileSkillMetric& Right) { return Left.SkillTag.ToString() < Right.SkillTag.ToString(); }); PublishProfileChange();
}

void UPRPlayerProfileSubsystem::HandleAttributeChanged(const FPRAttributeChange& Change)
{
	if (!bHasSession || Change.Attribute != UPRAttributeSet::GetEnergyAttribute() || Change.NewValue >= Change.OldValue) return;
	Snapshot.Resources.EnergySpent = FMath::Max(0.0f, Snapshot.Resources.EnergySpent + (Change.OldValue - Change.NewValue));
	PublishProfileChange();
}

void UPRPlayerProfileSubsystem::AddTaggedCount(TArray<FPRPlayerProfileTaggedCount>& Counts, const FGameplayTag Tag, const int32 Amount)
{
	if (!Tag.IsValid() || Amount <= 0) return; FPRPlayerProfileTaggedCount* Existing = Counts.FindByPredicate([Tag](const FPRPlayerProfileTaggedCount& Item) { return Item.Tag == Tag; }); if (!Existing) { if (Counts.Num() >= 16) return; Existing = &Counts.AddDefaulted_GetRef(); Existing->Tag = Tag; } Existing->Count = FMath::Min(TNumericLimits<int32>::Max(), Existing->Count + Amount); Counts.Sort([](const FPRPlayerProfileTaggedCount& Left, const FPRPlayerProfileTaggedCount& Right) { return Left.Tag.ToString() < Right.Tag.ToString(); });
}

bool UPRPlayerProfileSubsystem::ConsumeUniqueId(const FGuid& Id) { if (!Id.IsValid() || RecentIds.Contains(Id)) return false; RecentIds.Add(Id); if (RecentIds.Num() > 256) RecentIds.RemoveAt(0); return true; }
void UPRPlayerProfileSubsystem::PublishProfileChange() { if (bHasSession) ProfileChanged.Broadcast(Snapshot); }

#if WITH_DEV_AUTOMATION_TESTS
void UPRPlayerProfileSubsystem::BeginProfileSessionForAutomation() { BeginProfileSession(); }
void UPRPlayerProfileSubsystem::InjectAbilityLifecycleForAutomation(const FPRAbilityLifecycleEvent& Event) { HandleAbilityLifecycle(Event); }
void UPRPlayerProfileSubsystem::InjectCombatEventForAutomation(const FPRCombatEvent& Event) { HandleCombatEvent(Event); }
void UPRPlayerProfileSubsystem::InjectQTEResultForAutomation(const FPRQTEResult& Result) { HandleQTEResult(Result); }
void UPRPlayerProfileSubsystem::InjectRelationshipChangedForAutomation(const FPRRelationshipChangedEvent& Event) { HandleRelationshipChanged(Event); }
void UPRPlayerProfileSubsystem::InjectDivergenceResultForAutomation(const FPRDivergenceResult& Result) { HandleDivergenceResult(Result); }
#endif
