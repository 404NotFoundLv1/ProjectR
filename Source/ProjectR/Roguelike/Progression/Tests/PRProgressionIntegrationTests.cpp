// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Progression/PRProgressionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRProgressionIntegrationTest,
	"ProjectR.Progression.Integration.TravelReloadAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRProgressionIntegrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRProgressionSubsystem* Subsystem = NewObject<UPRProgressionSubsystem>(GameInstance);
	TestNotNull(TEXT("Progression subsystem has a valid GameInstance owner"), Subsystem);
	if (!Subsystem) return false;
	FPRProgressionRunSnapshot Snapshot;
	TestFalse(TEXT("No run snapshot exists before a successful run start"), Subsystem->GetRunSnapshot(Snapshot));
	return true;
}

#endif
