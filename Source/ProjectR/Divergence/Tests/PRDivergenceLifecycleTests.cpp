// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceTypes.h"
#include "Misc/AutomationTest.h"

namespace PRDivergenceLifecycleAutomation
{
const EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDivergenceEligibilityContractTest,
	"ProjectR.Divergence.Runtime.EligibilityBoundaries",
	PRDivergenceLifecycleAutomation::TestFlags)

bool FPRDivergenceEligibilityContractTest::RunTest(const FString& Parameters)
{
	using namespace PRDivergenceLifecycleAutomation;
	(void)Parameters;

	FPRDivergenceEligibilityInput Input;
	Input.bAuthority = true;
	Input.bCurrentPlayerDead = true;
	Input.bHasPrimaryCompanion = true;
	Input.bHasLoadedProfile = true;
	Input.bRunProtectionConsumed = false;
	Input.Trust = 50;
	Input.Overload = 79;
	TestTrue(TEXT("Trust 50 and Overload 79 are eligible"), FPRDivergenceContract::IsEligible(Input));

	Input.Trust = 49;
	TestFalse(TEXT("Trust 49 is ineligible"), FPRDivergenceContract::IsEligible(Input));
	Input.Trust = 50;
	Input.Overload = 80;
	TestFalse(TEXT("Overload 80 is ineligible"), FPRDivergenceContract::IsEligible(Input));
	Input.Overload = 79;
	Input.bRunProtectionConsumed = true;
	TestFalse(TEXT("Consumed protection is ineligible"), FPRDivergenceContract::IsEligible(Input));
	Input.bRunProtectionConsumed = false;
	Input.bCurrentPlayerDead = false;
	TestFalse(TEXT("Alive player is ineligible"), FPRDivergenceContract::IsEligible(Input));
	Input.bCurrentPlayerDead = true;
	Input.bAuthority = false;
	TestFalse(TEXT("Non-authority is ineligible"), FPRDivergenceContract::IsEligible(Input));
	return true;
}
