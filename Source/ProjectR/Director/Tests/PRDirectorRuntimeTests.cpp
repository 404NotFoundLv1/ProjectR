// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRMockDirectorProvider.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorMockContractTest,
	"ProjectR.Director.Runtime.MockIsDeterministicAndOffline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorMockContractTest::RunTest(const FString& Parameters)
{
	FPRDirectorRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.CandidateRuleIds.Add(FGameplayTag::RequestGameplayTag(TEXT("Rule.SurvivalProtocol")));
	FPRDirectorResponse First;
	FPRDirectorResponse Second;
	TestTrue(TEXT("The local mock can evaluate without a transport"), FPRMockDirectorProvider::BuildDeterministicResponse(Request, First));
	TestTrue(TEXT("The same request has the same local response"), FPRMockDirectorProvider::BuildDeterministicResponse(Request, Second));
	TestEqual(TEXT("The selected rule is stable"), First.RuleId, Second.RuleId);
	TestEqual(TEXT("The selected level is stable"), First.Level, Second.Level);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
