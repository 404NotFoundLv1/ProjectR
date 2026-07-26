// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRDirectorRuleDataAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorRuleAssetContractTest,
	"ProjectR.Director.Assets.RuleSchemaValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorRuleAssetContractTest::RunTest(const FString& Parameters)
{
	UPRDirectorRuleDataAsset* Rule = NewObject<UPRDirectorRuleDataAsset>();
	FPRDirectorParameterDefinition Parameter;
	Parameter.Name = TEXT("Threshold");
	Parameter.Minimum = 0.0f;
	Parameter.Maximum = 1.0f;
	Parameter.DefaultValue = 0.5f;
	Rule->ParameterSchema.Add(Parameter);
	TestFalse(TEXT("A rule without an exact whitelisted Rule tag is invalid"), Rule->IsRuleDefinitionValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
