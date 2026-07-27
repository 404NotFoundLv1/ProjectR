// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Account/PRRunSummaryBuilder.h"

#include "Misc/AutomationTest.h"

namespace PRAccountAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAccountIntegrationTravelProjectionAndCleanupTest,
	"ProjectR.Account.Integration.TravelProjectionAndCleanup",
	PRAccountAutomation::TestFlags)

bool FPRAccountIntegrationTravelProjectionAndCleanupTest::RunTest(const FString& Parameters)
{
	FPRRunSummaryBuilder Builder;
	Builder.Reset(FGuid::NewGuid(), FGuid::NewGuid(), 1337, FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), 100);
	Builder.RecordRoom(FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("CombatStandard")));
	const FPRRunSummary Summary = Builder.Build(EPRAccountTerminationReason::InterruptedRecovery, 140);
	TestEqual(TEXT("Summary preserves the run seed"), Summary.Seed, 1337);
	TestEqual(TEXT("Summary contains only the recorded room value"), Summary.RoomIds.Num(), 1);
	TestEqual(TEXT("Interrupted recovery does not award fragments"), Summary.CounterproofFragmentsAwarded, int32{0});
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
