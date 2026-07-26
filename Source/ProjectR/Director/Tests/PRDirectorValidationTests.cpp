// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRDirectorResponseValidator.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorValidationContractTest,
	"ProjectR.Director.Validation.SchemaRequestWhitelistAndText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorValidationContractTest::RunTest(const FString& Parameters)
{
	FPRDirectorRequest Request;
	Request.RequestId = FGuid::NewGuid();
	FPRDirectorResponse Response;
	Response.RequestId = FGuid::NewGuid();
	FPRDirectorValidationResult Validation;
	TestFalse(TEXT("Mismatched request ids are rejected before a rule may apply"),
		UPRDirectorResponseValidator::ValidateRequestIdentity(Request, Response, Validation));
	TestEqual(TEXT("The rejection remains explicit"), Validation.Result, EPRDirectorEvaluationResult::RejectedRequestId);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
