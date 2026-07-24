// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/PRDialogueTypes.h"

#include "Companions/PRCompanionDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Dialogue/PRCompanionDialogueSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueSelectionContractTest,
	"ProjectR.Dialogue.Selection.PriorityCooldownAndSafeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDialogueSelectionContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Critical-low-health has the highest fixed priority"),
		FPRDialogueContract::GetTriggerPriority(EPRDialogueTriggerKind::CriticalLowHealth), 100);
	TestEqual(TEXT("Critical-low-health cooldown"), FPRDialogueContract::GetTriggerCooldown(EPRDialogueTriggerKind::CriticalLowHealth), 12.0f);
	TestEqual(TEXT("Safe-state cooldown"), FPRDialogueContract::GetTriggerCooldown(EPRDialogueTriggerKind::SafeCombatCleared), 30.0f);
	TestTrue(TEXT("Only safe combat cleared permits a choice"), FPRDialogueContract::IsSafeStateTrigger(EPRDialogueTriggerKind::SafeCombatCleared));
	TestFalse(TEXT("Combat barks do not permit a choice"), FPRDialogueContract::IsSafeStateTrigger(EPRDialogueTriggerKind::QTESuccess));

	FPRDialogueRequest Earlier;
	Earlier.Priority = 60;
	Earlier.WorldTimeSeconds = 4.0;
	Earlier.LineId = TEXT("A");
	FPRDialogueRequest Later = Earlier;
	Later.WorldTimeSeconds = 5.0;
	Later.LineId = TEXT("Z");
	FPRDialogueRequest Higher = Later;
	Higher.Priority = 90;
	TestTrue(TEXT("Queue orders higher priority first"), FPRDialogueContract::IsRequestBefore(Higher, Earlier));
	TestTrue(TEXT("Queue orders equal priority by event time"), FPRDialogueContract::IsRequestBefore(Earlier, Later));
	Later.WorldTimeSeconds = Earlier.WorldTimeSeconds;
	TestTrue(TEXT("Queue orders equal timestamps by line id"), FPRDialogueContract::IsRequestBefore(Earlier, Later));
	TestTrue(TEXT("Recent event remains deduplicated for ten seconds"), FPRDialogueContract::IsWithinEventDedupWindow(5.0, 15.0));
	TestFalse(TEXT("Event can be reused after the dedup window"), FPRDialogueContract::IsWithinEventDedupWindow(5.0, 15.01));
	TestEqual(TEXT("Minimum spacing returns precise remaining delay"), FPRDialogueContract::GetDeferredStartDelay({ 9.5 }, 10.0), 0.75);
	TestEqual(TEXT("Three starts return their ten-second cap delay"), FPRDialogueContract::GetDeferredStartDelay({ 1.0, 5.0, 8.0 }, 10.0), 1.0);
	TestFalse(TEXT("Queued request remains valid at its three-second expiry boundary"), FPRDialogueContract::IsQueuedRequestExpired(5.0, 8.0));
	TestTrue(TEXT("Queued request expires after three seconds"), FPRDialogueContract::IsQueuedRequestExpired(5.0, 8.01));
	const FName AxiomCooldownKey = FPRDialogueContract::GetLineCooldownKey(FPRCompanionContract::AxiomTag(), TEXT("CriticalLowHealth"));
	const FName KindleCooldownKey = FPRDialogueContract::GetLineCooldownKey(FPRCompanionContract::KindleTag(), TEXT("CriticalLowHealth"));
	TestNotEqual(TEXT("Per-speaker cooldowns do not collide for the same line identity"), AxiomCooldownKey, KindleCooldownKey);
	return true;
}

namespace PRDialogueSelectionAutomation
{
class FImmediateAutomationSaveBackend final : public IPRSaveStorageBackend
{
public:
	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override
	{
		return Slots.Contains(Slot) ? ISaveGameSystem::ESaveExistsResult::OK : ISaveGameSystem::ESaveExistsResult::DoesNotExist;
	}

	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override
	{
		if (const TArray<uint8>* Data = Slots.Find(Slot))
		{
			OutData = *Data;
			return true;
		}
		return false;
	}

	virtual void SaveGameAsync(const FString& Slot, TSharedRef<const TArray<uint8>> Data, TFunction<void(bool)> Completion) override
	{
		++SaveCalls;
		Slots.Add(Slot, *Data);
		Completion(true);
	}

	virtual void LoadGameAsync(const FString& Slot, TFunction<void(bool, TArray<uint8>)> Completion) override
	{
		TArray<uint8> Data;
		Completion(LoadGame(Slot, Data), MoveTemp(Data));
	}

	virtual bool DeleteGame(const FString& Slot) override
	{
		return Slots.Remove(Slot) > 0;
	}

	int32 SaveCalls = 0;
	TMap<FString, TArray<uint8>> Slots;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueRuntimeChoiceTest,
	"ProjectR.Dialogue.Selection.AppliesOneDeltaAndRequestsOneAutomationSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDialogueRuntimeChoiceTest::RunTest(const FString& Parameters)
{
	using namespace PRDialogueSelectionAutomation;

	const TSharedRef<FImmediateAutomationSaveBackend> Backend = MakeShared<FImmediateAutomationSaveBackend>();
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UPRSaveSubsystem* Save = NewObject<UPRSaveSubsystem>(GameInstance);
	Save->State = EPRSaveSubsystemState::Ready;
	const FString AutomationBase = FString::Printf(
		TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	Save->Storage = FPRSaveStorage::CreateAutomation(AutomationBase, Backend);
	if (!TestTrue(TEXT("Dialogue fixture obtains an exact automation-only A/B storage pair"), Save->Storage.IsValid()))
	{
		return false;
	}
	FGuid CreateRequestId;
	TestEqual(TEXT("Automation-only profile is created"), Save->CreateNewDefaultProfile(CreateRequestId), EPRSaveResult::Success);

	UPRCompanionSubsystem* Companions = NewObject<UPRCompanionSubsystem>(GameInstance);
	Companions->bRegistryReady = true;
	Companions->SaveSubsystem = Save;
	Companions->RelationshipRecords = FPRCompanionContract::BuildDefaultRelationshipRecords();
	for (const FGameplayTag& CompanionId : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		UPRCompanionDataAsset* Asset = NewObject<UPRCompanionDataAsset>(Companions);
		Asset->CompanionId = CompanionId;
		Companions->LoadedCompanions.Add(CompanionId, Asset);
	}

	// The dialogue fixture only needs a valid World outer for the WorldSubsystem's
	// timer-cleanup path; registering a game World would leak editor subsystems.
	UWorld* World = NewObject<UWorld>();
	UPRCompanionDialogueSubsystem* Dialogue = NewObject<UPRCompanionDialogueSubsystem>(World);
	Dialogue->CompanionSubsystem = Companions;
	Dialogue->SaveSubsystem = Save;
	Dialogue->RuntimeState.RequestId = FGuid::NewGuid();
	Dialogue->RuntimeState.CompanionId = FPRCompanionContract::AxiomTag();
	Dialogue->RuntimeState.PresentationState = EPRDialoguePresentationState::Choice;
	FPRDialogueChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Acknowledge");
	Choice.RelationshipDelta.CompanionId = FPRCompanionContract::AxiomTag();
	Choice.RelationshipDelta.SourceId = TEXT("Dialogue.Acknowledge");
	Choice.RelationshipDelta.TrustDelta = 1;
	Choice.RelationshipDelta.AffectionDelta = 1;
	Dialogue->RuntimeState.Choices.Add(Choice);
	FPRDialogueChoiceDefinition AnalyzeChoice = Choice;
	AnalyzeChoice.ChoiceId = TEXT("Analyze");
	AnalyzeChoice.RelationshipDelta.SourceId = TEXT("Dialogue.Analyze");
	AnalyzeChoice.RelationshipDelta.AffectionDelta = 0;
	AnalyzeChoice.RelationshipDelta.EvaluationDelta = 1;
	Dialogue->RuntimeState.Choices.Add(AnalyzeChoice);

	FPRDialogueResult PublishedResult;
	Dialogue->OnDialogueResult().AddLambda([&PublishedResult](const FPRDialogueResult& Result) { PublishedResult = Result; });
	TestTrue(TEXT("The active safe choice is accepted once"), Dialogue->SubmitChoice(false));
	TestEqual(TEXT("Choice publishes an applied result"), PublishedResult.Resolution, EPRDialogueChoiceResolution::Applied);
	TestTrue(TEXT("Applied choice returns one save request identifier"), PublishedResult.SaveRequestId.IsValid());
	FPRCompanionRelationshipRecord Relationship;
	TestTrue(TEXT("Axiom relationship remains addressable"), Companions->GetRelationshipSnapshot(FPRCompanionContract::AxiomTag(), Relationship));
	TestEqual(TEXT("Acknowledge increments Trust once"), Relationship.State.Trust, 51);
	TestEqual(TEXT("Acknowledge increments Affection once"), Relationship.State.Affection, 51);
	TestEqual(TEXT("Exactly one automation save was requested"), Backend->SaveCalls, 1);
	TestFalse(TEXT("Resolved choice cannot request another save"), Dialogue->SubmitChoice(false));
	TestEqual(TEXT("Resolved choice keeps one save request"), Backend->SaveCalls, 1);
	return true;
}

#endif
