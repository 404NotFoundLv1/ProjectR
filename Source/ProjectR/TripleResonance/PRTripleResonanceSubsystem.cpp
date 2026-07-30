// Copyright ProjectR. All Rights Reserved.

#include "TripleResonance/PRTripleResonanceSubsystem.h"

#include "Chapters/PRChapterSubsystem.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Chapters/Headmind/PRHeadmindProjectionBossComponent.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "Engine/World.h"
#include "QTE/PRQTESubsystem.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "TripleResonance/Abilities/PRGA_TripleResonance.h"
#include "TripleResonance/PRTripleResonanceDataAsset.h"
#include "TripleResonance/PRTripleResonanceRegistryDataAsset.h"
#include "UI/PRTripleResonanceLegacyWidget.h"
#include "UI/PRTripleResonanceOverlayWidget.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

void UPRTripleResonanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ChapterSubsystem = GetGameInstance()->GetSubsystem<UPRChapterSubsystem>();
	RunStateSubsystem = GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
	ProgressionSubsystem = GetGameInstance()->GetSubsystem<UPRProgressionSubsystem>();
	CompanionSubsystem = GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>();
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	if (ChapterSubsystem.IsValid()) ChapterStateHandle = ChapterSubsystem->OnStateChanged().AddUObject(this, &UPRTripleResonanceSubsystem::HandleChapterStateChanged);
	if (RunStateSubsystem.IsValid()) RunStateHandle = RunStateSubsystem->OnRunStateChanged().AddUObject(this, &UPRTripleResonanceSubsystem::HandleRunStateChanged);
	if (ProgressionSubsystem.IsValid()) ProgressionRunSnapshotHandle = ProgressionSubsystem->OnRunSnapshotChanged().AddUObject(this, &UPRTripleResonanceSubsystem::HandleProgressionRunSnapshotChanged);
	if (CompanionSubsystem.IsValid()) RelationshipHandle = CompanionSubsystem->OnRelationshipChanged().AddUObject(this, &UPRTripleResonanceSubsystem::HandleRelationshipChanged);
	if (RunStateSubsystem.IsValid()) AccountDeletedHandle = RunStateSubsystem->OnAccountDeleted().AddUObject(this, &UPRTripleResonanceSubsystem::HandleAccountDeleted);
	if (SaveSubsystem.IsValid()) SaveOperationHandle = SaveSubsystem->OnSaveOperation().AddUObject(this, &UPRTripleResonanceSubsystem::HandleSaveOperation);
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRTripleResonanceSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRTripleResonanceSubsystem::HandleWorldCleanup);
	FPRTripleResonancePersistenceData Persistence;
	if (SaveSubsystem.IsValid() && SaveSubsystem->GetTripleResonancePersistenceSnapshot(Persistence))
	{
		LegacySnapshot.bHasSkillMemory = Persistence.SkillMemory.IsValid();
		LegacySnapshot.AbilityTag = Persistence.SkillMemory.AbilityTag;
		LegacySnapshot.bHasHighRiskProof = Persistence.bHasHighRiskProof;
	}
	BindQTEBridge();
	RefreshEligibility();
}

void UPRTripleResonanceSubsystem::Deinitialize()
{
	if (ChapterSubsystem.IsValid()) ChapterSubsystem->OnStateChanged().Remove(ChapterStateHandle);
	if (RunStateSubsystem.IsValid()) RunStateSubsystem->OnRunStateChanged().Remove(RunStateHandle);
	if (ProgressionSubsystem.IsValid()) ProgressionSubsystem->OnRunSnapshotChanged().Remove(ProgressionRunSnapshotHandle);
	if (CompanionSubsystem.IsValid()) CompanionSubsystem->OnRelationshipChanged().Remove(RelationshipHandle);
	if (RunStateSubsystem.IsValid()) RunStateSubsystem->OnAccountDeleted().Remove(AccountDeletedHandle);
	if (SaveSubsystem.IsValid()) SaveSubsystem->OnSaveOperation().Remove(SaveOperationHandle);
	if (DeferredPersistenceHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(DeferredPersistenceHandle);
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	if (QTESubsystem.IsValid())
	{
		QTESubsystem->OnSemanticInput().Remove(QTESemanticInputHandle);
		QTESubsystem->OnQTEResult().Remove(QTEResultHandle);
	}
	ChapterStateHandle.Reset(); RunStateHandle.Reset(); ProgressionRunSnapshotHandle.Reset(); RelationshipHandle.Reset(); QTESemanticInputHandle.Reset(); QTEResultHandle.Reset(); AccountDeletedHandle.Reset(); SaveOperationHandle.Reset(); PostWorldInitializationHandle.Reset(); WorldCleanupHandle.Reset(); DeferredPersistenceHandle.Reset();
	ClearRuntimeState();
	ClearPresentation();
	ChapterSubsystem.Reset(); RunStateSubsystem.Reset(); ProgressionSubsystem.Reset(); CompanionSubsystem.Reset(); QTESubsystem.Reset(); SaveSubsystem.Reset();
	Super::Deinitialize();
}

bool UPRTripleResonanceSubsystem::GetSnapshot(FPRTripleResonanceSnapshot& OutSnapshot) const { OutSnapshot = Snapshot; return true; }
bool UPRTripleResonanceSubsystem::GetLatestResult(FPRTripleResonanceExecutionResult& OutResult) const { OutResult = LatestResult; return bHasLatestResult; }
bool UPRTripleResonanceSubsystem::GetLegacySnapshot(FPRTripleResonanceLegacySnapshot& OutSnapshot) const { OutSnapshot = LegacySnapshot; return LegacySnapshot.bHasSkillMemory || LegacySnapshot.bHasHighRiskProof; }
EPRTripleResonanceOperationResult UPRTripleResonanceSubsystem::RetryPendingPersistence()
{
	if (!bHasFrozenPersistence || Snapshot.State != EPRTripleResonanceState::ReadyToRetry || !SaveSubsystem.IsValid()
		|| !SaveSubsystem->GetTripleResonancePersistenceSnapshot(PendingExpectedPersistence))
	{
		PublishOperation(EPRTripleResonanceOperationResult::Rejected, TEXT("TripleResonance.NoPendingPersistence"));
		return EPRTripleResonanceOperationResult::Rejected;
	}
	if (!BeginLegacyPersistence())
	{
		PublishOperation(EPRTripleResonanceOperationResult::ReadyToRetry, TEXT("TripleResonance.PersistenceNotStarted"));
		return EPRTripleResonanceOperationResult::ReadyToRetry;
	}
	return EPRTripleResonanceOperationResult::Started;
}

void UPRTripleResonanceSubsystem::HandleChapterStateChanged(const FPRChapterSnapshot&) { RefreshEligibility(); }
void UPRTripleResonanceSubsystem::HandleProgressionRunSnapshotChanged(const FPRProgressionRunSnapshotChangedEvent&) { RefreshEligibility(); }
void UPRTripleResonanceSubsystem::HandleRelationshipChanged(const FPRRelationshipChangedEvent&) { RefreshEligibility(); }
void UPRTripleResonanceSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues)
{
	if (World && World->GetGameInstance() == GetGameInstance()) { BindQTEBridge(); RefreshEligibility(); EnsurePresentation(); }
}

void UPRTripleResonanceSubsystem::HandleWorldCleanup(UWorld* World, bool, bool)
{
	if (!World || !QTESubsystem.IsValid() || QTESubsystem->GetWorld() != World) return;
	QTESubsystem->OnSemanticInput().Remove(QTESemanticInputHandle);
	QTESubsystem->OnQTEResult().Remove(QTEResultHandle);
	QTESemanticInputHandle.Reset(); QTEResultHandle.Reset(); QTESubsystem.Reset();
	ActiveQTERequestId.Invalidate();
}
void UPRTripleResonanceSubsystem::HandleSemanticInput(const FGameplayTag InputTag, const double)
{
	if (Snapshot.State != EPRTripleResonanceState::Ready || !InputTag.MatchesTagExact(UPRTagLibrary::GetInputExecuteTag())) return;
	if (StartExternalStep(EPRTripleResonanceStep::Axiom)) PublishOperation(EPRTripleResonanceOperationResult::Started, TEXT("TripleResonance.SequenceStarted"));
}

void UPRTripleResonanceSubsystem::HandleQTEResult(const FPRQTEResult& Result)
{
	if (Snapshot.State != EPRTripleResonanceState::QTEActive || Result.RequestId != ActiveQTERequestId
		|| !FPRTripleResonanceContract::IsExternalQTEId(Result.QTEId.PrimaryAssetName)) return;
	ActiveQTERequestId.Invalidate();
	if (!Result.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag())) { FailSequence(TEXT("TripleResonance.QTEFailed")); return; }
	const FName CompletedId = Result.QTEId.PrimaryAssetName;
	if (CompletedId == TEXT("TripleResonance_Axiom")) { StartExternalStep(EPRTripleResonanceStep::Kindle); return; }
	if (CompletedId == TEXT("TripleResonance_Kindle")) { StartExternalStep(EPRTripleResonanceStep::Null); return; }
	if (CompletedId == TEXT("TripleResonance_Null"))
	{
		Snapshot.ActiveStep = EPRTripleResonanceStep::None;
		Snapshot.State = EPRTripleResonanceState::AbilityPending;
		PublishState();
		if (!GrantAndActivateTransientAbility()) FailSequence(TEXT("TripleResonance.AbilityUnavailable"));
		return;
	}
	FailSequence(TEXT("TripleResonance.InvalidQTEResult"));
}

APRHeadmindProjectionBoss* UPRTripleResonanceSubsystem::ResolveFrozenHeadmind() const
{
	if (!GetWorld() || !Snapshot.Eligibility.BossSpawnId.IsValid() || Snapshot.Eligibility.WorldId != GetWorld()->GetFName()) return nullptr;
	for (TActorIterator<APRHeadmindProjectionBoss> It(GetWorld()); It; ++It)
	{
		if (It->GetSpawnId() == Snapshot.Eligibility.BossSpawnId) return *It;
	}
	return nullptr;
}

bool UPRTripleResonanceSubsystem::CanExecuteGrantedAbility(const AActor* Avatar) const
{
	APRHeadmindProjectionBoss* Boss = ResolveFrozenHeadmind();
	const UPRAttributeSet* Attributes = Boss ? Boss->GetAttributeSet() : nullptr;
	return Avatar && Snapshot.State == EPRTripleResonanceState::AbilityPending && Boss && Attributes
		&& Attributes->GetMaxHealth() > 0.0f && Attributes->GetHealth() / Attributes->GetMaxHealth() <= 0.20f;
}

bool UPRTripleResonanceSubsystem::ExecuteGrantedAbility(AActor* Avatar)
{
	if (!CanExecuteGrantedAbility(Avatar)) return false;
	APRHeadmindProjectionBoss* Boss = ResolveFrozenHeadmind();
	UPRHeadmindProjectionBossComponent* Headmind = Boss ? Boss->GetHeadmindProjectionBossComponent() : nullptr;
	UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	if (!Boss || !Headmind || !Combat || !Headmind->TryAcceptTripleResonanceCounter(Snapshot.Eligibility)) return false;
	bHighRiskCandidate = Headmind->GetRuntimeState().SynthesisPressure == 4;
	Snapshot.State = EPRTripleResonanceState::Executing;
	PublishState();
	FPRDamageRequest Request;
	Request.SourceId = TEXT("TripleResonance"); Request.DamageSource = this; Request.Instigator = Avatar; Request.Target = Boss;
	Request.AbilityTag = UPRTagLibrary::GetSkillTripleResonanceTag(); Request.RawDamage = 120.0f;
	Request.ImpactOrigin = Avatar->GetActorLocation(); Request.IncomingDirection = (Boss->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal();
	const EPRCombatRequestStatus Status = Combat->ApplyDamage(Request);
	if (Status != EPRCombatRequestStatus::Applied) { FailSequence(TEXT("TripleResonance.CombatRejected")); return false; }
	LatestResult.RunId = Snapshot.Eligibility.RunId; LatestResult.AccountId = Snapshot.Eligibility.AccountId; LatestResult.BossSpawnId = Snapshot.Eligibility.BossSpawnId;
	LatestResult.bBasiliskCountered = true; LatestResult.bExecuted = true; LatestResult.AppliedDamage = 120.0f;
	bHasLatestResult = true;
	Snapshot.State = EPRTripleResonanceState::Resolved;
	PublishState(); Resolved.Broadcast(LatestResult); PublishOperation(EPRTripleResonanceOperationResult::Succeeded, TEXT("TripleResonance.Resolved"));
	return true;
}

bool UPRTripleResonanceSubsystem::GrantAndActivateTransientAbility()
{
	AActor* Avatar = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	IAbilitySystemInterface* Interface = Avatar ? Cast<IAbilitySystemInterface>(Avatar) : nullptr;
	UPRAbilitySystemComponent* ASC = Interface ? Cast<UPRAbilitySystemComponent>(Interface->GetAbilitySystemComponent()) : nullptr;
	if (!ASC) return false;
	const UPRTripleResonanceRegistryDataAsset* Registry = LoadObject<UPRTripleResonanceRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/TripleResonance/Data/DA_TripleResonanceRegistry.DA_TripleResonanceRegistry"));
	const UPRTripleResonanceDataAsset* Definition = Registry ? Registry->Definition.LoadSynchronous() : nullptr;
	UClass* AbilityClass = Definition ? Definition->AbilityClass.LoadSynchronous() : nullptr;
	if (!AbilityClass) return false;
	TransientAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	if (!TransientAbilityHandle.IsValid()) return false;
	const bool bActivated = ASC->TryActivateAbility(TransientAbilityHandle);
	ASC->ClearAbility(TransientAbilityHandle);
	TransientAbilityHandle = FGameplayAbilitySpecHandle();
	return bActivated;
}
void UPRTripleResonanceSubsystem::HandleRunStateChanged(const FPRRunRuntimeState& InState)
{
	if (InState.State != EPRRunLifecycleState::RunActive) ClearRuntimeState();
	else RefreshEligibility();
}

void UPRTripleResonanceSubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	if (!bHasLatestResult || !LatestResult.bExecuted || bHasFrozenPersistence || Event.Record.Summary.RunId != LatestResult.RunId
		|| Event.Record.Summary.AccountId != LatestResult.AccountId || Event.Record.GraveyardOrdinal <= 0) return;
	if (!SaveSubsystem.IsValid() || !SaveSubsystem->GetTripleResonancePersistenceSnapshot(PendingExpectedPersistence)) return;
	if (Event.Record.GraveyardOrdinal <= PendingExpectedPersistence.LastProcessedGraveyardOrdinal) return;
	PendingTargetPersistence = PendingExpectedPersistence;
	PendingTargetPersistence.LastProcessedGraveyardOrdinal = Event.Record.GraveyardOrdinal;
	const FGameplayTag SelectedSkill = FPRTripleResonancePersistenceContract::SelectLegacySkillMemory(Event.Record.Summary.SkillSummaries);
	if (SelectedSkill.IsValid() && !PendingTargetPersistence.SkillMemory.IsValid())
	{
		PendingTargetPersistence.SkillMemory.SourceSummaryId = Event.Record.RecordId;
		PendingTargetPersistence.SkillMemory.AbilityTag = SelectedSkill;
		PendingTargetPersistence.SkillMemory.GraveyardOrdinal = Event.Record.GraveyardOrdinal;
		PendingTargetPersistence.SkillMemory.LegacySequence = PendingExpectedPersistence.LegacySequence + 1;
		PendingTargetPersistence.LegacySequence = PendingTargetPersistence.SkillMemory.LegacySequence;
	}
	const bool bHighRisk = bHighRiskCandidate && Event.Record.TerminationReason == EPRAccountTerminationReason::RoomSequenceCompleted
		&& Event.Record.Summary.bBossCompleted && Event.Record.Summary.RoomIds.Contains(UPRChapterContentRegistryDataAsset::GetHeadmindFinalRoomId())
		&& Event.Record.Summary.MinimumHealthRatio <= 0.25f;
	if (bHighRisk && !PendingTargetPersistence.bHasHighRiskProof)
	{
		PendingTargetPersistence.bHasHighRiskProof = true;
		PendingTargetPersistence.HighRiskProofSequence = PendingTargetPersistence.LegacySequence + 1;
	}
	FPRTripleResonancePersistenceContract::Normalize(PendingTargetPersistence);
	if (!FPRTripleResonancePersistenceContract::IsCanonical(PendingTargetPersistence)) return;
	bHasFrozenPersistence = true;
	if (!BeginLegacyPersistence())
	{
		Snapshot.State = EPRTripleResonanceState::ReadyToRetry; Snapshot.FailureReason = TEXT("TripleResonance.PersistenceNotStarted"); PublishState();
	}
}

void UPRTripleResonanceSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (!bHasFrozenPersistence || Event.RequestId != PendingSaveRequestId) return;
	PendingSaveRequestId.Invalidate();
	FPRTripleResonancePersistenceData Verified;
	if (Event.Result != EPRSaveResult::Success || !SaveSubsystem.IsValid() || !SaveSubsystem->GetTripleResonancePersistenceSnapshot(Verified)
		|| !FPRTripleResonancePersistenceData::StaticStruct()->CompareScriptStruct(&Verified, &PendingTargetPersistence, 0))
	{
		Snapshot.State = EPRTripleResonanceState::ReadyToRetry; Snapshot.FailureReason = TEXT("TripleResonance.PersistenceFailed"); PublishState();
		PublishOperation(EPRTripleResonanceOperationResult::ReadyToRetry, Snapshot.FailureReason); return;
	}
	LegacySnapshot.bHasSkillMemory = PendingTargetPersistence.SkillMemory.IsValid();
	LegacySnapshot.AbilityTag = PendingTargetPersistence.SkillMemory.AbilityTag;
	LegacySnapshot.bHasHighRiskProof = PendingTargetPersistence.bHasHighRiskProof;
	bHasFrozenPersistence = false;
	PublishOperation(EPRTripleResonanceOperationResult::Succeeded, TEXT("TripleResonance.PersistenceVerified"));
}

bool UPRTripleResonanceSubsystem::HandleDeferredPersistence(float)
{
	if (!bHasFrozenPersistence || !SaveSubsystem.IsValid()) return false;
	if (SaveSubsystem->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready) return true;
	DeferredPersistenceHandle.Reset();
	BeginLegacyPersistence();
	return false;
}

bool UPRTripleResonanceSubsystem::BeginLegacyPersistence()
{
	if (!SaveSubsystem.IsValid()) return false;
	if (SaveSubsystem->GetSaveRuntimeState().State == EPRSaveSubsystemState::Saving)
	{
		if (!DeferredPersistenceHandle.IsValid()) DeferredPersistenceHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPRTripleResonanceSubsystem::HandleDeferredPersistence), 0.0f);
		return true;
	}
	if (!SaveSubsystem->StageTripleResonancePersistenceTransaction(PendingExpectedPersistence, PendingTargetPersistence)
		|| SaveSubsystem->RequestSaveCurrentProfile(PendingSaveRequestId) != EPRSaveRequestStatus::Started) return false;
	Snapshot.State = EPRTripleResonanceState::Executing; PublishState();
	return true;
}

void UPRTripleResonanceSubsystem::RefreshEligibility()
{
	BindQTEBridge();
	if (bHasTriggered || (Snapshot.State != EPRTripleResonanceState::Unavailable && Snapshot.State != EPRTripleResonanceState::Ready)) return;
	FPRChapterSnapshot Chapter;
	FPRProgressionRunSnapshot Progression;
	if (!ChapterSubsystem.IsValid() || !RunStateSubsystem.IsValid() || !ProgressionSubsystem.IsValid() || !CompanionSubsystem.IsValid()
		|| !ChapterSubsystem->GetSnapshot(Chapter) || !ProgressionSubsystem->GetRunSnapshot(Progression))
	{
		Snapshot = FPRTripleResonanceSnapshot(); Snapshot.FailureReason = TEXT("TripleResonance.DependencyUnavailable"); PublishState(); return;
	}
	const FPRRunRuntimeState Run = RunStateSubsystem->GetRunRuntimeState();
	FPRTripleResonanceEligibilityInput Input;
	Input.Opportunity = Chapter.HeadmindBoss.TripleResonance;
	Input.bHasAuditorPrerequisite = Chapter.bHasTripleResonancePrerequisite;
	Input.bHasFrozenRunEntitlement = Progression.EntitlementIds.Contains(FPrimaryAssetId(TEXT("ProgressionNode"), TEXT("BondTripleResonance")));
	Input.bHasExactHeadmindIdentity = Run.State == EPRRunLifecycleState::RunActive
		&& Input.Opportunity.FrozenRunId == Run.RunId && Input.Opportunity.FrozenAccountId == Run.AccountId
		&& GetWorld() && Input.Opportunity.FrozenWorldId == FName(*GetWorld()->GetName());
	Input.RunId = Input.Opportunity.FrozenRunId;
	Input.AccountId = Input.Opportunity.FrozenAccountId;
	Input.BossSpawnId = Input.Opportunity.FrozenBossSpawnId;
	Input.WorldId = Input.Opportunity.FrozenWorldId;
	CompanionSubsystem->GetAllRelationshipSnapshots(Input.Relationships);
	Snapshot.Eligibility = FPRTripleResonanceEligibilityRules::Evaluate(Input);
	Snapshot.FailureReason = Snapshot.Eligibility.FailureReason;
	Snapshot.State = Snapshot.Eligibility.bEligible ? EPRTripleResonanceState::Ready : EPRTripleResonanceState::Unavailable;
	PublishState();
}

void UPRTripleResonanceSubsystem::BindQTEBridge()
{
	if (QTESubsystem.IsValid()) return;
	if (UWorld* World = GetWorld())
	{
		if (UPRQTESubsystem* QTE = World->GetSubsystem<UPRQTESubsystem>())
		{
			QTESubsystem = QTE;
			QTESemanticInputHandle = QTE->OnSemanticInput().AddUObject(this, &UPRTripleResonanceSubsystem::HandleSemanticInput);
			QTEResultHandle = QTE->OnQTEResult().AddUObject(this, &UPRTripleResonanceSubsystem::HandleQTEResult);
		}
	}
}

bool UPRTripleResonanceSubsystem::StartExternalStep(const EPRTripleResonanceStep Step)
{
	if (!QTESubsystem.IsValid() || (Step == EPRTripleResonanceStep::Axiom && bHasTriggered)) return false;
	const FPRTripleResonanceQTEStepDefinition Definition = FPRTripleResonanceContract::GetStepDefinition(Step);
	if (!Definition.QTEId.IsValid()) return false;
	const FGuid RequestId = FGuid::NewGuid();
	if (!QTESubsystem->StartExternalResultOnlyQTE(Definition.QTEId, RequestId)) return false;
	if (Step == EPRTripleResonanceStep::Axiom) bHasTriggered = true;
	ActiveQTERequestId = RequestId;
	Snapshot.State = EPRTripleResonanceState::QTEActive;
	Snapshot.ActiveStep = Step;
	Snapshot.RemainingSeconds = Definition.WindowSeconds;
	Snapshot.FailureReason = NAME_None;
	PublishState();
	return true;
}

void UPRTripleResonanceSubsystem::FailSequence(const FName Reason)
{
	ActiveQTERequestId.Invalidate();
	Snapshot.ActiveStep = EPRTripleResonanceStep::None;
	Snapshot.State = EPRTripleResonanceState::Failed;
	Snapshot.FailureReason = Reason;
	PublishState();
	PublishOperation(EPRTripleResonanceOperationResult::Failed, Reason);
}

void UPRTripleResonanceSubsystem::PublishState() { EnsurePresentation(); StateChanged.Broadcast(Snapshot); }
void UPRTripleResonanceSubsystem::PublishOperation(const EPRTripleResonanceOperationResult Result, const FName Reason)
{
	FPRTripleResonanceOperationEvent Event; Event.Result = Result; Event.Reason = Reason; Operation.Broadcast(Event);
}

void UPRTripleResonanceSubsystem::EnsurePresentation()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController()) return;
	if (!OverlayWidget.IsValid() && Snapshot.Eligibility.BossSpawnId.IsValid())
	{
		const TSubclassOf<UPRTripleResonanceOverlayWidget> Class = LoadClass<UPRTripleResonanceOverlayWidget>(nullptr,
			TEXT("/Game/ProjectR/UI/TripleResonance/WBP_TripleResonanceOverlay.WBP_TripleResonanceOverlay_C"));
		if (Class)
		{
			OverlayWidget = CreateWidget<UPRTripleResonanceOverlayWidget>(PlayerController, Class);
			if (OverlayWidget.IsValid()) OverlayWidget->AddToViewport(100);
		}
	}
	if (!LegacyWidget.IsValid() && (LegacySnapshot.bHasSkillMemory || LegacySnapshot.bHasHighRiskProof))
	{
		const TSubclassOf<UPRTripleResonanceLegacyWidget> Class = LoadClass<UPRTripleResonanceLegacyWidget>(nullptr,
			TEXT("/Game/ProjectR/UI/TripleResonance/WBP_TripleResonanceLegacySummary.WBP_TripleResonanceLegacySummary_C"));
		if (Class)
		{
			LegacyWidget = CreateWidget<UPRTripleResonanceLegacyWidget>(PlayerController, Class);
			if (LegacyWidget.IsValid()) LegacyWidget->AddToViewport(90);
		}
	}
}

void UPRTripleResonanceSubsystem::ClearPresentation()
{
	if (OverlayWidget.IsValid()) OverlayWidget->RemoveFromParent();
	if (LegacyWidget.IsValid()) LegacyWidget->RemoveFromParent();
	OverlayWidget.Reset();
	LegacyWidget.Reset();
}
void UPRTripleResonanceSubsystem::ClearRuntimeState()
{
	if (QTESubsystem.IsValid() && ActiveQTERequestId.IsValid() && QTESubsystem->GetRuntimeState().RequestId == ActiveQTERequestId) QTESubsystem->CancelActiveQTE();
	ActiveQTERequestId.Invalidate();
	if (TransientAbilityHandle.IsValid())
	{
		if (AActor* Avatar = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
		{
			if (IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Avatar)) Interface->GetAbilitySystemComponent()->ClearAbility(TransientAbilityHandle);
		}
		TransientAbilityHandle = FGameplayAbilitySpecHandle();
	}
	if (OverlayWidget.IsValid())
	{
		OverlayWidget->RemoveFromParent();
		OverlayWidget.Reset();
	}
	Snapshot = FPRTripleResonanceSnapshot(); bHasTriggered = false; PublishState();
}
