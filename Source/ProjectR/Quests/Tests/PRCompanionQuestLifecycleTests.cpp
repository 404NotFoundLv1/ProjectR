// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Quests/PRCompanionQuestSubsystem.h"
#include "Save/PRSaveSubsystem.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionQuestLifecycleTest,
	"ProjectR.CompanionQuest.Lifecycle.RuntimeStatesStayTransient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRCompanionQuestLifecycleTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRSaveSubsystem* Save = NewObject<UPRSaveSubsystem>(GameInstance);
	Save->LoadedSave = NewObject<UPRSaveGame>();
	Save->LoadedSave->SchemaVersion = UPRSaveGame::CurrentSchemaVersion;
	Save->LoadedSave->Profile.ProfileId = FGuid::NewGuid();
	FPRCompanionQuestRecord& Record = Save->LoadedSave->Profile.CompanionQuestPersistence.Records.AddDefaulted_GetRef();
	Record.QuestId = TEXT("Quest.Axiom.ImperfectOptimum");
	Record.State = EPRCompanionQuestPersistentState::Completed;
	Record.CompletionSequence = 1;
	FPRCompanionQuestPersistenceContract::Normalize(Save->LoadedSave->Profile.CompanionQuestPersistence);

	UPRCompanionQuestSubsystem* Quests = NewObject<UPRCompanionQuestSubsystem>(GameInstance);
	Quests->SaveSubsystem = Save;
	Quests->RegistryAsset = TSoftObjectPtr<UPRCompanionQuestRegistryDataAsset>(
		FSoftObjectPath(TEXT("/Game/ProjectR/Data/Quests/DA_CompanionQuestRegistry.DA_CompanionQuestRegistry")));
	Quests->PendingQuestId = Record.QuestId;
	Quests->PendingSaveRequestId = FGuid::NewGuid();
	Quests->PendingOperationId = FGuid::NewGuid();
	Quests->PendingTransaction = UPRCompanionQuestSubsystem::EPendingTransaction::Completion;
	int32 CompletionCount = 0;
	int32 EntitlementCount = 0;
	int32 DialogueCount = 0;
	Quests->OnQuestCompleted().AddLambda([&CompletionCount](FName) { ++CompletionCount; });
	Quests->OnEntitlementsChanged().AddLambda([&EntitlementCount](const FPRCompanionQuestEntitlementSnapshot&) { ++EntitlementCount; });
	Quests->OnDialogue().AddLambda([&DialogueCount](const FPRCompanionQuestDialogueLine&) { ++DialogueCount; });
	FPRSaveOperationEvent Success;
	Success.Operation = EPRSaveOperationType::Save;
	Success.RequestId = Quests->PendingSaveRequestId;
	Success.Result = EPRSaveResult::Success;
	Quests->HandleSaveOperation(Success);
	TestTrue(TEXT("Completed quest is visible only after matching persisted success"), Quests->IsQuestCompleted(Record.QuestId));
	TestEqual(TEXT("Completion publishes once"), CompletionCount, 1);
	TestEqual(TEXT("Entitlements publish once"), EntitlementCount, 1);
	TestEqual(TEXT("Line entitlement publishes its fixed local line once"), DialogueCount, 1);
	Quests->HandleSaveOperation(Success);
	TestEqual(TEXT("Repeated save callback cannot duplicate completion"), CompletionCount, 1);

	Quests->PendingQuestId = TEXT("Quest.Axiom.LowProbabilitySample");
	Quests->PendingSaveRequestId = FGuid::NewGuid();
	Quests->PendingOperationId = FGuid::NewGuid();
	Quests->PendingTransaction = UPRCompanionQuestSubsystem::EPendingTransaction::Activation;
	FPRSaveOperationEvent Failed;
	Failed.Operation = EPRSaveOperationType::Save;
	Failed.RequestId = Quests->PendingSaveRequestId;
	Failed.Result = EPRSaveResult::WriteFailed;
	Quests->HandleSaveOperation(Failed);
	FPRCompanionQuestSnapshot Snapshot;
	Quests->GetSnapshot(Snapshot);
	const FPRCompanionQuestEntrySnapshot* Entry = Snapshot.Entries.FindByPredicate([](const FPRCompanionQuestEntrySnapshot& Value)
	{
		return Value.QuestId == TEXT("Quest.Axiom.LowProbabilitySample");
	});
	TestNotNull(TEXT("Failed request retains its fixed quest entry"), Entry);
	if (Entry)
	{
		TestEqual(TEXT("Failed save is retryable rather than completed"), Entry->State, EPRCompanionQuestState::ReadyToRetry);
	}
	Quests->Deinitialize();
	FPRCompanionQuestSnapshot ClearedSnapshot;
	Quests->GetSnapshot(ClearedSnapshot);
	TestFalse(TEXT("Deinitialize clears transient quest runtime state"), ClearedSnapshot.bRegistryReady);
	TestTrue(TEXT("Deinitialize clears transient quest entries"), ClearedSnapshot.Entries.IsEmpty());
	return true;
}

#endif
