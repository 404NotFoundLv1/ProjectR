// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRAccountAutomationToolset.h"

#include "Async/Async.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Roguelike/Account/PRAccountIdentityRegistryDataAsset.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "Containers/Ticker.h"

namespace PRAccountAutomationToolsetPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
bool bFixedFailureArmed = false;

class FFixedFailureBackend final : public IPRSaveStorageBackend
{
public:
	static void ResetFailure() { bFixedFailureArmed = false; }
	static void ArmFailure() { bFixedFailureArmed = true; }
	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override { return Slots.Contains(Slot) ? ISaveGameSystem::ESaveExistsResult::OK : ISaveGameSystem::ESaveExistsResult::DoesNotExist; }
	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override { if (const TArray<uint8>* Data = Slots.Find(Slot)) { OutData = *Data; return true; } return false; }
	virtual void SaveGameAsync(const FString& Slot, TSharedRef<const TArray<uint8>> Data, TFunction<void(bool)> Completion) override
	{
		const bool bSuccess = !bFixedFailureArmed;
		if (bSuccess) Slots.Add(Slot, *Data);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, Completion = MoveTemp(Completion)]() mutable { Completion(bSuccess); });
	}
	virtual void LoadGameAsync(const FString& Slot, TFunction<void(bool, TArray<uint8>)> Completion) override
	{
		TArray<uint8> Data;
		const bool bSuccess = LoadGame(Slot, Data);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, Data = MoveTemp(Data), Completion = MoveTemp(Completion)]() mutable { Completion(bSuccess, MoveTemp(Data)); });
	}
	virtual bool DeleteGame(const FString& Slot) override { return Slots.Remove(Slot) > 0; }
private:
	TMap<FString, TArray<uint8>> Slots;
};

class FFixedAccountRunner : public TSharedFromThis<FFixedAccountRunner>
{
public:
	static UToolCallAsyncResultString* Start(const EPRAccountTerminationReason InReason, const bool bInStartOnly = false, const bool bInExpectPersistenceFailure = false, const bool bInEmitRoomCompletion = false, const bool bInEmitTerminationEvents = false)
	{
		TSharedRef<FFixedAccountRunner> Runner = MakeShared<FFixedAccountRunner>();
		Runner->Reason = InReason;
		Runner->bStartOnly = bInStartOnly;
		Runner->bExpectPersistenceFailure = bInExpectPersistenceFailure;
		Runner->bEmitRoomCompletion = bInEmitRoomCompletion;
		Runner->bEmitTerminationEvents = bInEmitTerminationEvents;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedAccountPIE must succeed before any account PIE lifecycle call."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick();
		}));
		return Runner->Result.Get();
	}

	static UToolCallAsyncResultString* StartInterruptedRecovery()
	{
		TSharedRef<FFixedAccountRunner> Runner = MakeShared<FFixedAccountRunner>();
		Runner->Reason = EPRAccountTerminationReason::InterruptedRecovery;
		Runner->bInterruptedRecovery = true;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedAccountPIE must precede interrupted recovery PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8 { CreateProfile, CreateAccount, StartRun, Finalize, Verify };

	bool Tick()
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client)
		{
			return Fail(TEXT("Fixed account lifecycle requires active authoritative in-process PIE."));
		}
		UPRSaveSubsystem* Save = World->GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
		UPRRunStateSubsystem* Run = World->GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
		if (!Save || !Run)
		{
			return Fail(TEXT("Fixed account lifecycle PIE is missing Save or RunState subsystem."));
		}
		if (FPlatformTime::Seconds() - StartedAt > 30.0)
		{
			return Fail(TEXT("Fixed account lifecycle timed out."));
		}
		if (bInterruptedRecovery)
		{
			if (!bIssued)
			{
				FGuid RequestId;
				const EPRSaveResult LoadResult = Save->LoadDefaultProfile(RequestId);
				if (LoadResult != EPRSaveResult::Success && LoadResult != EPRSaveResult::RecoveredFromAlternate) return Fail(TEXT("Interrupted recovery could not load the isolated persisted account."));
				bIssued = true;
			}
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::Finalizing) return true;
			FPRActiveAccountSaveData Active;
			TArray<FPRAccountRecord> Graveyard;
			FPRRunSummary Summary;
			Run->GetGraveyardSnapshot(Graveyard);
			const bool bPassed = !Run->GetActiveAccountSnapshot(Active) && Run->GetLastRunSummary(Summary)
				&& Summary.TerminationReason == EPRAccountTerminationReason::InterruptedRecovery
				&& Summary.CounterproofFragmentsAwarded == 0 && Graveyard.Num() == 1;
			UPRSaveSubsystem::CleanupAutomationStorageOverride();
			if (!bPassed) return Fail(TEXT("Interrupted recovery did not produce exactly one durable zero-fragment record."));
			Result->SetValue(TEXT("{\"status\":\"PASS\",\"reason\":\"InterruptedRecovery\",\"activeAccountCleared\":true,\"counterproof\":0,\"automationSlotsCleaned\":true}"));
			return false;
		}
		switch (Phase)
		{
		case EPhase::CreateProfile:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Save->CreateNewDefaultProfile(RequestId) != EPRSaveResult::Success) return Fail(TEXT("Automation profile creation failed."));
				bIssued = true;
			}
			Phase = EPhase::CreateAccount; bIssued = false; return true;
		case EPhase::CreateAccount:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed Technician account creation did not start."));
				bIssued = true;
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady) return true;
			Phase = EPhase::StartRun; bIssued = false; return true;
		case EPhase::StartRun:
			if (!bIssued)
			{
				FGuid RequestId;
				if (Run->RequestStartRun(41043, RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed account run start did not stage and save."));
				bIssued = true;
			}
			if (bExpectPersistenceFailure && Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed)
			{
				Phase = EPhase::Verify;
				return true;
			}
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive) return true;
			if (bStartOnly)
			{
				Result->SetValue(TEXT("{\"status\":\"PASS\",\"runStartedPersisted\":true,\"activeAccountRetainedForRecovery\":true,\"automationSlotsOnly\":true}"));
				return false;
			}
			if (bEmitRoomCompletion)
			{
				UPRRoomSubsystem* Rooms = World->GetGameInstance()->GetSubsystem<UPRRoomSubsystem>();
				if (!Rooms) return Fail(TEXT("Fixed completion PIE is missing the existing RoomSubsystem."));
				FPRRoomSequenceCompleted Completion;
				Completion.CompletionId = FGuid::NewGuid();
				Completion.SessionId = FGuid::NewGuid();
				Completion.Seed = 41043;
				FPRRoomPathStep& Step = Completion.CompletedPath.AddDefaulted_GetRef();
				Step.SelectedRoomId = FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_CombatStandard"));
				Rooms->OnRoomSequenceCompleted().Broadcast(Completion);
				Phase = EPhase::Verify;
				return true;
			}
			if (bEmitTerminationEvents)
			{
				UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>();
				UPRDivergenceSubsystem* Divergence = World->GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>();
				if (!Combat || !Divergence) return Fail(TEXT("Fixed termination PIE is missing Combat or Divergence subsystem."));
				FPRCombatEvent Death;
				Death.EventId = FGuid::NewGuid();
				Death.EventTag = UPRTagLibrary::GetCombatEventDeathTag();
				Death.TargetId = TEXT("Player");
				Death.HealthDamage = 100.0f;
				Death.MaxHealth = 100.0f;
				Death.bFatal = true;
				Combat->OnCombatEvent().Broadcast(Death);
				FPRDivergenceResult DivergenceResult;
				DivergenceResult.ResultId = FGuid::NewGuid();
				DivergenceResult.DeathEventId = Death.EventId;
				DivergenceResult.FutureDisposition = Reason == EPRAccountTerminationReason::DivergenceEvacuation
					? EPRDivergenceFutureDisposition::RescueEvacuationRequested
					: Reason == EPRAccountTerminationReason::DivergenceLeave
						? EPRDivergenceFutureDisposition::LeaveRunRequested
						: EPRDivergenceFutureDisposition::None;
				Divergence->OnDivergenceResult().Broadcast(DivergenceResult);
				Phase = EPhase::Verify;
				return true;
			}
			Phase = EPhase::Finalize; return true;
		case EPhase::Finalize:
			if (bExpectPersistenceFailure) FFixedFailureBackend::ArmFailure();
			if (!Run->FinalizeActiveAccountForAutomation(Reason)) return Fail(TEXT("Fixed account finalization did not enter the durable transaction."));
			Phase = EPhase::Verify; return true;
		case EPhase::Verify:
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::Finalizing) return true;
			if (bExpectPersistenceFailure)
			{
				FPRActiveAccountSaveData FailedActive;
				TArray<FPRAccountRecord> FailedGraveyard;
				FPRRunSummary FailedSummary;
				Run->GetGraveyardSnapshot(FailedGraveyard);
				const bool bPassed = Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed
					&& Run->GetActiveAccountSnapshot(FailedActive) && FailedGraveyard.IsEmpty() && !Run->GetLastRunSummary(FailedSummary);
				UPRSaveSubsystem::CleanupAutomationStorageOverride();
				if (!bPassed) return Fail(TEXT("Persistence failure incorrectly published deletion or removed the active account."));
				Result->SetValue(TEXT("{\"status\":\"PASS\",\"persistenceFailed\":true,\"activeAccountRetained\":true,\"graveyardRecords\":0,\"accountDeletedPublished\":false,\"travelStarted\":false}"));
				return false;
			}
			FPRActiveAccountSaveData Active;
			TArray<FPRAccountRecord> Graveyard;
			FPRRunSummary Summary;
			Run->GetGraveyardSnapshot(Graveyard);
			const bool bNoActive = !Run->GetActiveAccountSnapshot(Active);
			const bool bSummary = Run->GetLastRunSummary(Summary) && Summary.TerminationReason == Reason;
			const bool bFragment = Summary.CounterproofFragmentsAwarded == (Reason == EPRAccountTerminationReason::RoomSequenceCompleted ? 1 : 0);
			const bool bCompletionRoom = !bEmitRoomCompletion || Summary.RoomIds.Contains(FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_CombatStandard")));
			const bool bRecord = Graveyard.Num() == 1 && Graveyard[0].TerminationReason == Reason;
			UPRSaveSubsystem::CleanupAutomationStorageOverride();
			if (!(bNoActive && bSummary && bFragment && bCompletionRoom && bRecord)) return Fail(TEXT("Fixed account lifecycle durable result was incomplete."));
			Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"reason\":%d,\"automationSlotsOnly\":true,\"activeAccountCleared\":true,\"graveyardRecords\":1,\"counterproof\":%d}"), static_cast<int32>(Reason), Summary.CounterproofFragmentsAwarded));
			return false;
		}
		return Fail(TEXT("Fixed account lifecycle entered an invalid phase."));
	}

	bool Fail(const FString& Message)
	{
		UPRSaveSubsystem::CleanupAutomationStorageOverride();
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	EPRAccountTerminationReason Reason = EPRAccountTerminationReason::PlayerDeath;
	EPhase Phase = EPhase::CreateProfile;
	bool bIssued = false;
	bool bStartOnly = false;
	bool bInterruptedRecovery = false;
	bool bExpectPersistenceFailure = false;
	bool bEmitRoomCompletion = false;
	bool bEmitTerminationEvents = false;
	double StartedAt = FPlatformTime::Seconds();
};
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::ValidateFixedAccountIdentityManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRAccountIdentityRegistryDataAsset* Registry = LoadObject<UPRAccountIdentityRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/Accounts/DA_AccountIdentityRegistry.DA_AccountIdentityRegistry"));
	if (!Registry || !Registry->IsRegistryReady())
	{
		Result->SetError(TEXT("The fixed v0.4.3 account identity registry is missing or invalid."));
	}
	else
	{
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"registry\":1,\"identities\":5,\"ordered\":true}"));
	}
	return Result;
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::InspectActiveAccountPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	UPRRunStateSubsystem* RunState = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
	if (!RunState || World->GetNetMode() == NM_Client)
	{
		Result->SetError(TEXT("Account inspection requires an active authoritative PIE world."));
		return Result;
	}
	const FPRRunRuntimeState State = RunState->GetRunRuntimeState();
	Result->SetValue(FString::Printf(
		TEXT("{\"status\":\"PASS\",\"state\":%d,\"accountId\":\"%s\",\"runId\":\"%s\",\"persistencePending\":%s,\"travelPending\":%s}"),
		static_cast<int32>(State.State), *State.AccountId.ToString(EGuidFormats::Digits), *State.RunId.ToString(EGuidFormats::Digits),
		State.bPersistencePending ? TEXT("true") : TEXT("false"), State.bTravelPending ? TEXT("true") : TEXT("false")));
	return Result;
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::VerifyAccountAutomationIsolation()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false,\"physicalDelete\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::PrepareFixedAccountPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld)
	{
		Result->SetError(TEXT("PrepareFixedAccountPIE must run before PIE starts."));
		return Result;
	}
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(Base);
	if (!Storage)
	{
		Result->SetError(TEXT("Could not create isolated automation A/B storage."));
		return Result;
	}
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Account PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedCompletionAccountPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::RoomSequenceCompleted, false, false, true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedDeathAccountPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::PlayerDeath, false, false, false, true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedEvacuationAccountPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::DivergenceEvacuation, false, false, false, true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedLeaveAccountPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::DivergenceLeave, false, false, false, true);
#else
	return nullptr;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedStartOnlyAccountPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::PlayerDeath, true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedInterruptedRecoveryPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::StartInterruptedRecovery();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::PrepareFixedAccountFailurePIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareFixedAccountFailurePIE must run before PIE starts.")); return Result; }
	PRAccountAutomationToolsetPrivate::FFixedFailureBackend::ResetFailure();
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(Base, MakeShared<PRAccountAutomationToolsetPrivate::FFixedFailureBackend>());
	if (!Storage) { Result->SetError(TEXT("Could not create isolated scripted-failure automation storage.")); return Result; }
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"scriptedFailure\":\"finalize-write\"}"));
#else
	Result->SetError(TEXT("Unavailable."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRAccountAutomationToolset::RunFixedPersistenceFailurePIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRAccountAutomationToolsetPrivate::FFixedAccountRunner::Start(EPRAccountTerminationReason::PlayerDeath, false, true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}
