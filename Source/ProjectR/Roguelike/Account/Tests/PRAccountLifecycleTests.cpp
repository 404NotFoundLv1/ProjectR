// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Account/PRAccountRuntimeTypes.h"

#include "Misc/AutomationTest.h"

namespace PRAccountAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAccountLifecycleDeathDivergenceAndCompletionTest,
	"ProjectR.Account.Lifecycle.DeathDivergenceAndCompletion",
	PRAccountAutomation::TestFlags)

bool FPRAccountLifecycleDeathDivergenceAndCompletionTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Completed room sequence wins the same-frame death arbitration"),
		FPRRunTerminationArbiter::Resolve(true, true, EPRAccountTerminationReason::PlayerDeath),
		EPRAccountTerminationReason::RoomSequenceCompleted);
	TestEqual(
		TEXT("A valid divergence evacuation wins over pending death"),
		FPRRunTerminationArbiter::Resolve(false, true, EPRAccountTerminationReason::DivergenceEvacuation),
		EPRAccountTerminationReason::DivergenceEvacuation);
	TestEqual(
		TEXT("An unresolved death remains a player death"),
		FPRRunTerminationArbiter::Resolve(false, true, EPRAccountTerminationReason::PlayerDeath),
		EPRAccountTerminationReason::PlayerDeath);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
