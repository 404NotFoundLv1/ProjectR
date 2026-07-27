// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Progression/PRProgressionSubsystem.h"

#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRProgressionRuntimeTest,
	"ProjectR.Progression.Runtime.NextRunEffectsAndEntitlements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRProgressionRuntimeTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Progression subsystem reflects"), UPRProgressionSubsystem::StaticClass());
	const FPRProgressionRunSnapshot Snapshot;
	TestEqual(TEXT("Support multiplier defaults to one"), Snapshot.CompanionSupportIntervalMultiplier, 1.0f);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("ProgressionRuntimeAutomationWorld")));
	UPRCompanionRuntimeSubsystem* Runtime = World ? NewObject<UPRCompanionRuntimeSubsystem>(World) : nullptr;
	TestNotNull(TEXT("Companion policy seam is world-owned"), Runtime);
	if (Runtime)
	{
		TestFalse(TEXT("Progression cannot exceed its fixed 0.90 lower bound"),
			Runtime->SetSupportPolicy(TEXT("Progression.AISupport"), 0.89f, 1));
		TestTrue(TEXT("Progression can install the fixed 0.90 multiplier"),
			Runtime->SetSupportPolicy(TEXT("Progression.AISupport"), 0.90f, 1));
		float Multiplier = 0.0f;
		int32 Stride = 0;
		TestTrue(TEXT("Policy snapshot is available"), Runtime->GetSupportPolicy(Multiplier, Stride));
		TestEqual(TEXT("Fixed multiplier survives the policy seam"), Multiplier, 0.90f);
		TestEqual(TEXT("Progression does not alter suppression stride"), Stride, 1);
		TestTrue(TEXT("Progression policy can be cleared by its fixed source id"),
			Runtime->ClearSupportPolicy(TEXT("Progression.AISupport")));
	}
	if (World) World->DestroyWorld(false);
	return true;
}

#endif
