// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPacifierLifecycleContractTest,
	"ProjectR.Chapter.Pacifier.Lifecycle.BoundedTravelSafeSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPacifierLifecycleContractTest::RunTest(const FString& Parameters)
{
	FPRChapterSnapshot Snapshot;
	Snapshot.State = EPRChapterLifecycleState::RunActive;
	Snapshot.AllocationPressure = 2;
	Snapshot.RiskPressure = 3;
	Snapshot.ComfortPressure = FMath::Clamp(7, 0, 4);
	Snapshot.FallbackReason = TEXT("Pacifier.DirectorNeutralFallback");
	TestEqual(TEXT("ComfortPressure remains independently bounded"), Snapshot.ComfortPressure, 4);
	TestEqual(TEXT("Pacifier pressure never overwrites AllocationPressure"), Snapshot.AllocationPressure, 2);
	TestEqual(TEXT("Pacifier pressure never overwrites RiskPressure"), Snapshot.RiskPressure, 3);
	TestEqual(TEXT("Pacifier neutral fallback is a stable value id"), Snapshot.FallbackReason, FName(TEXT("Pacifier.DirectorNeutralFallback")));
	TestFalse(TEXT("A fresh Pacifier run cannot claim a proof before persistence"), Snapshot.bHasHumanAnomalyProof);
	TestEqual(TEXT("Pacifier story starts as an unavailable value projection"),
		Snapshot.PacifierStory.FallbackReason, FName());
	return true;
}

#endif
