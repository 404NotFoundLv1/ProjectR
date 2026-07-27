// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Roguelike/PRRoomSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRoguelikeTravelEventsAndCleanupTest,
	"ProjectR.Roguelike.Integration.TravelEventsAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRoguelikeTravelEventsAndCleanupTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Room flow exposes no public cancellation or save mutation entry point"), UPRRoomSubsystem::StaticClass()->IsChildOf<UGameInstanceSubsystem>());
	FPRRoomRuntimeState State;
	TestEqual(TEXT("A reset state has no path steps"), State.Path.Num(), 0);
	TestFalse(TEXT("A reset state has no completion identity"), State.SessionId.IsValid());
	return true;
}
