// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRDirectorRuleEffectTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorRuleWhitelistFoundationTest,
	"ProjectR.Director.Rules.Foundation.FixedTwelveRuleWhitelist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorRuleWhitelistFoundationTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> RuleIds = FPRDirectorRuleEffectContract::GetRequiredRuleIds();
	TestEqual(TEXT("The executable registry has exactly twelve rule identifiers"), RuleIds.Num(), 12);
	for (int32 Index = 1; Index < RuleIds.Num(); ++Index)
	{
		TestTrue(TEXT("Rule identifiers are strictly lexical for deterministic registry validation"),
			RuleIds[Index - 1].ToString() < RuleIds[Index].ToString());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
