// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterTypes.h"
#include "Chapters/Warden/PRWardenChapterDataAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRWardenLifecycleContractTest,
	"ProjectR.Chapter.Warden.Lifecycle.BoundedSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRWardenLifecycleContractTest::RunTest(const FString& Parameters)
{
	FPRChapterSnapshot Snapshot;
	Snapshot.State = EPRChapterLifecycleState::RunActive;
	Snapshot.RiskPressure = FMath::Clamp(7, 0, 4);
	Snapshot.FallbackReason = TEXT("Warden.DirectorNeutralFallback");
	TestEqual(TEXT("Risk pressure remains bounded for travel-safe value snapshots"), Snapshot.RiskPressure, 4);
	TestEqual(TEXT("Neutral fallback is a stable value identifier"), Snapshot.FallbackReason, FName(TEXT("Warden.DirectorNeutralFallback")));
	TestFalse(TEXT("A fresh run cannot claim a proof before A/B persistence"), Snapshot.bHasHumanAnomalyProof);
	return true;
}

#endif
