// Copyright ProjectR. All Rights Reserved.

#include "PRWardenAutomationToolset.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Enemies/PREnemySubsystem.h"
#include "Editor.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "Containers/Ticker.h"

namespace PRWardenAutomationToolsetPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
class FFixedWardenSelectionRunner final : public TSharedFromThis<FFixedWardenSelectionRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedWardenSelectionRunner> Runner = MakeShared<FFixedWardenSelectionRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedWardenPIE must run before this fixed PIE check."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8 { CreateProfile, StageAllocatorProof, CreateAccount, StartRun, Verify };

	bool Tick()
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client) return Fail(TEXT("Warden selection requires active authoritative in-process PIE."));
		if (FPlatformTime::Seconds() - StartedAt > 30.0) return Fail(TEXT("Fixed Warden selection timed out."));
		UGameInstance* GameInstance = World->GetGameInstance();
		UPRSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UPRSaveSubsystem>() : nullptr;
		UPRRunStateSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRChapterSubsystem* Chapter = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
		if (!Save || !Run || !Chapter) return Fail(TEXT("Fixed Warden PIE is missing Save, RunState, or Chapter subsystem."));

		switch (Phase)
		{
		case EPhase::CreateProfile:
			if (!bIssued) { FGuid RequestId; if (Save->CreateNewDefaultProfile(RequestId) != EPRSaveResult::Success) return Fail(TEXT("Could not create the isolated Warden fixture profile.")); bIssued = true; }
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready || !Save->GetSaveRuntimeState().bHasLoadedProfile) return true;
			Phase = EPhase::StageAllocatorProof; bIssued = false; return true;
		case EPhase::StageAllocatorProof:
			if (!bIssued) { if (!Chapter->StageFixedAllocatorProofForAutomation()) return Fail(TEXT("Could not durably stage the fixed Allocator proof.")); bIssued = true; return true; }
			if (Save->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready || Save->GetSaveRuntimeState().bSaveRequestQueued) return true;
			Phase = EPhase::CreateAccount; bIssued = false; return true;
		case EPhase::CreateAccount:
			if (!bIssued) { FGuid RequestId; if (Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed Warden account creation did not start.")); bIssued = true; }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady) return true;
			Phase = EPhase::StartRun; bIssued = false; return true;
		case EPhase::StartRun:
			if (!bIssued) { FGuid RequestId; if (Run->RequestStartRun(61101, RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed Warden run did not start.")); bIssued = true; }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive) return true;
			Phase = EPhase::Verify; return true;
		case EPhase::Verify:
			FPRChapterSnapshot Snapshot;
			if (!Chapter->GetSnapshot(Snapshot) || Snapshot.ContentId != UPRChapterContentRegistryDataAsset::GetWardenContentId() || Snapshot.DirectiveId != TEXT("Warden.RouteForewarning"))
			{
				return Fail(FString::Printf(TEXT("Allocator proof did not select the fixed Warden seed-61101 definition (content=%s,directive=%s,state=%d,fallback=%s)."), *Snapshot.ContentId.ToString(), *Snapshot.DirectiveId.ToString(), static_cast<int32>(Snapshot.State), *Snapshot.FallbackReason.ToString()));
			}
			if (!UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId()).IsValid())
			{
				return Fail(TEXT("The fixed Warden Enemy Registry was not registered before the first chapter room could spawn a closed PrimaryAssetId enemy."));
			}
			Result->SetValue(TEXT("{\"status\":\"PASS\",\"content\":\"Warden\",\"seed\":61101,\"directive\":\"Warden.RouteForewarning\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"));
			return false;
		}
		return Fail(TEXT("Fixed Warden selection entered an invalid phase."));
	}

	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	EPhase Phase = EPhase::CreateProfile;
	bool bIssued = false;
	double StartedAt = FPlatformTime::Seconds();
};

class FFixedWardenInitialCombatRunner final : public TSharedFromThis<FFixedWardenInitialCombatRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedWardenInitialCombatRunner> Runner = MakeShared<FFixedWardenInitialCombatRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride()) { Runner->Result->SetError(TEXT("PrepareFixedWardenPIE must run before this fixed combat check.")); return Runner->Result.Get(); }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8 { SelectCombat, AwaitEncounter };

	bool Tick()
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client) return Fail(TEXT("Fixed Warden combat check requires active authoritative in-process PIE."));
		if (FPlatformTime::Seconds() - StartedAt > 30.0) return Fail(TEXT("Fixed Warden initial combat timed out."));
		UGameInstance* GameInstance = World->GetGameInstance();
		UPRRoomSubsystem* Room = GameInstance ? GameInstance->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		if (!Room) return Fail(TEXT("Fixed Warden combat check is missing RoomSubsystem."));
		FPRRoomRuntimeState State;
		if (!Room->GetRoomRuntimeState(State)) return Fail(TEXT("Fixed Warden combat check could not read room state."));
		if (Phase == EPhase::SelectCombat)
		{
			if (State.FlowStatus != EPRRoomFlowStatus::SelectingRoom || !State.Path.IsValidIndex(0) || State.Path[0].CandidateRoomIds.IsEmpty())
			{
				FString Candidates;
				if (State.Path.IsValidIndex(0)) for (const FPrimaryAssetId& Candidate : State.Path[0].CandidateRoomIds) { if (!Candidates.IsEmpty()) Candidates += TEXT(","); Candidates += Candidate.ToString(); }
				return Fail(FString::Printf(TEXT("Seed 61101 did not expose a closed first Warden combat candidate (status=%d,candidates=%s)."), static_cast<int32>(State.FlowStatus), *Candidates));
			}
			SelectedRoomId = State.Path[0].CandidateRoomIds[0]; // Candidates are canonical PrimaryAssetId-sorted fixed Warden content.
			if (Room->SelectRoom(SelectedRoomId) != EPRRoomOperationResult::Succeeded) return Fail(TEXT("The fixed first Warden combat room was rejected."));
			Phase = EPhase::AwaitEncounter;
			return true;
		}
		if (State.FlowStatus == EPRRoomFlowStatus::Cancelled) return Fail(TEXT("The fixed first Warden combat room cancelled during travel or spawn."));
		if (State.FlowStatus != EPRRoomFlowStatus::EncounterActive) return true;
		UPREnemySubsystem* Enemies = World->GetSubsystem<UPREnemySubsystem>();
		if (!Enemies || !Enemies->IsRegistryReady() || Enemies->GetConfiguredContentRegistryId() != UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId()) return Fail(TEXT("The fixed Warden Enemy Registry was not active for the first actual combat room."));
		Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"seed\":61101,\"room\":\"%s\",\"encounter\":\"active\",\"enemyRegistry\":\"Warden\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"), *SelectedRoomId.ToString()));
		return false;
	}

	bool Fail(const FString& Message) { Result->SetError(Message); return false; }
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	EPhase Phase = EPhase::SelectCombat;
	FPrimaryAssetId SelectedRoomId;
	double StartedAt = FPlatformTime::Seconds();
};
#endif
}

UToolCallAsyncResultString* UPRWardenAutomationToolset::InspectFixedWardenRegistry()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_RoguelikeContentRegistry_Warden.DA_RoguelikeContentRegistry_Warden"));
	if (!Registry) { Result->SetError(TEXT("Fixed Warden registry asset is unavailable.")); return Result; }
	auto CountInvalid = [](const auto& References, auto Validate)
	{
		int32 Invalid = 0;
		for (const auto& Reference : References) { const auto* Asset = Reference.LoadSynchronous(); if (!Asset || !(Asset->*Validate)()) ++Invalid; }
		return Invalid;
	};
	const int32 InvalidRooms = CountInvalid(Registry->Rooms, &UPRRoomDataAsset::IsRoomDefinitionValid);
	const int32 InvalidEncounters = CountInvalid(Registry->Encounters, &UPREncounterDataAsset::IsEncounterDefinitionValid);
	const int32 InvalidEvents = CountInvalid(Registry->Events, &UPRRoomEventDataAsset::IsEventDefinitionValid);
	const int32 InvalidPolicies = CountInvalid(Registry->RewardPolicies, &UPRRewardPolicyDataAsset::IsPolicyDefinitionValid);
	const int32 InvalidRewards = CountInvalid(Registry->Rewards, &UPRRewardDataAsset::IsRewardDefinitionValid);
	int32 InvalidRules = 0;
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : Registry->ChapterRules) { const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous(); if (!Rule || !Rule->IsRuleDefinitionValid()) ++InvalidRules; }
	int32 MissingPreferredRooms = 0, InvalidPressureBindings = 0, InvalidRoomClosure = 0, InvalidEventRoomBindings = 0, InvalidPolicyRewards = 0, ShopRooms = 0;
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : Registry->ChapterRules) { if (const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous()) for (const FPrimaryAssetId& Id : Rule->PreferredRoomIds) if (!Registry->FindRoom(Id)) ++MissingPreferredRooms; }
	for (const FPRChapterEventPressureBinding& Binding : Registry->EventPressureBindings) { if (!Registry->FindEvent(Binding.EventId) || Binding.ChoiceId.IsNone() || FMath::Abs(Binding.PressureDelta) > 1) ++InvalidPressureBindings; }
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Registry->Rooms) { const UPRRoomDataAsset* Room = Reference.LoadSynchronous(); if (!Room || !Registry->FindEncounter(Room->EncounterId) || !Registry->FindPolicy(Room->RewardPolicyId)) ++InvalidRoomClosure; else if (Room->TypeTag.ToString() == TEXT("Room.Type.Shop")) ++ShopRooms; }
	for (const FPRRoomEventBinding& Binding : Registry->EventRoomBindings) if (!Registry->FindRoom(Binding.RoomId) || !Registry->FindEvent(Binding.EventId)) ++InvalidEventRoomBindings;
	for (const TSoftObjectPtr<UPRRewardPolicyDataAsset>& Reference : Registry->RewardPolicies) if (const UPRRewardPolicyDataAsset* Policy = Reference.LoadSynchronous()) for (const FPrimaryAssetId& Id : Policy->RewardIds) if (!Registry->FindReward(Id)) ++InvalidPolicyRewards;
	Result->SetValue(FString::Printf(TEXT("{\"registryReady\":%s,\"invalidRooms\":%d,\"invalidEncounters\":%d,\"invalidEvents\":%d,\"invalidPolicies\":%d,\"invalidRewards\":%d,\"invalidRules\":%d,\"missingPreferredRooms\":%d,\"invalidPressureBindings\":%d,\"invalidRoomClosure\":%d,\"invalidEventRoomBindings\":%d,\"invalidPolicyRewards\":%d,\"shopRooms\":%d}"), Registry->IsRegistryReady() ? TEXT("true") : TEXT("false"), InvalidRooms, InvalidEncounters, InvalidEvents, InvalidPolicies, InvalidRewards, InvalidRules, MissingPreferredRooms, InvalidPressureBindings, InvalidRoomClosure, InvalidEventRoomBindings, InvalidPolicyRewards, ShopRooms));
	return Result;
}

UToolCallAsyncResultString* UPRWardenAutomationToolset::PrepareFixedWardenPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareFixedWardenPIE must run before PIE starts.")); return Result; }
	// The Save policy deliberately admits only this exact automation prefix; the
	// Warden distinction is represented by this no-argument toolset, not a slot.
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(Base);
	if (!Storage) { Result->SetError(TEXT("Could not create isolated Warden automation A/B storage.")); return Result; }
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Warden PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRWardenAutomationToolset::RunFixedWardenSelectionPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRWardenAutomationToolsetPrivate::FFixedWardenSelectionRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRWardenAutomationToolset::RunFixedWardenInitialCombatPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRWardenAutomationToolsetPrivate::FFixedWardenInitialCombatRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRWardenAutomationToolset::CleanupFixedWardenPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("CleanupFixedWardenPIE must run after PIE stops.")); return Result; }
	UPRSaveSubsystem::CleanupAutomationStorageOverride();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"automationStorageCleaned\":true,\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Unavailable."));
#endif
	return Result;
}
