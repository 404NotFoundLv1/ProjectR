// Copyright ProjectR. All Rights Reserved.

#include "PRAuditorAutomationToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "Async/Async.h"
#include "Chapters/Auditor/PRAuditorChapterBoss.h"
#include "Chapters/Auditor/PRAuditorChapterBossComponent.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Containers/Ticker.h"
#include "Core/PRTagLibrary.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "Save/PRAccountSaveTypes.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRAuditorAutomationPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
constexpr double TimeoutSeconds = 90.0;
const TCHAR* RegistryPath = TEXT("/Game/ProjectR/Chapters/Auditor/DA_RoguelikeContentRegistry_Auditor.DA_RoguelikeContentRegistry_Auditor");

class FFixedStorage final : public IPRSaveStorageBackend
{
public:
	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override { return Slots.Contains(Slot) ? ISaveGameSystem::ESaveExistsResult::OK : ISaveGameSystem::ESaveExistsResult::DoesNotExist; }
	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override { const TArray<uint8>* Data = Slots.Find(Slot); if (!Data) return false; OutData = *Data; return true; }
	virtual void SaveGameAsync(const FString& Slot, TSharedRef<const TArray<uint8>> Data, TFunction<void(bool)> Completion) override { const bool bFail = SavesUntilFailure == 1; if (SavesUntilFailure > 0) --SavesUntilFailure; if (!bFail) Slots.Add(Slot, *Data); AsyncTask(ENamedThreads::GameThread, [bSuccess = !bFail, Completion = MoveTemp(Completion)]() mutable { Completion(bSuccess); }); }
	virtual void LoadGameAsync(const FString& Slot, TFunction<void(bool, TArray<uint8>)> Completion) override { TArray<uint8> Data; const bool bSuccess = LoadGame(Slot, Data); AsyncTask(ENamedThreads::GameThread, [bSuccess, Data = MoveTemp(Data), Completion = MoveTemp(Completion)]() mutable { Completion(bSuccess, MoveTemp(Data)); }); }
	virtual bool DeleteGame(const FString& Slot) override { return Slots.Remove(Slot) > 0; }
	void FailNthUpcomingSave(const int32 Ordinal) { SavesUntilFailure = FMath::Max(1, Ordinal); }
private:
	TMap<FString, TArray<uint8>> Slots;
	int32 SavesUntilFailure = INDEX_NONE;
};

TSharedPtr<FFixedStorage> Storage;
enum class EBootstrapPhase : uint8 { CreateProfile, StageProofs, CreateAccount, StartRun, Ready };

struct FBootstrap
{
	EPRAccountOperationResult Tick(const int32 Seed, FString& OutFailure)
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UPRSaveSubsystem* Save = GI ? GI->GetSubsystem<UPRSaveSubsystem>() : nullptr;
		UPRRunStateSubsystem* Run = GI ? GI->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!World || World->GetNetMode() == NM_Client || !Save || !Run || !Chapter) { OutFailure = TEXT("Auditor fixed fixture requires authoritative PIE and its three subsystems."); return EPRAccountOperationResult::RejectedInvalidState; }
		switch (Phase)
		{
		case EBootstrapPhase::CreateProfile:
			if (!bIssued) { FGuid Id; if (Save->CreateNewDefaultProfile(Id) != EPRSaveResult::Success) { OutFailure = TEXT("Could not create isolated Auditor profile."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			Phase = EBootstrapPhase::CreateAccount; bIssued = false; return EPRAccountOperationResult::Started;
		case EBootstrapPhase::StageProofs:
			if (!bIssued) { if (!Chapter->StageFixedAuditorPrerequisitesForAutomation()) { OutFailure = TEXT("Could not persist fixed Allocator/Warden/Pacifier prerequisites."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready || Save->GetSaveRuntimeState().bSaveRequestQueued) return EPRAccountOperationResult::Started;
			if (!Chapter->RefreshFixedAuditorSelectionForAutomation()) { OutFailure = TEXT("Could not refresh the fixed Auditor selection after prerequisite persistence."); return EPRAccountOperationResult::RejectedInvalidState; }
			Phase = EBootstrapPhase::StartRun; bIssued = false; return EPRAccountOperationResult::Started;
		case EBootstrapPhase::CreateAccount:
			if (!bIssued)
			{
				FPRProfileSaveData Profile;
				if (!Save->GetLoadedProfileSnapshot(Profile)) { OutFailure = TEXT("Auditor fixture lost its loaded Profile before account creation."); return EPRAccountOperationResult::RejectedInvalidState; }
				if (!FPRAccountPersistenceContract::IsCanonical(Profile.AccountPersistence)) { OutFailure = TEXT("Auditor fixture Profile has a non-canonical Account partition after Chapter prerequisite staging."); return EPRAccountOperationResult::RejectedInvalidState; }
				FGuid Id;
				const EPRAccountOperationResult CreateResult = Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), Id);
				if (CreateResult != EPRAccountOperationResult::Started) { OutFailure = FString::Printf(TEXT("Auditor fixture account creation was rejected (%d)."), static_cast<int32>(CreateResult)); return EPRAccountOperationResult::RejectedInvalidState; }
				bIssued = true;
			}
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed)
			{
				FGuid Id;
				if (Run->RetryPendingPersistence(Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Auditor fixture account persistence retry was rejected."); return EPRAccountOperationResult::RejectedInvalidState; }
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady) return EPRAccountOperationResult::Started;
			Phase = EBootstrapPhase::StageProofs; bIssued = false; return EPRAccountOperationResult::Started;
		case EBootstrapPhase::StartRun:
			if (!bIssued) { FGuid Id; if (Run->RequestStartRun(Seed, Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Auditor fixture run failed to start."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed)
			{
				FGuid Id;
				if (Run->RetryPendingPersistence(Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Auditor fixture run persistence retry was rejected."); return EPRAccountOperationResult::RejectedInvalidState; }
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive) return EPRAccountOperationResult::Started;
			Phase = EBootstrapPhase::Ready; return EPRAccountOperationResult::Succeeded;
		case EBootstrapPhase::Ready: return EPRAccountOperationResult::Succeeded;
		}
		OutFailure = TEXT("Auditor fixture entered an invalid bootstrap phase."); return EPRAccountOperationResult::RejectedInvalidState;
	}
	EBootstrapPhase Phase = EBootstrapPhase::CreateProfile;
	bool bIssued = false;
};

class FSelectionRunner final : public TSharedFromThis<FSelectionRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FSelectionRunner> Runner = MakeShared<FSelectionRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride()) { Runner->Result->SetError(TEXT("PrepareFixedAuditorPIE must run before Auditor PIE.")); return Runner->Result.Get(); }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); }));
		return Runner->Result.Get();
	}
private:
	bool Tick()
	{
		if (FPlatformTime::Seconds() - Started > TimeoutSeconds) return Fail(TEXT("Fixed Auditor selection timed out."));
		FString Failure; const EPRAccountOperationResult Step = Bootstrap.Tick(61301, Failure);
		if (Step == EPRAccountOperationResult::Started) return true;
		if (Step != EPRAccountOperationResult::Succeeded) return Fail(Failure);
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr; UPRRoomSubsystem* Room = GI ? GI->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		FPRChapterSnapshot ChapterState; FPRRoomRuntimeState RoomState;
		if (!Chapter || !Room || !Chapter->GetSnapshot(ChapterState) || !Room->GetRoomRuntimeState(RoomState)) return Fail(TEXT("Auditor selection lost public snapshots."));
		if (ChapterState.ContentId != UPRChapterContentRegistryDataAsset::GetAuditorContentId() || ChapterState.DirectiveId != TEXT("Auditor.DistanceAudit") || ChapterState.AuditPressure != 0 || RoomState.Seed != 61301 || RoomState.PathLength != 7 || RoomState.Path.Num() != 7 || RoomState.FlowStatus != EPRRoomFlowStatus::SelectingRoom || Room->GetConfiguredContentRegistryId() != UPRChapterContentRegistryDataAsset::GetAuditorRoomRegistryId()) return Fail(TEXT("Prerequisite proofs did not select fixed Seed-61301 Auditor closure."));
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61301,\"pathLength\":7,\"content\":\"Auditor\",\"directive\":\"Auditor.DistanceAudit\",\"auditPressure\":0,\"userSlotsTouched\":false}")); return false;
	}
	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; double Started = FPlatformTime::Seconds();
};

class FSettlementRunner final : public TSharedFromThis<FSettlementRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FSettlementRunner> Runner = MakeShared<FSettlementRunner>(); Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride() || !Storage.IsValid()) { Runner->Result->SetError(TEXT("PrepareFixedAuditorPIE must run before Auditor settlement PIE.")); return Runner->Result.Get(); }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); })); return Runner->Result.Get();
	}
private:
	enum class EPhase : uint8 { Bootstrap, Finalize, AwaitFailure, Retry, AwaitSuccess, Verify };
	bool Tick()
	{
		if (FPlatformTime::Seconds() - Started > TimeoutSeconds) return Fail(TEXT("Fixed Auditor settlement timed out."));
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr; UPRRunStateSubsystem* Run = GI ? GI->GetSubsystem<UPRRunStateSubsystem>() : nullptr; UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!Run || !Chapter) return Fail(TEXT("Auditor settlement lost RunState or Chapter subsystem."));
		if (Phase == EPhase::Bootstrap) { FString Failure; const EPRAccountOperationResult Step = Bootstrap.Tick(61304, Failure); if (Step == EPRAccountOperationResult::Started) return true; if (Step != EPRAccountOperationResult::Succeeded) return Fail(Failure); Phase = EPhase::Finalize; return true; }
		if (Phase == EPhase::Finalize) { if (!Chapter->StageFixedAuditorCompletionFactsForAutomation()) return Fail(TEXT("Could not stage Auditor completion facts.")); if (!bArmed) { Storage->FailNthUpcomingSave(3); bArmed = true; } Run->FinalizeActiveAccountForAutomation(EPRAccountTerminationReason::RoomSequenceCompleted); Phase = EPhase::AwaitFailure; return true; }
		FPRChapterSnapshot State; if (!Chapter->GetSnapshot(State)) return Fail(TEXT("Auditor settlement lost its snapshot."));
		if (Phase == EPhase::AwaitFailure) { if (State.State != EPRChapterLifecycleState::ReadyToRetry) return true; if (State.bHasHumanAnomalyProof) return Fail(TEXT("Failed Auditor write published a proof.")); Phase = EPhase::Retry; return true; }
		if (Phase == EPhase::Retry) { if (Chapter->RetryPendingSettlement() != EPRChapterOperationResult::Succeeded) return Fail(TEXT("Frozen Auditor retry did not start.")); Phase = EPhase::AwaitSuccess; return true; }
		if (Phase == EPhase::AwaitSuccess) { if (State.State != EPRChapterLifecycleState::Completed) return true; FPRChapterCompletionResult Completion; if (!State.bHasHumanAnomalyProof || !Chapter->GetLatestCompletion(Completion) || Completion.ProofId != UPRChapterContentRegistryDataAsset::GetAuditorProofId() || Completion.SettlementSequence != 4) return Fail(TEXT("Auditor retry did not publish one canonical proof.")); Phase = EPhase::Verify; return true; }
		if (Chapter->RetryPendingSettlement() != EPRChapterOperationResult::RejectedInvalidState) return Fail(TEXT("Duplicate Auditor retry was accepted.")); Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61304,\"firstSave\":\"WriteFailed\",\"stateAfterFailure\":\"ReadyToRetry\",\"retry\":\"PASS\",\"proof\":\"HumanAnomalyProof.Auditor\",\"settlementSequence\":4,\"duplicateRetry\":\"RejectedInvalidState\",\"userSlotsTouched\":false}")); return false;
	}
	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; EPhase Phase = EPhase::Bootstrap; bool bArmed = false; double Started = FPlatformTime::Seconds();
};

/**
 * Exercises the real closed Auditor content path.  It deliberately uses the
 * public room choices and Combat damage boundary: no completion facts are
 * staged and no private chapter state is written.
 */
class FFullPathRunner final : public TSharedFromThis<FFullPathRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFullPathRunner> Runner = MakeShared<FFullPathRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedAuditorPIE must run before fixed full-path PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); }));
		return Runner->Result.Get();
	}

	~FFullPathRunner()
	{
		if (UPRRoomSubsystem* Room = BoundRoom.Get()) Room->OnRewardOfferChanged().Remove(RewardOfferHandle);
	}

private:
	enum class EBossStep : uint8 { KillVariants, EnterRepeatedAudit, CounterRepeatedAudit, EnterVerdict, CounterVerdict, Defeat, AwaitCompletion };

	bool Tick()
	{
		if (FPlatformTime::Seconds() - StartedAt > 150.0)
		{
			FPRRoomRuntimeState TimeoutState;
			if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
			{
				if (UGameInstance* GI = World->GetGameInstance()) if (UPRRoomSubsystem* Room = GI->GetSubsystem<UPRRoomSubsystem>()) Room->GetRoomRuntimeState(TimeoutState);
			}
			return Fail(FString::Printf(TEXT("Fixed Auditor full path timed out (flow=%d,step=%d,bossStep=%d)."), static_cast<int32>(TimeoutState.FlowStatus), TimeoutState.CurrentStepIndex, static_cast<int32>(BossStep)));
		}
		FString Failure;
		const EPRAccountOperationResult BootstrapResult = Bootstrap.Tick(61302, Failure);
		if (BootstrapResult == EPRAccountOperationResult::Started) return true;
		if (BootstrapResult != EPRAccountOperationResult::Succeeded) return Fail(Failure);
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UPRRoomSubsystem* Room = GI ? GI->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!World || !Room || !Chapter) return Fail(TEXT("Fixed Auditor full path lost World, Room, or Chapter authority."));
		BindRewardOffer(*Room);
		FPRRoomRuntimeState State;
		if (!Room->GetRoomRuntimeState(State)) return Fail(TEXT("Fixed Auditor full path could not read Room state."));
		if (State.FlowStatus == EPRRoomFlowStatus::Cancelled) return Fail(FString::Printf(TEXT("Fixed Auditor room flow cancelled at step %d (%s)."), State.CurrentStepIndex, *State.ActiveRoomId.ToString()));
		if (State.FlowStatus == EPRRoomFlowStatus::Completed)
		{
			FPRChapterSnapshot Snapshot;
			FPRChapterCompletionResult Completion;
			if (!Chapter->GetSnapshot(Snapshot) || Snapshot.State != EPRChapterLifecycleState::Completed) return true;
			if (!Snapshot.bHasHumanAnomalyProof || !Chapter->GetLatestCompletion(Completion)
				|| Completion.ProofId != UPRChapterContentRegistryDataAsset::GetAuditorProofId() || Completion.SettlementSequence != 4)
				return Fail(TEXT("Fixed Auditor full path did not durably publish its one canonical proof."));
			if (!bSawCombat || !bSawEvent || !bSawShop || !bSawSafe || !bSawBoss || !bSawThreeVariants || !bRepeatedCountered || !bVerdictCountered || !bBossCompletedOnce)
				return Fail(TEXT("Fixed Seed-61302 Auditor path did not cover every required acceptance fact."));
			Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"seed\":61302,\"combat\":true,\"event\":true,\"shopNoEconomy\":true,\"safe\":true,\"threeEnemyVariants\":true,\"repeatedBuildCountered\":true,\"verdictCountered\":true,\"bossCompletionCount\":1,\"proof\":\"HumanAnomalyProof.Auditor\",\"settlementSequence\":4,\"auditPressure\":%d,\"userSlotsTouched\":false}"), MaxAuditPressure));
			return false;
		}
		switch (State.FlowStatus)
		{
		case EPRRoomFlowStatus::SelectingRoom: return SelectNextRoom(*Room, State);
		case EPRRoomFlowStatus::SelectingEvent: return ResolveEvent(*Room, State);
		case EPRRoomFlowStatus::SelectingReward: return ResolveReward(*Room, State);
		case EPRRoomFlowStatus::EncounterActive: return DriveEncounter(*World, *Room);
		default: return true;
		}
	}

	void BindRewardOffer(UPRRoomSubsystem& Room)
	{
		if (BoundRoom.Get() == &Room && RewardOfferHandle.IsValid()) return;
		if (UPRRoomSubsystem* Previous = BoundRoom.Get()) Previous->OnRewardOfferChanged().Remove(RewardOfferHandle);
		BoundRoom = &Room;
		RewardOfferHandle = Room.OnRewardOfferChanged().AddLambda([WeakThis = TWeakPtr<FFullPathRunner>(AsShared())](const FPRRewardOffer& Offer)
		{
			if (const TSharedPtr<FFullPathRunner> This = WeakThis.Pin()) This->LatestOffer = Offer;
		});
	}

	bool SelectNextRoom(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const int32 Next = State.CurrentStepIndex + 1;
		if (!State.Path.IsValidIndex(Next) || State.Path[Next].CandidateRoomIds.IsEmpty()) return Fail(TEXT("Fixed Auditor path exposed no candidate."));
		const FPrimaryAssetId Previous = State.Path.IsValidIndex(State.CurrentStepIndex) ? State.Path[State.CurrentStepIndex].SelectedRoomId : FPrimaryAssetId();
		const FPRRoomPathStep* FollowingStep = State.Path.IsValidIndex(Next + 1) ? &State.Path[Next + 1] : nullptr;
		const FPrimaryAssetId* Selected = State.Path[Next].CandidateRoomIds.FindByPredicate(
			[Previous, FollowingStep](const FPrimaryAssetId& Candidate)
			{
				if (Candidate == Previous) return false;
				// A legal player choice must leave at least one non-repeating candidate
				// at the following step.  The production Room contract rejects an
				// immediate repeat, so the fixed verifier makes that choice explicit.
				return !FollowingStep || FollowingStep->CandidateRoomIds.ContainsByPredicate(
					[Candidate](const FPrimaryAssetId& Following) { return Following != Candidate; });
			});
		if (!Selected)
		{
			FString Candidates;
			for (const FPrimaryAssetId& Candidate : State.Path[Next].CandidateRoomIds)
			{
				if (!Candidates.IsEmpty()) Candidates += TEXT(",");
				Candidates += Candidate.ToString();
			}
			return Fail(FString::Printf(TEXT("Fixed Auditor path only exposed the immediately previous room (step=%d,previous=%s,candidates=[%s])."), Next, *Previous.ToString(), *Candidates));
		}
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, RegistryPath);
		const UPRRoomDataAsset* Definition = Registry ? Registry->FindRoom(*Selected) : nullptr;
		if (!Definition) return Fail(TEXT("Auditor candidate escaped the closed registry."));
		const FString Type = Definition->TypeTag.ToString();
		bSawCombat |= Type == TEXT("Room.Type.Combat") || Type == TEXT("Room.Type.Elite");
		bSawEvent |= Type == TEXT("Room.Type.Event"); bSawShop |= Type == TEXT("Room.Type.Shop"); bSawSafe |= Type == TEXT("Room.Type.Safe"); bSawBoss |= Type == TEXT("Room.Type.Boss");
		LatestOffer = FPRRewardOffer();
		return Room.SelectRoom(*Selected) == EPRRoomOperationResult::Succeeded ? true : Fail(TEXT("Fixed Auditor candidate was rejected."));
	}

	bool ResolveEvent(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, RegistryPath);
		const FPrimaryAssetId EventId = Registry ? Registry->FindEventForRoom(State.ActiveRoomId) : FPrimaryAssetId();
		const UPRRoomEventDataAsset* Event = Registry ? Registry->FindEvent(EventId) : nullptr;
		if (!Event || Event->Choices.IsEmpty()) return Fail(TEXT("Fixed Auditor Event has no closed choice set."));
		FName ChoiceId = Event->Choices[0].ChoiceId;
		int32 BestDelta = MIN_int32;
		for (const FPRRoomEventChoice& Choice : Event->Choices) { int32 Delta = 0; if (Registry->FindPressureDelta(EventId, Choice.ChoiceId, Delta) && Delta > BestDelta) { BestDelta = Delta; ChoiceId = Choice.ChoiceId; } }
		if (Room.SelectEventChoice(ChoiceId) != EPRRoomOperationResult::Succeeded) return Fail(TEXT("Fixed Auditor Event choice was rejected."));
		if (UPRChapterSubsystem* Chapter = Room.GetGameInstance() ? Room.GetGameInstance()->GetSubsystem<UPRChapterSubsystem>() : nullptr) { FPRChapterSnapshot Snapshot; if (Chapter->GetSnapshot(Snapshot)) MaxAuditPressure = FMath::Max(MaxAuditPressure, Snapshot.AuditPressure); }
		return true;
	}

	bool ResolveReward(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		if (!State.ActiveRewardOfferId.IsValid() || LatestOffer.OfferId != State.ActiveRewardOfferId || LatestOffer.Choices.Num() != 3 || LatestOffer.bResolved) return true;
		FGuid HandleId;
		return Room.SelectReward(LatestOffer.Choices[0].RewardId, HandleId) == EPRRoomOperationResult::Succeeded && HandleId.IsValid()
			? true : Fail(TEXT("Fixed Auditor three-choice reward was rejected."));
	}

	bool DriveEncounter(UWorld& World, UPRRoomSubsystem& Room)
	{
		UPREnemySubsystem* Enemies = World.GetSubsystem<UPREnemySubsystem>();
		UPRCombatSubsystem* Combat = World.GetSubsystem<UPRCombatSubsystem>();
		APawn* Player = UGameplayStatics::GetPlayerPawn(&World, 0);
		if (!Enemies || !Combat || !Player) return Fail(TEXT("Fixed Auditor encounter lost Enemy, Combat, or Player authority."));
		FGuid BossSpawnId;
		if (!Room.GetExpectedBossSpawnId(BossSpawnId))
		{
			TArray<FGuid> SpawnIds; Room.GetActiveEncounterSpawnIds(SpawnIds);
			for (const FGuid SpawnId : SpawnIds) { APREnemyCharacter* Enemy = nullptr; if (Enemies->ResolveSpawnedEnemy(SpawnId, Enemy) && Enemy && !Enemy->IsEnemyDead() && !ApplyDamage(*Combat, *Player, *Enemy, 1000000.0f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Combat rejected a fixed Auditor normal encounter hit.")); }
			return true;
		}
		TArray<FGuid> SpawnIds; Room.GetActiveEncounterSpawnIds(SpawnIds); bSawThreeVariants |= SpawnIds.Num() == 4;
		APREnemyCharacter* SpawnedBoss = nullptr;
		if (!Enemies->ResolveSpawnedEnemy(BossSpawnId, SpawnedBoss)) return Fail(TEXT("Auditor final encounter did not resolve its Boss."));
		APRAuditorChapterBoss* Boss = Cast<APRAuditorChapterBoss>(SpawnedBoss);
		UPRAuditorChapterBossComponent* ChapterComponent = Boss ? Boss->GetAuditorChapterBossComponent() : nullptr;
		if (!Boss || !ChapterComponent || !Boss->GetAuditorBossComponent()) return Fail(TEXT("Auditor final encounter spawned the wrong Boss contract."));
		switch (BossStep)
		{
		case EBossStep::KillVariants:
			for (const FGuid SpawnId : SpawnIds) { if (SpawnId == BossSpawnId) continue; APREnemyCharacter* Enemy = nullptr; if (Enemies->ResolveSpawnedEnemy(SpawnId, Enemy) && Enemy && !Enemy->IsEnemyDead() && !ApplyDamage(*Combat, *Player, *Enemy, 1000000.0f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Combat rejected an Auditor variant cleanup hit.")); }
			BossStep = EBossStep::EnterRepeatedAudit; return true;
		case EBossStep::EnterRepeatedAudit:
			if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.60f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Could not enter Auditor repeated-build audit through Combat."));
			BossStep = EBossStep::CounterRepeatedAudit; return true;
		case EBossStep::CounterRepeatedAudit:
			if (ChapterComponent->GetRuntimeState().Phase != EPRAuditorChapterBossPhase::RepeatedBuildAudit) return true;
			for (const TCHAR* Skill : { TEXT("Skill.FireSlash"), TEXT("Skill.ThunderDrop"), TEXT("Skill.AfterimageDodge"), TEXT("Skill.VectorHook") }) { if (ChapterComponent->GetRuntimeState().RemainingAuditUnits <= 0) break; if (!ApplyDamage(*Combat, *Player, *Boss, 0.1f, Skill)) return Fail(TEXT("Could not counter Auditor repeated-build audit.")); }
			bRepeatedCountered = ChapterComponent->GetRuntimeState().bRepeatedBuildCountered; BossStep = EBossStep::EnterVerdict; return true;
		case EBossStep::EnterVerdict:
			if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.20f, TEXT("Skill.CounterProofWall"))) return Fail(TEXT("Could not enter Auditor verdict escalation through Combat."));
			BossStep = EBossStep::CounterVerdict; return true;
		case EBossStep::CounterVerdict:
			if (ChapterComponent->GetRuntimeState().Phase != EPRAuditorChapterBossPhase::VerdictEscalation) return true;
			for (const TCHAR* Skill : { TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"), TEXT("Skill.ThunderDrop"), TEXT("Skill.AfterimageDodge"), TEXT("Skill.VectorHook"), TEXT("Skill.CounterProofWall") }) { if (ChapterComponent->GetRuntimeState().RemainingVerdictSkills <= 0) break; if (!ApplyDamage(*Combat, *Player, *Boss, 0.1f, Skill)) return Fail(TEXT("Could not counter Auditor verdict escalation.")); }
			bVerdictCountered = ChapterComponent->GetRuntimeState().bVerdictCountered; BossStep = EBossStep::Defeat; return true;
		case EBossStep::Defeat:
			if (!ApplyDamage(*Combat, *Player, *Boss, 1000000.0f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Could not defeat Auditor through Combat authority."));
			bBossCompletedOnce = Boss->GetAuditorBossComponent()->GetPhase() == EPRAuditorBossPhase::Defeated; BossStep = EBossStep::AwaitCompletion; return true;
		case EBossStep::AwaitCompletion: return true;
		}
		return Fail(TEXT("Fixed Auditor Boss runner entered an invalid step."));
	}

	bool ApplyDamage(UPRCombatSubsystem& Combat, APawn& Player, APREnemyCharacter& Enemy, float RawDamage, const TCHAR* AbilityTag) const
	{
		FPRDamageRequest Request; Request.SourceId = TEXT("Auditor.FixedPIE"); Request.DamageSource = &Player; Request.Instigator = &Player; Request.Target = &Enemy; Request.AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTag, false); Request.RawDamage = RawDamage; Request.ImpactOrigin = Player.GetActorLocation(); Request.IncomingDirection = (Enemy.GetActorLocation() - Player.GetActorLocation()).GetSafeNormal();
		return Combat.ApplyDamage(Request) == EPRCombatRequestStatus::Applied;
	}

	bool ApplyToHealthRatio(UPRCombatSubsystem& Combat, APawn& Player, APREnemyCharacter& Enemy, float TargetRatio, const TCHAR* AbilityTag) const
	{
		const UPRAttributeSet* Attributes = Enemy.GetAttributeSet();
		if (!Attributes || Attributes->GetMaxHealth() <= UE_SMALL_NUMBER) return false;
		const float TargetHealth = Attributes->GetMaxHealth() * TargetRatio;
		return ApplyDamage(Combat, Player, Enemy, Attributes->GetShield() + FMath::Max(0.1f, Attributes->GetHealth() - TargetHealth), AbilityTag);
	}

	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; TWeakObjectPtr<UPRRoomSubsystem> BoundRoom; FDelegateHandle RewardOfferHandle; FPRRewardOffer LatestOffer; EBossStep BossStep = EBossStep::KillVariants; bool bSawCombat = false; bool bSawEvent = false; bool bSawShop = false; bool bSawSafe = false; bool bSawBoss = false; bool bSawThreeVariants = false; bool bRepeatedCountered = false; bool bVerdictCountered = false; bool bBossCompletedOnce = false; int32 MaxAuditPressure = 0; double StartedAt = FPlatformTime::Seconds();
};
#endif
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::InspectFixedAuditorRegistry()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, PRAuditorAutomationPrivate::RegistryPath);
	if (!Registry) { Result->SetError(TEXT("Fixed Auditor Registry is unavailable.")); return Result; }
	Result->SetValue(FString::Printf(TEXT("{\"ready\":%s,\"rooms\":%d,\"encounters\":%d,\"events\":%d,\"policies\":%d,\"pressureBindings\":%d}"), Registry->IsRegistryReady() ? TEXT("true") : TEXT("false"), Registry->Rooms.Num(), Registry->Encounters.Num(), Registry->Events.Num(), Registry->RewardPolicies.Num(), Registry->EventPressureBindings.Num())); return Result;
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::PrepareFixedAuditorPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareFixedAuditorPIE must run before PIE starts.")); return Result; }
	PRAuditorAutomationPrivate::Storage = MakeShared<PRAuditorAutomationPrivate::FFixedStorage>();
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Local = FPRSaveStorage::CreateAutomation(Base, PRAuditorAutomationPrivate::Storage.ToSharedRef());
	if (!Local)
	{
		PRAuditorAutomationPrivate::Storage.Reset();
		Result->SetError(TEXT("Could not create isolated Auditor automation A/B storage."));
		return Result;
	}
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Local));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"failureBackend\":\"fixed-nth-save-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Auditor PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::RunFixedAuditorSelectionPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAuditorAutomationPrivate::FSelectionRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Auditor PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::RunFixedAuditorFullPathPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAuditorAutomationPrivate::FFullPathRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Auditor PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::RunFixedAuditorSettlementPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAuditorAutomationPrivate::FSettlementRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Auditor PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::InspectFixedAuditorPIEState()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UPRSaveSubsystem* Save = GI ? GI->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	UPRRunStateSubsystem* Run = GI ? GI->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
	UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr;
	UPRRoomSubsystem* Room = GI ? GI->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	UPREnemySubsystem* Enemies = World ? World->GetSubsystem<UPREnemySubsystem>() : nullptr;
	FPRChapterSnapshot ChapterState;
	if (Chapter) Chapter->GetSnapshot(ChapterState);
	FPRRoomRuntimeState RoomState;
	if (Room) Room->GetRoomRuntimeState(RoomState);
	FGuid BossSpawnId;
	APREnemyCharacter* Boss = nullptr;
	if (Room && Enemies && Room->GetExpectedBossSpawnId(BossSpawnId)) Enemies->ResolveSpawnedEnemy(BossSpawnId, Boss);
	const UPRAttributeSet* BossAttributes = Boss ? Boss->GetAttributeSet() : nullptr;
	const UPRAbilitySystemComponent* BossASC = Boss ? Boss->GetProjectRAbilitySystemComponent() : nullptr;
	const APRAuditorChapterBoss* AuditorBoss = Cast<APRAuditorChapterBoss>(Boss);
	const UPRAuditorChapterBossComponent* ChapterBoss = AuditorBoss ? AuditorBoss->GetAuditorChapterBossComponent() : nullptr;
	const UPRAuditorBossComponent* BaseBoss = AuditorBoss ? AuditorBoss->GetAuditorBossComponent() : nullptr;
	const FPRAuditorChapterBossRuntimeState ChapterBossState = ChapterBoss ? ChapterBoss->GetRuntimeState() : FPRAuditorChapterBossRuntimeState();
	Result->SetValue(FString::Printf(
		TEXT("{\"pie\":%s,\"save\":%d,\"profileLoaded\":%s,\"saveQueued\":%s,\"run\":%d,\"chapter\":%d,\"content\":\"%s\",\"fallback\":\"%s\",\"roomFlow\":%d,\"bossFound\":%s,\"bossHealth\":%.1f,\"bossAliveTags\":%d,\"bossDeadTags\":%d,\"baseBossPhase\":%d,\"chapterBossPhase\":%d,\"auditUnits\":%d,\"auditPressure\":%d,\"chapterBossDegraded\":%s}"),
		World ? TEXT("true") : TEXT("false"),
		Save ? static_cast<int32>(Save->GetSaveRuntimeState().State) : -1,
		Save && Save->GetSaveRuntimeState().bHasLoadedProfile ? TEXT("true") : TEXT("false"),
		Save && Save->GetSaveRuntimeState().bSaveRequestQueued ? TEXT("true") : TEXT("false"),
		Run ? static_cast<int32>(Run->GetRunRuntimeState().State) : -1,
		static_cast<int32>(ChapterState.State),
		*ChapterState.ContentId.ToString(),
		*ChapterState.FallbackReason.ToString(),
		static_cast<int32>(RoomState.FlowStatus),
		Boss ? TEXT("true") : TEXT("false"),
		BossAttributes ? BossAttributes->GetHealth() : -1.0f,
		BossASC ? BossASC->GetGameplayTagCount(UPRTagLibrary::GetStateAliveTag()) : -1,
		BossASC ? BossASC->GetGameplayTagCount(UPRTagLibrary::GetStateDeadTag()) : -1,
		BaseBoss ? static_cast<int32>(BaseBoss->GetPhase()) : -1,
		ChapterBoss ? static_cast<int32>(ChapterBossState.Phase) : -1,
		ChapterBossState.RemainingAuditUnits,
		ChapterBossState.AuditPressure,
		ChapterBossState.bDegradedNoOp ? TEXT("true") : TEXT("false")));
	return Result;
}

UToolCallAsyncResultString* UPRAuditorAutomationToolset::CleanupFixedAuditorPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("CleanupFixedAuditorPIE must run after PIE stops.")); return Result; }
	PRAuditorAutomationPrivate::Storage.Reset(); UPRSaveSubsystem::CleanupAutomationStorageOverride(); Result->SetValue(TEXT("{\"status\":\"CLEAN\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Auditor PIE automation unavailable."));
#endif
	return Result;
}
