// Copyright ProjectR. All Rights Reserved.

#include "PRPacifierAutomationToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "Chapters/Pacifier/PRPacifierBoss.h"
#include "Chapters/Pacifier/PRPacifierBossComponent.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyContentRegistryDataAsset.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRPacifierAutomationToolsetPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
constexpr double RunnerTimeoutSeconds = 120.0;
const TCHAR* PacifierRoomRegistryPath =
	TEXT("/Game/ProjectR/Chapters/Pacifier/DA_RoguelikeContentRegistry_Pacifier.DA_RoguelikeContentRegistry_Pacifier");
const TCHAR* PacifierEnemyRegistryPath =
	TEXT("/Game/ProjectR/Chapters/Pacifier/DA_EnemyContentRegistry_Pacifier.DA_EnemyContentRegistry_Pacifier");

class FFixedFailureStorageBackend final : public IPRSaveStorageBackend
{
public:
	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override
	{
		return Slots.Contains(Slot)
			? ISaveGameSystem::ESaveExistsResult::OK
			: ISaveGameSystem::ESaveExistsResult::DoesNotExist;
	}

	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override
	{
		const TArray<uint8>* Found = Slots.Find(Slot);
		if (!Found) return false;
		OutData = *Found;
		return true;
	}

	virtual void SaveGameAsync(
		const FString& Slot,
		TSharedRef<const TArray<uint8>> Data,
		TFunction<void(bool)> Completion) override
	{
		const bool bShouldFail = SavesUntilFailure == 1;
		if (SavesUntilFailure > 0) --SavesUntilFailure;
		if (!bShouldFail) Slots.Add(Slot, *Data);
		Completion(!bShouldFail);
	}

	virtual void LoadGameAsync(
		const FString& Slot,
		TFunction<void(bool, TArray<uint8>)> Completion) override
	{
		TArray<uint8> Data;
		const bool bLoaded = LoadGame(Slot, Data);
		Completion(bLoaded, MoveTemp(Data));
	}

	virtual bool DeleteGame(const FString& Slot) override
	{
		return Slots.Remove(Slot) > 0;
	}

	void FailNthUpcomingSave(const int32 SaveOrdinal)
	{
		SavesUntilFailure = FMath::Max(1, SaveOrdinal);
	}

	int32 GetSavesUntilFailure() const { return SavesUntilFailure; }

private:
	TMap<FString, TArray<uint8>> Slots;
	int32 SavesUntilFailure = INDEX_NONE;
};

TSharedPtr<FFixedFailureStorageBackend> FailureBackend;

enum class EBootstrapPhase : uint8
{
	CreateProfile,
	StagePrerequisites,
	CreateAccount,
	StartRun,
	Ready
};

struct FFixedPacifierBootstrap
{
	EPRAccountOperationResult Tick(const int32 Seed, FString& OutFailure)
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client)
		{
			OutFailure = TEXT("Fixed Pacifier fixture requires active authoritative in-process PIE.");
			return EPRAccountOperationResult::RejectedInvalidState;
		}
		UGameInstance* GameInstance = World->GetGameInstance();
		UPRSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UPRSaveSubsystem>() : nullptr;
		UPRRunStateSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!Save || !Run || !Chapter)
		{
			OutFailure = TEXT("Fixed Pacifier fixture is missing Save, RunState, or Chapter subsystem.");
			return EPRAccountOperationResult::RejectedInvalidState;
		}

		switch (Phase)
		{
		case EBootstrapPhase::CreateProfile:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Save->CreateNewDefaultProfile(RequestId) != EPRSaveResult::Success)
				{
					OutFailure = TEXT("Could not create the isolated Pacifier fixture profile.");
					return EPRAccountOperationResult::RejectedInvalidState;
				}
				bIssued = true;
			}
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready
				|| !Save->GetSaveRuntimeState().bHasLoadedProfile)
			{
				return EPRAccountOperationResult::Started;
			}
			Phase = EBootstrapPhase::StagePrerequisites;
			bIssued = false;
			return EPRAccountOperationResult::Started;
		case EBootstrapPhase::StagePrerequisites:
			if (!bIssued)
			{
				if (!Chapter->StageFixedPacifierPrerequisitesForAutomation())
				{
					OutFailure = TEXT("Could not persist the fixed Allocator/Warden prerequisite proof chain.");
					return EPRAccountOperationResult::RejectedInvalidState;
				}
				bIssued = true;
			}
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready
				|| Save->GetSaveRuntimeState().bSaveRequestQueued)
			{
				return EPRAccountOperationResult::Started;
			}
			Phase = EBootstrapPhase::CreateAccount;
			bIssued = false;
			return EPRAccountOperationResult::Started;
		case EBootstrapPhase::CreateAccount:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Run->RequestCreateAccount(
						FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")),
						RequestId)
					!= EPRAccountOperationResult::Started)
				{
					OutFailure = TEXT("Fixed Pacifier account creation did not start.");
					return EPRAccountOperationResult::RejectedInvalidState;
				}
				bIssued = true;
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady)
			{
				return EPRAccountOperationResult::Started;
			}
			Phase = EBootstrapPhase::StartRun;
			bIssued = false;
			return EPRAccountOperationResult::Started;
		case EBootstrapPhase::StartRun:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Run->RequestStartRun(Seed, RequestId) != EPRAccountOperationResult::Started)
				{
					OutFailure = TEXT("Fixed Pacifier run did not start.");
					return EPRAccountOperationResult::RejectedInvalidState;
				}
				bIssued = true;
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive)
			{
				return EPRAccountOperationResult::Started;
			}
			Phase = EBootstrapPhase::Ready;
			return EPRAccountOperationResult::Succeeded;
		case EBootstrapPhase::Ready:
			return EPRAccountOperationResult::Succeeded;
		}
		OutFailure = TEXT("Fixed Pacifier bootstrap entered an invalid phase.");
		return EPRAccountOperationResult::RejectedInvalidState;
	}

	EBootstrapPhase Phase = EBootstrapPhase::CreateProfile;
	bool bIssued = false;
};

class FFixedPacifierSelectionRunner final : public TSharedFromThis<FFixedPacifierSelectionRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedPacifierSelectionRunner> Runner = MakeShared<FFixedPacifierSelectionRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedPacifierPIE must run before selection PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		if (FPlatformTime::Seconds() - StartedAt > RunnerTimeoutSeconds) return Fail(TEXT("Fixed Pacifier selection timed out."));
		FString Failure;
		const EPRAccountOperationResult BootstrapResult = Bootstrap.Tick(61201, Failure);
		if (BootstrapResult == EPRAccountOperationResult::Started) return true;
		if (BootstrapResult != EPRAccountOperationResult::Succeeded) return Fail(Failure);

		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UPRChapterSubsystem* Chapter = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		UPRRoomSubsystem* Room = GameInstance ? GameInstance->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		FPRChapterSnapshot Snapshot;
		FPRRoomRuntimeState RoomState;
		if (!Chapter || !Room || !Chapter->GetSnapshot(Snapshot) || !Room->GetRoomRuntimeState(RoomState))
		{
			return Fail(TEXT("Fixed Pacifier selection lost its public subsystem snapshots."));
		}
		if (Snapshot.ContentId != UPRChapterContentRegistryDataAsset::GetPacifierContentId()
			|| Snapshot.DirectiveId != TEXT("Pacifier.EmotionalDampening")
			|| Snapshot.ComfortPressure != 0
			|| RoomState.Seed != 61201
			|| RoomState.PathLength != 7
			|| RoomState.Path.Num() != 7
			|| RoomState.FlowStatus != EPRRoomFlowStatus::SelectingRoom
			|| Room->GetConfiguredContentRegistryId() != UPRChapterContentRegistryDataAsset::GetPacifierRoomRegistryId())
		{
			return Fail(TEXT("Allocator/Warden proofs did not select the exact Seed-61201 Pacifier closure."));
		}
		for (const FPRRoomPathStep& Step : RoomState.Path)
		{
			if (Step.CandidateRoomIds.IsEmpty()) return Fail(TEXT("Seed 61201 exposed an empty Pacifier candidate step."));
			for (int32 Index = 1; Index < Step.CandidateRoomIds.Num(); ++Index)
			{
				if (Step.CandidateRoomIds[Index - 1].ToString() >= Step.CandidateRoomIds[Index].ToString())
				{
					return Fail(TEXT("Seed 61201 candidates are not in strict PrimaryAssetId order."));
				}
			}
		}
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61201,\"pathLength\":7,\"content\":\"Pacifier\",\"directive\":\"Pacifier.EmotionalDampening\",\"comfortPressure\":0,\"registryClosed\":true,\"userSlotsTouched\":false}"));
		return false;
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	FFixedPacifierBootstrap Bootstrap;
	double StartedAt = FPlatformTime::Seconds();
};

class FFixedPacifierFullPathRunner final : public TSharedFromThis<FFixedPacifierFullPathRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedPacifierFullPathRunner> Runner = MakeShared<FFixedPacifierFullPathRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedPacifierPIE must run before full-path PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

	~FFixedPacifierFullPathRunner()
	{
		if (UPRRoomSubsystem* Room = BoundRoom.Get())
		{
			Room->OnRewardOfferChanged().Remove(RewardOfferHandle);
		}
	}

private:
	enum class EBossStep : uint8
	{
		KillVariants,
		EnterIllusion,
		CounterIllusion,
		EnterLure,
		CounterLure,
		EnterSuppression,
		CounterSuppression,
		Defeat,
		AwaitCompletion
	};

	bool Tick()
	{
		if (FPlatformTime::Seconds() - StartedAt > RunnerTimeoutSeconds)
		{
			UWorld* TimeoutWorld = GEditor ? GEditor->PlayWorld : nullptr;
			UGameInstance* TimeoutGameInstance = TimeoutWorld ? TimeoutWorld->GetGameInstance() : nullptr;
			UPRRoomSubsystem* TimeoutRoom =
				TimeoutGameInstance ? TimeoutGameInstance->GetSubsystem<UPRRoomSubsystem>() : nullptr;
			UPRChapterSubsystem* TimeoutChapter =
				TimeoutGameInstance ? TimeoutGameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
			UPRRunStateSubsystem* TimeoutRun =
				TimeoutGameInstance ? TimeoutGameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
			FPRRoomRuntimeState TimeoutRoomState;
			if (TimeoutRoom) TimeoutRoom->GetRoomRuntimeState(TimeoutRoomState);
			FPRChapterSnapshot TimeoutChapterState;
			if (TimeoutChapter) TimeoutChapter->GetSnapshot(TimeoutChapterState);
			bool bTimeoutRoomVerified = false;
			bool bTimeoutBossVerified = false;
			bool bTimeoutAccountVerified = false;
			bool bTimeoutSettlementRequested = false;
			bool bTimeoutSettlementPending = false;
			if (TimeoutChapter)
			{
				TimeoutChapter->GetFixedPacifierSettlementDiagnosticsForAutomation(
					bTimeoutRoomVerified,
					bTimeoutBossVerified,
					bTimeoutAccountVerified,
					bTimeoutSettlementRequested,
					bTimeoutSettlementPending);
			}
			EPRPacifierBossPhase TimeoutBossPhase = EPRPacifierBossPhase::Dormant;
			int32 TimeoutProjectionCount = 0;
			int32 TimeoutSuppressionLayers = 0;
			FGuid TimeoutBossSpawnId;
			if (TimeoutWorld && TimeoutRoom && TimeoutRoom->GetExpectedBossSpawnId(TimeoutBossSpawnId))
			{
				if (UPREnemySubsystem* TimeoutEnemies = TimeoutWorld->GetSubsystem<UPREnemySubsystem>())
				{
					APREnemyCharacter* TimeoutEnemy = nullptr;
					if (TimeoutEnemies->ResolveSpawnedEnemy(TimeoutBossSpawnId, TimeoutEnemy))
					{
						if (const APRPacifierBoss* TimeoutBoss = Cast<APRPacifierBoss>(TimeoutEnemy))
						{
							if (const UPRPacifierBossComponent* TimeoutComponent = TimeoutBoss->GetPacifierBossComponent())
							{
								TimeoutBossPhase = TimeoutComponent->GetRuntimeState().Phase;
								TimeoutProjectionCount = TimeoutComponent->GetRuntimeState().ActiveProjectionCount;
								TimeoutSuppressionLayers = TimeoutComponent->GetRuntimeState().SuppressionLayers;
							}
						}
					}
				}
			}
			return Fail(FString::Printf(
				TEXT("Fixed Pacifier full path timed out (flow=%d,step=%d,bossStep=%d,bossPhase=%d,projections=%d,layers=%d,chapterState=%d,chapterFallback=%s,runState=%d,roomVerified=%s,bossVerified=%s,accountVerified=%s,settlementRequested=%s,settlementPending=%s)."),
				static_cast<int32>(TimeoutRoomState.FlowStatus),
				TimeoutRoomState.CurrentStepIndex,
				static_cast<int32>(BossStep),
				static_cast<int32>(TimeoutBossPhase),
				TimeoutProjectionCount,
				TimeoutSuppressionLayers,
				static_cast<int32>(TimeoutChapterState.State),
				*TimeoutChapterState.FallbackReason.ToString(),
				TimeoutRun ? static_cast<int32>(TimeoutRun->GetRunRuntimeState().State) : -1,
				bTimeoutRoomVerified ? TEXT("true") : TEXT("false"),
				bTimeoutBossVerified ? TEXT("true") : TEXT("false"),
				bTimeoutAccountVerified ? TEXT("true") : TEXT("false"),
				bTimeoutSettlementRequested ? TEXT("true") : TEXT("false"),
				bTimeoutSettlementPending ? TEXT("true") : TEXT("false")));
		}
		FString Failure;
		const EPRAccountOperationResult BootstrapResult = Bootstrap.Tick(61202, Failure);
		if (BootstrapResult == EPRAccountOperationResult::Started) return true;
		if (BootstrapResult != EPRAccountOperationResult::Succeeded) return Fail(Failure);

		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UPRRoomSubsystem* Room = GameInstance ? GameInstance->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!World || !Room || !Chapter) return Fail(TEXT("Fixed Pacifier full path lost its runtime subsystems."));
		BindRewardOffer(*Room);

		FPRRoomRuntimeState State;
		if (!Room->GetRoomRuntimeState(State)) return Fail(TEXT("Fixed Pacifier full path could not read Room state."));
		if (State.FlowStatus == EPRRoomFlowStatus::Cancelled)
		{
			TArray<FPRRewardApplicationHandle> Applied;
			Room->GetAppliedRewards(Applied);
			FString RewardState;
			for (const FPRRewardApplicationHandle& Handle : Applied)
			{
				if (!RewardState.IsEmpty()) RewardState += TEXT(",");
				RewardState += FString::Printf(TEXT("%s:T%d"), *Handle.FamilyId.ToString(), Handle.Tier);
			}
			return Fail(FString::Printf(
				TEXT("Fixed Pacifier room flow cancelled (step=%d,activeRoom=%s,rewardOffer=%s,fallback=%s,applied=[%s])."),
				State.CurrentStepIndex,
				*State.ActiveRoomId.ToString(),
				*State.ActiveRewardOfferId.ToString(),
				*State.ChapterOfferFallbackReason.ToString(),
				*RewardState));
		}
		if (State.FlowStatus == EPRRoomFlowStatus::Completed)
		{
			FPRChapterSnapshot Snapshot;
			FPRChapterCompletionResult Completion;
			if (!Chapter->GetSnapshot(Snapshot))
			{
				return Fail(TEXT("Completed Pacifier path lost its value snapshots."));
			}
			if (Snapshot.State != EPRChapterLifecycleState::Completed) return true;
			if (!Snapshot.bHasHumanAnomalyProof
				|| !Chapter->GetLatestCompletion(Completion)
				|| Completion.ProofId != UPRChapterContentRegistryDataAsset::GetPacifierProofId()
				|| Completion.SettlementSequence != 3)
			{
				return Fail(TEXT("Successful full path did not durably publish exactly one Pacifier proof."));
			}
			if (!bSawCombat || !bSawEvent || !bSawShop || !bSawSafe || !bSawBoss || !bSawThreeVariants
				|| !bIllusionCountered || !bLureCountered || !bSuppressionCountered || !bBossCompletedOnce)
			{
				return Fail(TEXT("Fixed Seed-61202 path did not cover every Pacifier acceptance fact."));
			}
			Result->SetValue(FString::Printf(
				TEXT("{\"status\":\"PASS\",\"seed\":61202,\"pathLength\":8,\"combat\":true,\"event\":true,\"shopNoEconomy\":true,\"safe\":true,\"threeEnemyVariants\":true,\"illusionCountered\":true,\"lureCountered\":true,\"suppressionCountered\":true,\"bossCompletionCount\":1,\"proof\":\"HumanAnomalyProof.Pacifier\",\"settlementSequence\":3,\"userSlotsTouched\":false,\"comfortPressure\":%d}"),
				MaxComfortPressure));
			return false;
		}

		switch (State.FlowStatus)
		{
		case EPRRoomFlowStatus::SelectingRoom:
			return SelectNextRoom(*Room, State);
		case EPRRoomFlowStatus::SelectingEvent:
			return ResolveEvent(*Room, State);
		case EPRRoomFlowStatus::SelectingReward:
			return ResolveReward(*Room, State);
		case EPRRoomFlowStatus::EncounterActive:
			return DriveEncounter(*World, *Room, State);
		default:
			return true;
		}
	}

	void BindRewardOffer(UPRRoomSubsystem& Room)
	{
		if (BoundRoom.Get() == &Room && RewardOfferHandle.IsValid()) return;
		if (UPRRoomSubsystem* Previous = BoundRoom.Get()) Previous->OnRewardOfferChanged().Remove(RewardOfferHandle);
		BoundRoom = &Room;
		RewardOfferHandle = Room.OnRewardOfferChanged().AddLambda(
			[WeakThis = TWeakPtr<FFixedPacifierFullPathRunner>(AsShared())](const FPRRewardOffer& Offer)
			{
				if (const TSharedPtr<FFixedPacifierFullPathRunner> This = WeakThis.Pin()) This->LatestOffer = Offer;
			});
	}

	bool SelectNextRoom(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const int32 NextStepIndex = State.CurrentStepIndex + 1;
		if (!State.Path.IsValidIndex(NextStepIndex)
			|| State.Path[NextStepIndex].CandidateRoomIds.IsEmpty())
		{
			return Fail(TEXT("Pacifier path exposed no deterministic candidate."));
		}
		const FPrimaryAssetId Previous =
			State.Path.IsValidIndex(State.CurrentStepIndex)
				? State.Path[State.CurrentStepIndex].SelectedRoomId
				: FPrimaryAssetId();
		const FPrimaryAssetId* SelectedCandidate = State.Path[NextStepIndex].CandidateRoomIds.FindByPredicate(
			[Previous](const FPrimaryAssetId& Candidate) { return Candidate != Previous; });
		if (!SelectedCandidate) return Fail(TEXT("Pacifier path exposed only the immediately previous room."));
		const FPrimaryAssetId Selected = *SelectedCandidate;
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry =
			LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, PacifierRoomRegistryPath);
		const UPRRoomDataAsset* Definition = Registry ? Registry->FindRoom(Selected) : nullptr;
		if (!Definition) return Fail(TEXT("Pacifier candidate escaped its closed Registry."));
		const FString Type = Definition->TypeTag.ToString();
		bSawCombat |= Type == TEXT("Room.Type.Combat") || Type == TEXT("Room.Type.Elite");
		bSawEvent |= Type == TEXT("Room.Type.Event");
		bSawShop |= Type == TEXT("Room.Type.Shop");
		bSawSafe |= Type == TEXT("Room.Type.Safe");
		bSawBoss |= Type == TEXT("Room.Type.Boss");
		LatestOffer = FPRRewardOffer();
		const EPRRoomOperationResult SelectionResult = Room.SelectRoom(Selected);
		if (SelectionResult != EPRRoomOperationResult::Succeeded)
		{
			return Fail(FString::Printf(
				TEXT("Closed Pacifier candidate was rejected (step=%d,room=%s,result=%d)."),
				NextStepIndex,
				*Selected.ToString(),
				static_cast<int32>(SelectionResult)));
		}
		return true;
	}

	bool ResolveEvent(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry =
			LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, PacifierRoomRegistryPath);
		const FPrimaryAssetId EventId = Registry ? Registry->FindEventForRoom(State.ActiveRoomId) : FPrimaryAssetId();
		const UPRRoomEventDataAsset* Event = Registry ? Registry->FindEvent(EventId) : nullptr;
		if (!Event || Event->Choices.IsEmpty()) return Fail(TEXT("Pacifier Event room has no fixed choice closure."));
		FName ChoiceId = Event->Choices[0].ChoiceId;
		int32 BestDelta = MIN_int32;
		for (const FPRRoomEventChoice& Choice : Event->Choices)
		{
			int32 Delta = 0;
			if (Registry->FindPressureDelta(EventId, Choice.ChoiceId, Delta) && Delta > BestDelta)
			{
				BestDelta = Delta;
				ChoiceId = Choice.ChoiceId;
			}
		}
		if (Room.SelectEventChoice(ChoiceId) != EPRRoomOperationResult::Succeeded)
		{
			return Fail(TEXT("Fixed Pacifier Event choice was rejected."));
		}
		if (UGameInstance* GameInstance = Room.GetGameInstance())
		{
			if (UPRChapterSubsystem* Chapter = GameInstance->GetSubsystem<UPRChapterSubsystem>())
			{
				FPRChapterSnapshot Snapshot;
				if (Chapter->GetSnapshot(Snapshot)) MaxComfortPressure = FMath::Max(MaxComfortPressure, Snapshot.ComfortPressure);
			}
		}
		return true;
	}

	bool ResolveReward(UPRRoomSubsystem& Room, const FPRRoomRuntimeState& State)
	{
		if (!State.ActiveRewardOfferId.IsValid()
			|| LatestOffer.OfferId != State.ActiveRewardOfferId
			|| LatestOffer.Choices.Num() != 3
			|| LatestOffer.bResolved)
		{
			return true;
		}
		const UPRChapterRoguelikeContentRegistryDataAsset* Registry =
			LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, PacifierRoomRegistryPath);
		TArray<FPRRewardApplicationHandle> Applied;
		Room.GetAppliedRewards(Applied);
		const FPRRewardOfferChoice* SelectedChoice = nullptr;
		int32 BestFutureFamilyCount = MIN_int32;
		int32 SelectedTier = MAX_int32;
		bool bSelectedExistingFamily = false;
		for (const FPRRewardOfferChoice& Choice : LatestOffer.Choices)
		{
			const UPRRewardDataAsset* Reward = Registry ? Registry->FindReward(Choice.RewardId) : nullptr;
			if (!Reward) continue;
			const bool bExistingFamily = Applied.ContainsByPredicate(
				[Reward](const FPRRewardApplicationHandle& Handle) { return Handle.FamilyId == Reward->FamilyId; });
			TArray<FPRRewardApplicationHandle> Simulated = Applied;
			Simulated.RemoveAll(
				[Reward](const FPRRewardApplicationHandle& Handle)
				{
					return Handle.FamilyId == Reward->FamilyId && Handle.Tier <= Reward->Tier;
				});
			FPRRewardApplicationHandle SimulatedHandle;
			SimulatedHandle.RewardId = Choice.RewardId;
			SimulatedHandle.FamilyId = Reward->FamilyId;
			SimulatedHandle.Tier = Reward->Tier;
			Simulated.Add(SimulatedHandle);
			TSet<FName> FutureFamilies;
			for (const TSoftObjectPtr<UPRRewardDataAsset>& FutureRewardRef : Registry->Rewards)
			{
				const UPRRewardDataAsset* FutureReward = FutureRewardRef.LoadSynchronous();
				if (!FutureReward
					|| !FPRRewardContract::CanSelectFamilyTier(
						Simulated,
						FutureReward->FamilyId,
						FutureReward->Tier))
				{
					continue;
				}
				bool bExcluded = false;
				for (const FPRRewardApplicationHandle& Handle : Simulated)
				{
					const UPRRewardDataAsset* Existing = Registry->FindReward(Handle.RewardId);
					if (Existing && FutureReward->MutualExclusionTags.HasAny(Existing->MutualExclusionTags))
					{
						bExcluded = true;
						break;
					}
				}
				if (!bExcluded) FutureFamilies.Add(FutureReward->FamilyId);
			}
			const int32 FutureFamilyCount = FutureFamilies.Num();
			if (!SelectedChoice
				|| FutureFamilyCount > BestFutureFamilyCount
				|| (FutureFamilyCount == BestFutureFamilyCount && bExistingFamily && !bSelectedExistingFamily)
				|| (FutureFamilyCount == BestFutureFamilyCount
					&& bExistingFamily == bSelectedExistingFamily
					&& Reward->Tier < SelectedTier))
			{
				SelectedChoice = &Choice;
				BestFutureFamilyCount = FutureFamilyCount;
				SelectedTier = Reward->Tier;
				bSelectedExistingFamily = bExistingFamily;
			}
		}
		if (!SelectedChoice) return Fail(TEXT("Fixed Pacifier reward offer escaped the closed Reward Registry."));
		FGuid HandleId;
		if (Room.SelectReward(SelectedChoice->RewardId, HandleId) != EPRRoomOperationResult::Succeeded
			|| !HandleId.IsValid())
		{
			return Fail(TEXT("Fixed three-choice Pacifier reward could not be selected."));
		}
		return true;
	}

	bool DriveEncounter(UWorld& World, UPRRoomSubsystem& Room, const FPRRoomRuntimeState&)
	{
		UPREnemySubsystem* Enemies = World.GetSubsystem<UPREnemySubsystem>();
		UPRCombatSubsystem* Combat = World.GetSubsystem<UPRCombatSubsystem>();
		APawn* Player = UGameplayStatics::GetPlayerPawn(&World, 0);
		if (!Enemies || !Combat || !Player) return Fail(TEXT("Pacifier encounter lost Enemy, Combat, or player authority."));
		FGuid BossSpawnId;
		if (!Room.GetExpectedBossSpawnId(BossSpawnId))
		{
			TArray<FGuid> SpawnIds;
			Room.GetActiveEncounterSpawnIds(SpawnIds);
			for (const FGuid SpawnId : SpawnIds)
			{
				APREnemyCharacter* Enemy = nullptr;
				if (Enemies->ResolveSpawnedEnemy(SpawnId, Enemy) && Enemy && !Enemy->IsEnemyDead())
				{
					if (!ApplyDamage(*Combat, *Player, *Enemy, 1000000.0f, TEXT("Skill.ShadowThrust")))
					{
						return Fail(TEXT("Combat authority rejected the fixed normal Pacifier encounter hit."));
					}
				}
			}
			return true;
		}

		TArray<FGuid> SpawnIds;
		Room.GetActiveEncounterSpawnIds(SpawnIds);
		bSawThreeVariants |= SpawnIds.Num() == 4;
		APREnemyCharacter* BossEnemy = nullptr;
		if (!Enemies->ResolveSpawnedEnemy(BossSpawnId, BossEnemy) || !BossEnemy)
		{
			return Fail(TEXT("Pacifier final Encounter did not expose its fixed Boss spawn."));
		}
		APRPacifierBoss* Boss = Cast<APRPacifierBoss>(BossEnemy);
		UPRPacifierBossComponent* Component = Boss ? Boss->GetPacifierBossComponent() : nullptr;
		if (!Boss || !Component) return Fail(TEXT("Pacifier final Encounter spawned the wrong Boss class."));

		switch (BossStep)
		{
		case EBossStep::KillVariants:
			for (const FGuid SpawnId : SpawnIds)
			{
				if (SpawnId == BossSpawnId) continue;
				APREnemyCharacter* Enemy = nullptr;
				if (Enemies->ResolveSpawnedEnemy(SpawnId, Enemy) && Enemy && !Enemy->IsEnemyDead())
				{
					if (!ApplyDamage(*Combat, *Player, *Enemy, 1000000.0f, TEXT("Skill.ShadowThrust")))
						return Fail(TEXT("Combat authority rejected a Pacifier variant cleanup hit."));
				}
			}
			BossStep = EBossStep::EnterIllusion;
			return true;
		case EBossStep::EnterIllusion:
			if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.70f, TEXT("Skill.ShadowThrust")))
				return Fail(TEXT("Could not enter Pacifier Illusion Split through Combat."));
			BossStep = EBossStep::CounterIllusion;
			return true;
		case EBossStep::CounterIllusion:
			if (Component->GetRuntimeState().ActiveProjectionCount != 2) return true;
			if (!ApplyDamage(*Combat, *Player, *Boss, 0.1f, TEXT("Skill.FireSlash"))
				|| !ApplyDamage(*Combat, *Player, *Boss, 0.1f, TEXT("Skill.ThunderDrop")))
			{
				return Fail(TEXT("Could not counter Pacifier Illusion Split with two distinct P0 skills."));
			}
			bIllusionCountered = Component->GetRuntimeState().bIllusionSplitCountered
				&& Component->GetRuntimeState().ActiveProjectionCount == 0;
			BossStep = EBossStep::EnterLure;
			return true;
		case EBossStep::EnterLure:
			if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.45f, TEXT("Skill.AfterimageDodge")))
				return Fail(TEXT("Could not enter Pacifier Low-risk Reward Lure through Combat."));
			BossStep = EBossStep::CounterLure;
			return true;
		case EBossStep::CounterLure:
			if (Component->GetRuntimeState().Phase != EPRPacifierBossPhase::LowRiskRewardLure) return true;
			Player->SetActorLocation(Boss->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f));
			if (!ApplyDamage(*Combat, *Player, *Boss, 0.1f, TEXT("Skill.VectorHook")))
				return Fail(TEXT("Could not counter Pacifier Low-risk Reward Lure through Combat."));
			bLureCountered = Component->GetRuntimeState().bLowRiskLureCountered;
			BossStep = EBossStep::EnterSuppression;
			return true;
		case EBossStep::EnterSuppression:
			if (!ApplyToHealthRatio(*Combat, *Player, *Boss, 0.20f, TEXT("Skill.CounterProofWall")))
				return Fail(TEXT("Could not enter Pacifier Adventure Yield Suppression through Combat."));
			BossStep = EBossStep::CounterSuppression;
			return true;
		case EBossStep::CounterSuppression:
			if (Component->GetRuntimeState().Phase != EPRPacifierBossPhase::AdventureYieldSuppression) return true;
			for (const TCHAR* Skill : {TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"), TEXT("Skill.ThunderDrop"), TEXT("Skill.AfterimageDodge")})
			{
				if (Component->GetRuntimeState().SuppressionLayers <= 0) break;
				if (!ApplyDamage(*Combat, *Player, *Boss, 0.1f, Skill))
					return Fail(TEXT("Could not remove a Pacifier suppression layer through Combat."));
			}
			bSuppressionCountered = Component->GetRuntimeState().bYieldSuppressionCountered
				&& Component->GetRuntimeState().SuppressionLayers == 0;
			BossStep = EBossStep::Defeat;
			return true;
		case EBossStep::Defeat:
			if (!ApplyDamage(*Combat, *Player, *Boss, 1000000.0f, TEXT("Skill.ShadowThrust")))
				return Fail(TEXT("Could not defeat Pacifier through Combat authority."));
			bBossCompletedOnce = Component->GetRuntimeState().Phase == EPRPacifierBossPhase::Defeated
				&& Component->GetRuntimeState().ActiveProjectionCount == 0;
			BossStep = EBossStep::AwaitCompletion;
			return true;
		case EBossStep::AwaitCompletion:
			return true;
		}
		return Fail(TEXT("Pacifier Boss runner entered an invalid phase."));
	}

	bool ApplyDamage(
		UPRCombatSubsystem& Combat,
		APawn& Player,
		APREnemyCharacter& Enemy,
		const float RawDamage,
		const TCHAR* AbilityTag) const
	{
		FPRDamageRequest Request;
		Request.SourceId = TEXT("Pacifier.FixedPIE");
		Request.DamageSource = &Player;
		Request.Instigator = &Player;
		Request.Target = &Enemy;
		Request.AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTag, false);
		Request.RawDamage = RawDamage;
		Request.ImpactOrigin = Player.GetActorLocation();
		Request.IncomingDirection = (Enemy.GetActorLocation() - Player.GetActorLocation()).GetSafeNormal();
		return Combat.ApplyDamage(Request) == EPRCombatRequestStatus::Applied;
	}

	bool ApplyToHealthRatio(
		UPRCombatSubsystem& Combat,
		APawn& Player,
		APREnemyCharacter& Enemy,
		const float TargetRatio,
		const TCHAR* AbilityTag) const
	{
		const UPRAttributeSet* Attributes = Enemy.GetAttributeSet();
		if (!Attributes || Attributes->GetMaxHealth() <= UE_SMALL_NUMBER) return false;
		const float TargetHealth = Attributes->GetMaxHealth() * TargetRatio;
		const float Damage = Attributes->GetShield() + FMath::Max(0.1f, Attributes->GetHealth() - TargetHealth);
		return ApplyDamage(Combat, Player, Enemy, Damage, AbilityTag);
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	FFixedPacifierBootstrap Bootstrap;
	TWeakObjectPtr<UPRRoomSubsystem> BoundRoom;
	FDelegateHandle RewardOfferHandle;
	FPRRewardOffer LatestOffer;
	EBossStep BossStep = EBossStep::KillVariants;
	bool bSawCombat = false;
	bool bSawEvent = false;
	bool bSawShop = false;
	bool bSawSafe = false;
	bool bSawBoss = false;
	bool bSawThreeVariants = false;
	bool bIllusionCountered = false;
	bool bLureCountered = false;
	bool bSuppressionCountered = false;
	bool bBossCompletedOnce = false;
	int32 MaxComfortPressure = 0;
	double StartedAt = FPlatformTime::Seconds();
};

class FFixedPacifierPersistenceRunner final : public TSharedFromThis<FFixedPacifierPersistenceRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedPacifierPersistenceRunner> Runner = MakeShared<FFixedPacifierPersistenceRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride() || !FailureBackend.IsValid())
		{
			Runner->Result->SetError(TEXT("PrepareFixedPacifierPIE must run before persistence-retry PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8
	{
		Bootstrap,
		Finalize,
		AwaitFailure,
		Retry,
		AwaitSuccess,
		VerifyIdempotency
	};

	bool Tick()
	{
		if (FPlatformTime::Seconds() - StartedAt > RunnerTimeoutSeconds)
		{
			UWorld* TimeoutWorld = GEditor ? GEditor->PlayWorld : nullptr;
			UGameInstance* TimeoutGameInstance = TimeoutWorld ? TimeoutWorld->GetGameInstance() : nullptr;
			const UPRRunStateSubsystem* TimeoutRun =
				TimeoutGameInstance ? TimeoutGameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
			const UPRChapterSubsystem* TimeoutChapter =
				TimeoutGameInstance ? TimeoutGameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
			FPRChapterSnapshot TimeoutSnapshot;
			if (TimeoutChapter) TimeoutChapter->GetSnapshot(TimeoutSnapshot);
			return Fail(FString::Printf(
				TEXT("Fixed Pacifier persistence retry timed out (phase=%d,runState=%d,chapterState=%d,savesUntilFailure=%d)."),
				static_cast<int32>(Phase),
				TimeoutRun ? static_cast<int32>(TimeoutRun->GetRunRuntimeState().State) : -1,
				static_cast<int32>(TimeoutSnapshot.State),
				FailureBackend.IsValid() ? FailureBackend->GetSavesUntilFailure() : -2));
		}
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UPRRunStateSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!Run || !Chapter) return Fail(TEXT("Fixed Pacifier persistence retry lost its subsystems."));

		switch (Phase)
		{
		case EPhase::Bootstrap:
		{
			FString Failure;
			const EPRAccountOperationResult BootstrapResult = Bootstrap.Tick(61204, Failure);
			if (BootstrapResult == EPRAccountOperationResult::Started) return true;
			if (BootstrapResult != EPRAccountOperationResult::Succeeded) return Fail(Failure);
			Phase = EPhase::Finalize;
			return true;
		}
		case EPhase::Finalize:
		{
			if (!Chapter->StageFixedPacifierCompletionFactsForAutomation())
				return Fail(TEXT("Could not stage the fixed Pacifier completion facts."));
			if (!bFailureArmed)
			{
				// Account finalization is the first save and Memory's bounded
				// AccountDeleted consumer is the second. Fail the following atomic
				// Chapter/Proof transaction and leave both upstream value facts durable.
				FailureBackend->FailNthUpcomingSave(3);
				bFailureArmed = true;
			}
			const bool bReportedPending =
				Run->FinalizeActiveAccountForAutomation(EPRAccountTerminationReason::RoomSequenceCompleted);
			const EPRRunLifecycleState RunState = Run->GetRunRuntimeState().State;
			if (!bReportedPending
				&& RunState != EPRRunLifecycleState::FinalizedTravelPending)
			{
				// RequestStartRun publishes RunActive before its successful persistence
				// callback clears the pending operation. Wait for that bounded transition;
				// the rejected call has no side effects. The fixed storage backend can
				// also complete synchronously, in which case FinalizedTravelPending is
				// the authoritative success observation even though the seam returns false.
				return true;
			}
			Phase = EPhase::AwaitFailure;
			return true;
		}
		case EPhase::AwaitFailure:
		{
			FPRChapterSnapshot Snapshot;
			if (!Chapter->GetSnapshot(Snapshot)) return Fail(TEXT("Could not read failure snapshot."));
			if (Snapshot.State != EPRChapterLifecycleState::ReadyToRetry) return true;
			FPRChapterCompletionResult Completion;
			if (Snapshot.bHasHumanAnomalyProof || Chapter->GetLatestCompletion(Completion))
			{
				return Fail(TEXT("Failed Pacifier save published proof state."));
			}
			Phase = EPhase::Retry;
			return true;
		}
		case EPhase::Retry:
			if (Chapter->RetryPendingSettlement() != EPRChapterOperationResult::Succeeded)
				return Fail(TEXT("Frozen Pacifier settlement retry did not start."));
			Phase = EPhase::AwaitSuccess;
			return true;
		case EPhase::AwaitSuccess:
		{
			FPRChapterSnapshot Snapshot;
			if (!Chapter->GetSnapshot(Snapshot) || Snapshot.State != EPRChapterLifecycleState::Completed) return true;
			FPRChapterCompletionResult Completion;
			if (!Snapshot.bHasHumanAnomalyProof
				|| !Chapter->GetLatestCompletion(Completion)
				|| Completion.ProofId != UPRChapterContentRegistryDataAsset::GetPacifierProofId()
				|| Completion.SettlementSequence != 3)
			{
				return Fail(TEXT("Successful retry did not publish one canonical Pacifier proof."));
			}
			Phase = EPhase::VerifyIdempotency;
			return true;
		}
		case EPhase::VerifyIdempotency:
		{
			if (Chapter->RetryPendingSettlement() != EPRChapterOperationResult::RejectedInvalidState)
				return Fail(TEXT("Duplicate Pacifier settlement retry was not rejected."));
			FPRChapterCompletionResult Completion;
			if (!Chapter->GetLatestCompletion(Completion) || Completion.SettlementSequence != 3)
				return Fail(TEXT("Duplicate retry changed Pacifier settlement sequence."));
			Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":61204,\"firstSave\":\"WriteFailed\",\"stateAfterFailure\":\"ReadyToRetry\",\"retry\":\"PASS\",\"proofCount\":1,\"settlementSequence\":3,\"duplicateRetry\":\"RejectedInvalidState\",\"userSlotsTouched\":false}"));
			return false;
		}
		}
		return Fail(TEXT("Fixed Pacifier persistence runner entered an invalid phase."));
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	FFixedPacifierBootstrap Bootstrap;
	EPhase Phase = EPhase::Bootstrap;
	bool bFailureArmed = false;
	double StartedAt = FPlatformTime::Seconds();
};
#endif
} // namespace PRPacifierAutomationToolsetPrivate

UToolCallAsyncResultString* UPRPacifierAutomationToolset::InspectFixedPacifierRegistry()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	const UPRChapterRoguelikeContentRegistryDataAsset* RoomRegistry =
		LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(
			nullptr,
			PRPacifierAutomationToolsetPrivate::PacifierRoomRegistryPath);
	const UPREnemyContentRegistryDataAsset* EnemyRegistry =
		LoadObject<UPREnemyContentRegistryDataAsset>(
			nullptr,
			PRPacifierAutomationToolsetPrivate::PacifierEnemyRegistryPath);
	if (!RoomRegistry || !EnemyRegistry)
	{
		Result->SetError(TEXT("Fixed Pacifier registries are unavailable."));
		return Result;
	}
	int32 InvalidRooms = 0;
	int32 InvalidEncounters = 0;
	int32 InvalidEvents = 0;
	int32 InvalidPolicies = 0;
	int32 InvalidRewards = 0;
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : RoomRegistry->Rooms)
	{
		const UPRRoomDataAsset* Asset = Reference.LoadSynchronous();
		InvalidRooms += !Asset || !Asset->IsRoomDefinitionValid();
	}
	for (const TSoftObjectPtr<UPREncounterDataAsset>& Reference : RoomRegistry->Encounters)
	{
		const UPREncounterDataAsset* Asset = Reference.LoadSynchronous();
		InvalidEncounters += !Asset || !Asset->IsEncounterDefinitionValid();
	}
	for (const TSoftObjectPtr<UPRRoomEventDataAsset>& Reference : RoomRegistry->Events)
	{
		const UPRRoomEventDataAsset* Asset = Reference.LoadSynchronous();
		InvalidEvents += !Asset || !Asset->IsEventDefinitionValid();
	}
	for (const TSoftObjectPtr<UPRRewardPolicyDataAsset>& Reference : RoomRegistry->RewardPolicies)
	{
		const UPRRewardPolicyDataAsset* Asset = Reference.LoadSynchronous();
		InvalidPolicies += !Asset || !Asset->IsPolicyDefinitionValid();
	}
	for (const TSoftObjectPtr<UPRRewardDataAsset>& Reference : RoomRegistry->Rewards)
	{
		const UPRRewardDataAsset* Asset = Reference.LoadSynchronous();
		InvalidRewards += !Asset || !Asset->IsRewardDefinitionValid();
	}
	TSet<FName> EnemyNames;
	for (const FPREnemyContentRegistryEntry& Entry : EnemyRegistry->Entries)
	{
		if (Entry.PrototypeId.PrimaryAssetType == FPrimaryAssetType(TEXT("ProjectREnemy")))
		{
			EnemyNames.Add(Entry.PrototypeId.PrimaryAssetName);
		}
	}
	const bool bExactEnemyIds =
		EnemyNames.Num() == 4
		&& EnemyNames.Contains(TEXT("DA_Enemy_PacifierComfortGuard"))
		&& EnemyNames.Contains(TEXT("DA_Enemy_PacifierMirageEmitter"))
		&& EnemyNames.Contains(TEXT("DA_Enemy_PacifierRiskSuppressor"))
		&& EnemyNames.Contains(TEXT("DA_Boss_Pacifier"));
	Result->SetValue(FString::Printf(
		TEXT("{\"roomRegistryReady\":%s,\"enemyRegistryReady\":%s,\"rooms\":%d,\"encounters\":%d,\"events\":%d,\"policies\":%d,\"rewards\":%d,\"enemyEntries\":%d,\"exactEnemyIds\":%s,\"invalidRooms\":%d,\"invalidEncounters\":%d,\"invalidEvents\":%d,\"invalidPolicies\":%d,\"invalidRewards\":%d}"),
		RoomRegistry->IsRegistryReady() ? TEXT("true") : TEXT("false"),
		EnemyRegistry->IsRegistryReady() ? TEXT("true") : TEXT("false"),
		RoomRegistry->Rooms.Num(),
		RoomRegistry->Encounters.Num(),
		RoomRegistry->Events.Num(),
		RoomRegistry->RewardPolicies.Num(),
		RoomRegistry->Rewards.Num(),
		EnemyRegistry->Entries.Num(),
		bExactEnemyIds ? TEXT("true") : TEXT("false"),
		InvalidRooms,
		InvalidEncounters,
		InvalidEvents,
		InvalidPolicies,
		InvalidRewards));
	return Result;
}

UToolCallAsyncResultString* UPRPacifierAutomationToolset::PrepareFixedPacifierPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld)
	{
		Result->SetError(TEXT("PrepareFixedPacifierPIE must run before PIE starts."));
		return Result;
	}
	const FString Base = FString::Printf(
		TEXT("ProjectR_Automation_%s"),
		*FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	PRPacifierAutomationToolsetPrivate::FailureBackend =
		MakeShared<PRPacifierAutomationToolsetPrivate::FFixedFailureStorageBackend>();
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(
		Base,
		PRPacifierAutomationToolsetPrivate::FailureBackend.ToSharedRef());
	if (!Storage)
	{
		PRPacifierAutomationToolsetPrivate::FailureBackend.Reset();
		Result->SetError(TEXT("Could not create isolated Pacifier automation A/B storage."));
		return Result;
	}
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"failureBackend\":\"fixed-nth-save-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Pacifier PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRPacifierAutomationToolset::RunFixedPacifierSelectionPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRPacifierAutomationToolsetPrivate::FFixedPacifierSelectionRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->SetError(TEXT("Unavailable."));
	return Result;
#endif
}

UToolCallAsyncResultString* UPRPacifierAutomationToolset::RunFixedPacifierFullPathPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRPacifierAutomationToolsetPrivate::FFixedPacifierFullPathRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->SetError(TEXT("Unavailable."));
	return Result;
#endif
}

UToolCallAsyncResultString* UPRPacifierAutomationToolset::RunFixedPacifierPersistenceRetryPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRPacifierAutomationToolsetPrivate::FFixedPacifierPersistenceRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->SetError(TEXT("Unavailable."));
	return Result;
#endif
}

UToolCallAsyncResultString* UPRPacifierAutomationToolset::CleanupFixedPacifierPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld)
	{
		Result->SetError(TEXT("CleanupFixedPacifierPIE must run after PIE stops."));
		return Result;
	}
	UPRSaveSubsystem::CleanupAutomationStorageOverride();
	PRPacifierAutomationToolsetPrivate::FailureBackend.Reset();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"automationStorageCleaned\":true,\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Unavailable."));
#endif
	return Result;
}
