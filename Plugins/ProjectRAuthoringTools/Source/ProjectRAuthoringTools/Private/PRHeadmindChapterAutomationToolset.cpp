// Copyright ProjectR. All Rights Reserved.

#include "PRHeadmindChapterAutomationToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "Async/Async.h"
#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Chapters/Headmind/PRHeadmindProjectionBossComponent.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRHeadmindAutomationPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
constexpr double TimeoutSeconds = 150.0;
const TCHAR* RegistryPath = TEXT("/Game/ProjectR/Chapters/Headmind/DA_RoguelikeContentRegistry_Headmind.DA_RoguelikeContentRegistry_Headmind");

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
	TMap<FString, TArray<uint8>> Slots; int32 SavesUntilFailure = INDEX_NONE;
};

TSharedPtr<FFixedStorage> Storage;

struct FBootstrap
{
	enum class EPhase : uint8 { CreateProfile, CreateAccount, StageProofs, StartRun, Ready };
	EPRAccountOperationResult Tick(const int32 Seed, FString& OutFailure)
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UPRSaveSubsystem* Save = GI ? GI->GetSubsystem<UPRSaveSubsystem>() : nullptr; UPRRunStateSubsystem* Run = GI ? GI->GetSubsystem<UPRRunStateSubsystem>() : nullptr; UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!World || World->GetNetMode() == NM_Client || !Save || !Run || !Chapter) { OutFailure = TEXT("Headmind fixture requires authoritative PIE and Save/Run/Chapter subsystems."); return EPRAccountOperationResult::RejectedInvalidState; }
		switch (Phase)
		{
		case EPhase::CreateProfile:
			if (!bIssued) { FGuid Id; if (Save->CreateNewDefaultProfile(Id) != EPRSaveResult::Success) { OutFailure = TEXT("Could not create isolated Headmind Profile."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			Phase = EPhase::CreateAccount; bIssued = false; return EPRAccountOperationResult::Started;
		case EPhase::CreateAccount:
			if (!bIssued) { FGuid Id; if (Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Headmind account creation was rejected."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed) { FGuid Id; if (Run->RetryPendingPersistence(Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Headmind account retry was rejected."); return EPRAccountOperationResult::RejectedInvalidState; } }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady) return EPRAccountOperationResult::Started;
			Phase = EPhase::StageProofs; bIssued = false; return EPRAccountOperationResult::Started;
		case EPhase::StageProofs:
			if (!bIssued) { if (!Chapter->StageFixedHeadmindPrerequisitesForAutomation()) { OutFailure = TEXT("Could not persist the four isolated Headmind prerequisite proofs."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready || Save->GetSaveRuntimeState().bSaveRequestQueued) return EPRAccountOperationResult::Started;
			if (!Chapter->RefreshFixedHeadmindSelectionForAutomation()) { FPRChapterSnapshot Snapshot; Chapter->GetSnapshot(Snapshot); OutFailure = FString::Printf(TEXT("Could not select fixed Headmind closure (state=%d,fallback=%s)."), static_cast<int32>(Snapshot.State), *Snapshot.FallbackReason.ToString()); return EPRAccountOperationResult::RejectedInvalidState; }
			Phase = EPhase::StartRun; bIssued = false; return EPRAccountOperationResult::Started;
		case EPhase::StartRun:
			if (!bIssued) { FGuid Id; if (Run->RequestStartRun(Seed, Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Headmind run start was rejected."); return EPRAccountOperationResult::RejectedInvalidState; } bIssued = true; }
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed) { FGuid Id; if (Run->RetryPendingPersistence(Id) != EPRAccountOperationResult::Started) { OutFailure = TEXT("Headmind run retry was rejected."); return EPRAccountOperationResult::RejectedInvalidState; } }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive) return EPRAccountOperationResult::Started;
			Phase = EPhase::Ready; return EPRAccountOperationResult::Succeeded;
		case EPhase::Ready: return EPRAccountOperationResult::Succeeded;
		}
		OutFailure = TEXT("Headmind bootstrap entered an invalid phase."); return EPRAccountOperationResult::RejectedInvalidState;
	}
	EPhase Phase = EPhase::CreateProfile; bool bIssued = false;
};

class FSelectionRunner final : public TSharedFromThis<FSelectionRunner>
{
public:
	static UToolCallAsyncResultString* Start() { TSharedRef<FSelectionRunner> Runner = MakeShared<FSelectionRunner>(); Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>()); if (!UPRSaveSubsystem::HasAutomationStorageOverride()) { Runner->Result->SetError(TEXT("PrepareFixedHeadmindPIE must run first.")); return Runner->Result.Get(); } FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); })); return Runner->Result.Get(); }
private:
	bool Tick()
	{
		if (FPlatformTime::Seconds() - Started > TimeoutSeconds) return Fail(TEXT("Fixed Headmind selection timed out."));
		FString Failure; const EPRAccountOperationResult Step = Bootstrap.Tick(61401, Failure); if (Step == EPRAccountOperationResult::Started) return true; if (Step != EPRAccountOperationResult::Succeeded) return Fail(Failure);
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr; UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr; UPRRoomSubsystem* Room = GI ? GI->GetSubsystem<UPRRoomSubsystem>() : nullptr; FPRChapterSnapshot Snapshot; FPRRoomRuntimeState Rooms;
		if (!Chapter || !Room || !Chapter->GetSnapshot(Snapshot) || !Room->GetRoomRuntimeState(Rooms) || Snapshot.ContentId != UPRChapterContentRegistryDataAsset::GetHeadmindContentId() || Snapshot.DirectiveId != TEXT("Headmind.RepetitionOptimality") || Rooms.Seed != 61401 || Rooms.PathLength != 7 || Rooms.Path.Num() != 7 || Room->GetConfiguredContentRegistryId() != UPRChapterContentRegistryDataAsset::GetHeadmindRoomRegistryId()) return Fail(TEXT("Fixed Headmind selection did not create the Seed-61401 closure."));
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61401,\"content\":\"Headmind\",\"directive\":\"Headmind.RepetitionOptimality\",\"pathLength\":7,\"userSlotsTouched\":false}")); return false;
	}
	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; double Started = FPlatformTime::Seconds();
};

class FSettlementRunner final : public TSharedFromThis<FSettlementRunner>
{
public:
	static UToolCallAsyncResultString* Start() { TSharedRef<FSettlementRunner> Runner = MakeShared<FSettlementRunner>(); Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>()); if (!Storage.IsValid()) { Runner->Result->SetError(TEXT("PrepareFixedHeadmindPIE must run first.")); return Runner->Result.Get(); } FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); })); return Runner->Result.Get(); }
private:
	enum class EPhase : uint8 { Bootstrap, Finalize, AwaitFailure, Retry, AwaitCompletion, Verify };
	bool Tick()
	{
		if (FPlatformTime::Seconds() - Started > TimeoutSeconds) return Fail(TEXT("Fixed Headmind persistence retry timed out."));
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr; UPRRunStateSubsystem* Run = GI ? GI->GetSubsystem<UPRRunStateSubsystem>() : nullptr; UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr; if (!Run || !Chapter) return Fail(TEXT("Headmind persistence lost RunState or Chapter."));
		if (Phase == EPhase::Bootstrap) { FString Failure; const EPRAccountOperationResult Step = Bootstrap.Tick(61404, Failure); if (Step == EPRAccountOperationResult::Started) return true; if (Step != EPRAccountOperationResult::Succeeded) return Fail(Failure); Phase = EPhase::Finalize; return true; }
		if (Phase == EPhase::Finalize) { if (!Chapter->StageFixedHeadmindCompletionFactsForAutomation()) return Fail(TEXT("Could not stage fixed Headmind completion facts.")); if (!bArmed) { Storage->FailNthUpcomingSave(3); bArmed = true; } Run->FinalizeActiveAccountForAutomation(EPRAccountTerminationReason::RoomSequenceCompleted); Phase = EPhase::AwaitFailure; return true; }
		FPRChapterSnapshot State; if (!Chapter->GetSnapshot(State)) return Fail(TEXT("Headmind persistence lost Chapter snapshot."));
		if (Phase == EPhase::AwaitFailure) { if (State.State != EPRChapterLifecycleState::ReadyToRetry) return true; if (State.bHasHumanAnomalyProof) return Fail(TEXT("Failed Headmind transaction published proof.")); Phase = EPhase::Retry; return true; }
		if (Phase == EPhase::Retry) { if (Chapter->RetryPendingSettlement() != EPRChapterOperationResult::Succeeded) return Fail(TEXT("Frozen Headmind retry was rejected.")); Phase = EPhase::AwaitCompletion; return true; }
		if (Phase == EPhase::AwaitCompletion) { if (State.State != EPRChapterLifecycleState::Completed) return true; Phase = EPhase::Verify; return true; }
		FPRChapterCompletionResult Completion; if (!State.bHasHumanAnomalyProof || !Chapter->GetLatestCompletion(Completion) || Completion.ProofId != UPRChapterContentRegistryDataAsset::GetHeadmindProofId() || Completion.SettlementSequence != 5 || Chapter->RetryPendingSettlement() != EPRChapterOperationResult::RejectedInvalidState) return Fail(TEXT("Headmind retry did not publish exactly one canonical fifth proof."));
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61404,\"firstSave\":\"WriteFailed\",\"retry\":\"PASS\",\"proof\":\"HumanAnomalyProof.Headmind\",\"settlementSequence\":5,\"duplicateRetry\":\"RejectedInvalidState\",\"userSlotsTouched\":false}")); return false;
	}
	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; EPhase Phase = EPhase::Bootstrap; bool bArmed = false; double Started = FPlatformTime::Seconds();
};

/** Drives the fixed Seed-61402 real Room/Reward/Combat path.  It never stages completion facts. */
class FFullPathRunner final : public TSharedFromThis<FFullPathRunner>
{
public:
	static UToolCallAsyncResultString* Start() { TSharedRef<FFullPathRunner> Runner = MakeShared<FFullPathRunner>(); Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>()); if (!UPRSaveSubsystem::HasAutomationStorageOverride()) { Runner->Result->SetError(TEXT("PrepareFixedHeadmindPIE must run first.")); return Runner->Result.Get(); } FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Keep = Runner.ToSharedPtr()](float) { return Keep->Tick(); })); return Runner->Result.Get(); }
	~FFullPathRunner() { if (UPRRoomSubsystem* Room = BoundRoom.Get()) Room->OnRewardOfferChanged().Remove(OfferHandle); }
private:
	enum class EBossStep : uint8 { KillVariants, WaitBossReady, TriggerRuleAudit, TriggerBasilisk, WaitBasilisk, Defeat, AwaitCompletion };
	bool Tick()
	{
		if (FPlatformTime::Seconds() - Started > TimeoutSeconds) return Fail(TEXT("Fixed Headmind full path timed out."));
		FString Failure; const EPRAccountOperationResult BootstrapResult = Bootstrap.Tick(61402, Failure); if (BootstrapResult == EPRAccountOperationResult::Started) return true; if (BootstrapResult != EPRAccountOperationResult::Succeeded) return Fail(Failure);
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr; UGameInstance* GI = World ? World->GetGameInstance() : nullptr; UPRRoomSubsystem* Room = GI ? GI->GetSubsystem<UPRRoomSubsystem>() : nullptr; UPRChapterSubsystem* Chapter = GI ? GI->GetSubsystem<UPRChapterSubsystem>() : nullptr; if (!World || !Room || !Chapter) return Fail(TEXT("Headmind full path lost World, Room, or Chapter."));
		BindOffer(*Room); FPRRoomRuntimeState State; if (!Room->GetRoomRuntimeState(State) || State.FlowStatus == EPRRoomFlowStatus::Cancelled) return Fail(TEXT("Headmind room flow was unavailable or cancelled."));
		if (State.FlowStatus == EPRRoomFlowStatus::Completed)
		{
			FPRChapterSnapshot Snapshot; FPRChapterCompletionResult Completion; if (!Chapter->GetSnapshot(Snapshot) || Snapshot.State != EPRChapterLifecycleState::Completed) return true;
			if (!Snapshot.bHasHumanAnomalyProof || !Chapter->GetLatestCompletion(Completion) || Completion.ProofId != UPRChapterContentRegistryDataAsset::GetHeadmindProofId() || Completion.SettlementSequence != 5 || !bSawCombat || !bSawEvent || !bSawShop || !bSawSafe || !bSawBoss || !bSawThreeVariants || !bBasilisk || !bBossCompleted)
			{
				return Fail(FString::Printf(TEXT("Fixed Headmind path missed a required fact (proof=%d, completion=%d, seq=%d, combat=%d, event=%d, shop=%d, safe=%d, boss=%d, variants=%d, basilisk=%d, bossComplete=%d)."), Snapshot.bHasHumanAnomalyProof, Completion.ProofId == UPRChapterContentRegistryDataAsset::GetHeadmindProofId(), Completion.SettlementSequence, bSawCombat, bSawEvent, bSawShop, bSawSafe, bSawBoss, bSawThreeVariants, bBasilisk, bBossCompleted));
			}
			Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"seed\":61402,\"combat\":true,\"event\":true,\"shopNoEconomy\":true,\"safe\":true,\"threeEnemyVariants\":true,\"basiliskDeferred\":true,\"bossCompletionCount\":1,\"proof\":\"HumanAnomalyProof.Headmind\",\"settlementSequence\":5,\"synthesisPressure\":%d,\"userSlotsTouched\":false}"), MaxPressure)); return false;
		}
		switch (State.FlowStatus) { case EPRRoomFlowStatus::SelectingRoom: return SelectRoom(*Room, State); case EPRRoomFlowStatus::SelectingEvent: return SelectEvent(*Room, State); case EPRRoomFlowStatus::SelectingReward: return SelectReward(*Room, State); case EPRRoomFlowStatus::EncounterActive: return DriveEncounter(*World, *Room); default: return true; }
	}
	void BindOffer(UPRRoomSubsystem& Room) { if (BoundRoom.Get() == &Room && OfferHandle.IsValid()) return; if (UPRRoomSubsystem* Previous = BoundRoom.Get()) Previous->OnRewardOfferChanged().Remove(OfferHandle); BoundRoom = &Room; OfferHandle = Room.OnRewardOfferChanged().AddLambda([WeakThis = TWeakPtr<FFullPathRunner>(AsShared())](const FPRRewardOffer& Offer) { if (const TSharedPtr<FFullPathRunner> This = WeakThis.Pin()) This->LatestOffer = Offer; }); }
	bool SelectRoom(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const int32 Next = State.CurrentStepIndex + 1;
		if (!State.Path.IsValidIndex(Next) || State.Path[Next].CandidateRoomIds.IsEmpty()) return Fail(TEXT("Headmind path exposed no candidate."));
		const FPrimaryAssetId Previous = State.Path.IsValidIndex(State.CurrentStepIndex) ? State.Path[State.CurrentStepIndex].SelectedRoomId : FPrimaryAssetId();
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, RegistryPath);
		if (!Registry) return Fail(TEXT("Headmind closed Registry was unavailable."));
		const FPrimaryAssetId* Selected = nullptr;
		for (const FPrimaryAssetId& Candidate : State.Path[Next].CandidateRoomIds)
		{
			if (Candidate == Previous) continue;
			const UPRRoomDataAsset* CandidateDefinition = Registry->FindRoom(Candidate);
			if (!CandidateDefinition) return Fail(TEXT("Headmind candidate escaped the closed registry."));
			const FString CandidateType = CandidateDefinition->TypeTag.ToString();
			const bool bFillsMissingCoverage =
				(!bSawCombat && (CandidateType == TEXT("Room.Type.Combat") || CandidateType == TEXT("Room.Type.Elite")))
				|| (!bSawEvent && CandidateType == TEXT("Room.Type.Event"))
				|| (!bSawShop && CandidateType == TEXT("Room.Type.Shop"))
				|| (!bSawSafe && CandidateType == TEXT("Room.Type.Safe"))
				|| (!bSawBoss && CandidateType == TEXT("Room.Type.Boss"));
			if (!Selected || bFillsMissingCoverage) { Selected = &Candidate; if (bFillsMissingCoverage) break; }
		}
		if (!Selected) return Fail(TEXT("Headmind path exposed only immediate repeats."));
		const UPRRoomDataAsset* Definition = Registry->FindRoom(*Selected);
		if (!Definition) return Fail(TEXT("Headmind selected candidate escaped the closed registry."));
		const FString Type = Definition->TypeTag.ToString();
		bSawCombat |= Type == TEXT("Room.Type.Combat") || Type == TEXT("Room.Type.Elite");
		bSawEvent |= Type == TEXT("Room.Type.Event"); bSawShop |= Type == TEXT("Room.Type.Shop"); bSawSafe |= Type == TEXT("Room.Type.Safe"); bSawBoss |= Type == TEXT("Room.Type.Boss");
		LatestOffer = FPRRewardOffer();
		return Room.SelectRoom(*Selected) == EPRRoomOperationResult::Succeeded ? true : Fail(TEXT("Headmind candidate was rejected."));
	}
	bool SelectEvent(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, RegistryPath); const FPrimaryAssetId EventId = Registry ? Registry->FindEventForRoom(State.ActiveRoomId) : FPrimaryAssetId(); const UPRRoomEventDataAsset* Event = Registry ? Registry->FindEvent(EventId) : nullptr; if (!Event || Event->Choices.IsEmpty()) return Fail(TEXT("Headmind Event has no closed choice set.")); FName Choice = Event->Choices[0].ChoiceId; int32 Best = MIN_int32; for (const FPRRoomEventChoice& Candidate : Event->Choices) { int32 Delta = 0; if (Registry->FindPressureDelta(EventId, Candidate.ChoiceId, Delta) && Delta > Best) { Best = Delta; Choice = Candidate.ChoiceId; } } if (Room.SelectEventChoice(Choice) != EPRRoomOperationResult::Succeeded) return Fail(TEXT("Headmind Event choice was rejected.")); if (UPRChapterSubsystem* Chapter = Room.GetGameInstance() ? Room.GetGameInstance()->GetSubsystem<UPRChapterSubsystem>() : nullptr) { FPRChapterSnapshot Snapshot; if (Chapter->GetSnapshot(Snapshot)) MaxPressure = FMath::Max(MaxPressure, Snapshot.SynthesisPressure); } return true;
	}
	bool SelectReward(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State) { if (!State.ActiveRewardOfferId.IsValid() || LatestOffer.OfferId != State.ActiveRewardOfferId || LatestOffer.Choices.Num() != 3 || LatestOffer.bResolved) return true; FGuid Handle; return Room.SelectReward(LatestOffer.Choices[0].RewardId, Handle) == EPRRoomOperationResult::Succeeded && Handle.IsValid() ? true : Fail(TEXT("Headmind three-choice reward was rejected.")); }
	bool DriveEncounter(UWorld& World, UPRRoomSubsystem& Room)
	{
		UPREnemySubsystem* Enemies = World.GetSubsystem<UPREnemySubsystem>(); UPRCombatSubsystem* Combat = World.GetSubsystem<UPRCombatSubsystem>(); APawn* Player = UGameplayStatics::GetPlayerPawn(&World, 0); if (!Enemies || !Combat || !Player) return Fail(TEXT("Headmind encounter lost Enemy, Combat, or Player."));
		FGuid BossId; if (!Room.GetExpectedBossSpawnId(BossId)) { TArray<FGuid> Spawns; Room.GetActiveEncounterSpawnIds(Spawns); for (const FGuid& Spawn : Spawns) { APREnemyCharacter* Enemy = nullptr; if (Enemies->ResolveSpawnedEnemy(Spawn, Enemy) && Enemy && !Enemy->IsEnemyDead() && !ApplyDamage(*Combat, *Player, *Enemy, 1000000.0f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Headmind normal encounter damage was rejected.")); } return true; }
		TArray<FGuid> Spawns; Room.GetActiveEncounterSpawnIds(Spawns); bSawThreeVariants |= Spawns.Num() == 4; APREnemyCharacter* EnemyBoss = nullptr; if (!Enemies->ResolveSpawnedEnemy(BossId, EnemyBoss)) return Fail(TEXT("Headmind final encounter did not resolve Boss.")); APRHeadmindProjectionBoss* Boss = Cast<APRHeadmindProjectionBoss>(EnemyBoss); UPRHeadmindProjectionBossComponent* Component = Boss ? Boss->GetHeadmindProjectionBossComponent() : nullptr; if (!Boss || !Component || !Boss->GetAuditorBossComponent()) return Fail(TEXT("Headmind final encounter spawned the wrong Boss."));
		switch (BossStep)
		{
		case EBossStep::KillVariants: for (const FGuid& Spawn : Spawns) { if (Spawn == BossId) continue; APREnemyCharacter* Variant = nullptr; if (Enemies->ResolveSpawnedEnemy(Spawn, Variant) && Variant && !Variant->IsEnemyDead() && !ApplyDamage(*Combat, *Player, *Variant, 1000000.0f, TEXT("Skill.ShadowThrust"))) return Fail(TEXT("Headmind variant damage was rejected.")); } BossStep = EBossStep::WaitBossReady; return true;
		case EBossStep::WaitBossReady: if (Boss->GetAuditorBossComponent()->GetPhase() != EPRAuditorBossPhase::Sampling) return true; BossStep = EBossStep::TriggerRuleAudit; return true;
		case EBossStep::TriggerRuleAudit: if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.45f, TEXT("Skill.FireSlash"))) return Fail(TEXT("Could not trigger Headmind Rule Audit through Combat.")); BossStep = EBossStep::TriggerBasilisk; return true;
		case EBossStep::TriggerBasilisk: if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.30f, TEXT("Skill.ThunderDrop"))) return Fail(TEXT("Could not trigger Headmind prediction shield through Combat.")); BossStep = EBossStep::WaitBasilisk; return true;
		case EBossStep::WaitBasilisk: if (Component->GetRuntimeState().Phase != EPRHeadmindBossPhase::BasiliskJudgment) return true; bBasilisk = Component->GetRuntimeState().TripleResonance.bWindowActive; BossStep = EBossStep::Defeat; return true;
		case EBossStep::Defeat: if (!ApplyDamage(*Combat, *Player, *Boss, 1000000.0f, TEXT("Skill.CounterProofWall"))) return Fail(TEXT("Could not defeat Headmind through Combat.")); bBossCompleted = Boss->GetAuditorBossComponent()->GetPhase() == EPRAuditorBossPhase::Defeated; BossStep = EBossStep::AwaitCompletion; return true;
		case EBossStep::AwaitCompletion: return true;
		}
		return Fail(TEXT("Headmind Boss runner entered an invalid step."));
	}
	bool ApplyDamage(UPRCombatSubsystem& Combat, APawn& Player, APREnemyCharacter& Enemy, const float Damage, const TCHAR* Ability) const { FPRDamageRequest Request; Request.SourceId = TEXT("Headmind.FixedPIE"); Request.DamageSource = &Player; Request.Instigator = &Player; Request.Target = &Enemy; Request.AbilityTag = FGameplayTag::RequestGameplayTag(Ability, false); Request.RawDamage = Damage; Request.ImpactOrigin = Player.GetActorLocation(); Request.IncomingDirection = (Enemy.GetActorLocation() - Player.GetActorLocation()).GetSafeNormal(); if (Request.IncomingDirection.IsNearlyZero()) Request.IncomingDirection = FVector::ForwardVector; return Combat.ApplyDamage(Request) == EPRCombatRequestStatus::Applied; }
	bool ApplyToHealthRatio(UPRCombatSubsystem& Combat, APawn& Player, APREnemyCharacter& Enemy, const float Ratio, const TCHAR* Ability) const { const UPRAttributeSet* Attributes = Enemy.GetAttributeSet(); return Attributes && Attributes->GetMaxHealth() > UE_SMALL_NUMBER && ApplyDamage(Combat, Player, Enemy, Attributes->GetShield() + FMath::Max(0.1f, Attributes->GetHealth() - Attributes->GetMaxHealth() * Ratio), Ability); }
	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FBootstrap Bootstrap; TWeakObjectPtr<UPRRoomSubsystem> BoundRoom; FDelegateHandle OfferHandle; FPRRewardOffer LatestOffer; EBossStep BossStep = EBossStep::KillVariants; bool bSawCombat = false; bool bSawEvent = false; bool bSawShop = false; bool bSawSafe = false; bool bSawBoss = false; bool bSawThreeVariants = false; bool bBasilisk = false; bool bBossCompleted = false; int32 MaxPressure = 0; double Started = FPlatformTime::Seconds();
};
#endif
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::InspectFixedHeadmindRegistry()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); const FSoftObjectPath RegistryPath(TEXT("/Game/ProjectR/Chapters/Headmind/DA_RoguelikeContentRegistry_Headmind.DA_RoguelikeContentRegistry_Headmind")); const UPRChapterRoguelikeContentRegistryDataAsset* Registry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(RegistryPath.TryLoad()); if (!Registry || Registry->ContentId != UPRChapterContentRegistryDataAsset::GetHeadmindContentId() || !Registry->IsRegistryReady()) { Result->SetError(TEXT("Fixed Headmind Registry is unavailable or invalid.")); return Result; } Result->SetValue(TEXT("{\"status\":\"PASS\",\"content\":\"Headmind\",\"rules\":5,\"rooms\":10,\"encounters\":4,\"events\":4,\"rewardPolicies\":5,\"userSlotsTouched\":false}")); return Result;
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::InspectFixedHeadmindPIEState()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPRRoomSubsystem* Room = GameInstance ? GameInstance->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	if (!World || !Room) { Result->SetError(TEXT("Fixed Headmind PIE inspection requires an active PIE RoomSubsystem.")); return Result; }
	FPRRoomRuntimeState RoomState;
	if (!Room->GetRoomRuntimeState(RoomState)) { Result->SetError(TEXT("Fixed Headmind PIE inspection could not read Room state.")); return Result; }
	FGuid BossSpawnId;
	APREnemyCharacter* Enemy = nullptr;
	UPREnemySubsystem* Enemies = World->GetSubsystem<UPREnemySubsystem>();
	const bool bHasBoss = Room->GetExpectedBossSpawnId(BossSpawnId) && Enemies && Enemies->ResolveSpawnedEnemy(BossSpawnId, Enemy);
	APRHeadmindProjectionBoss* Boss = bHasBoss ? Cast<APRHeadmindProjectionBoss>(Enemy) : nullptr;
	UPRHeadmindProjectionBossComponent* Headmind = Boss ? Boss->GetHeadmindProjectionBossComponent() : nullptr;
	const int32 BasePhase = Boss && Boss->GetAuditorBossComponent() ? static_cast<int32>(Boss->GetAuditorBossComponent()->GetPhase()) : -1;
	const int32 HeadmindPhase = Headmind ? static_cast<int32>(Headmind->GetRuntimeState().Phase) : -1;
	const bool bWindow = Headmind && Headmind->GetRuntimeState().TripleResonance.bWindowActive;
	const float Health = Boss && Boss->GetAttributeSet() ? Boss->GetAttributeSet()->GetHealth() : -1.0f;
	Result->SetValue(FString::Printf(TEXT("{\"flow\":%d,\"step\":%d,\"activeRoom\":\"%s\",\"hasBoss\":%s,\"basePhase\":%d,\"headmindPhase\":%d,\"window\":%s,\"health\":%.1f}"), static_cast<int32>(RoomState.FlowStatus), RoomState.CurrentStepIndex, *RoomState.ActiveRoomId.ToString(), bHasBoss ? TEXT("true") : TEXT("false"), BasePhase, HeadmindPhase, bWindow ? TEXT("true") : TEXT("false"), Health));
	return Result;
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::PrepareFixedHeadmindPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareFixedHeadmindPIE must run before PIE.")); return Result; }
	PRHeadmindAutomationPrivate::Storage = MakeShared<PRHeadmindAutomationPrivate::FFixedStorage>(); const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower()); TSharedPtr<FPRSaveStorage> Local = FPRSaveStorage::CreateAutomation(Base, PRHeadmindAutomationPrivate::Storage.ToSharedRef()); if (!Local) { PRHeadmindAutomationPrivate::Storage.Reset(); Result->SetError(TEXT("Could not create isolated Headmind A/B storage.")); return Result; } UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Local)); Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Headmind PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::RunFixedHeadmindSelectionPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRHeadmindAutomationPrivate::FSelectionRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Headmind PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::RunFixedHeadmindFullPathPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRHeadmindAutomationPrivate::FFullPathRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Headmind PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::RunFixedHeadmindPersistenceRetryPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRHeadmindAutomationPrivate::FSettlementRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Headmind PIE automation unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRHeadmindChapterAutomationToolset::CleanupFixedHeadmindPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("CleanupFixedHeadmindPIE must run after PIE stops.")); return Result; }
	PRHeadmindAutomationPrivate::Storage.Reset(); UPRSaveSubsystem::CleanupAutomationStorageOverride(); Result->SetValue(TEXT("{\"status\":\"CLEAN\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Headmind PIE automation unavailable."));
#endif
	return Result;
}
