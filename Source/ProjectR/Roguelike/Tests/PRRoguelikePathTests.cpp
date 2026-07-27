// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Roguelike/PRRoomSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRoguelikePathLengthTest,
	"ProjectR.Roguelike.Path.DeterministicSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRoguelikePathLengthTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Seed 1101 produces a bounded path length"), UPRRoomSubsystem::GetRoomPathLengthForSeed(1101), 7);
	TestEqual(TEXT("Seed 2202 produces a bounded path length"), UPRRoomSubsystem::GetRoomPathLengthForSeed(2202), 8);
	TestEqual(TEXT("Seed 3303 produces a bounded path length"), UPRRoomSubsystem::GetRoomPathLengthForSeed(3303), 9);
	TestTrue(TEXT("All signed seeds remain within the contract bound"), UPRRoomSubsystem::GetRoomPathLengthForSeed(-1) >= 6 && UPRRoomSubsystem::GetRoomPathLengthForSeed(-1) <= 10);
	return true;
}
