// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRPlayerProfileSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerProfileContractTest,
	"ProjectR.Director.Profile.ValueSnapshotAndSessionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRPlayerProfileContractTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRPlayerProfileSubsystem* Profile = NewObject<UPRPlayerProfileSubsystem>(GameInstance);
	FPRPlayerProfileSnapshot Snapshot;
	TestFalse(TEXT("A profile has no snapshot before a successful session begins"), Profile->GetSnapshot(Snapshot));
	Profile->BeginProfileSessionForAutomation();
	TestTrue(TEXT("A new session exposes a value-only snapshot"), Profile->GetSnapshot(Snapshot));
	TestEqual(TEXT("Snapshot schema remains frozen at one"), Snapshot.SchemaVersion, 1);
	TestTrue(TEXT("Profile session id is generated once per session"), Snapshot.ProfileSessionId.IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
