// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRDirectorRuleEffectExecutor.h"
#include "Director/PRDirectorRuleEffectTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorTwelveRuleIntegrationTest,
	"ProjectR.Director.Rules.Integration.AllTwelveRulesHaveDeterministicRuntimePaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorTwelveRuleIntegrationTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> RuleIds = FPRDirectorRuleEffectContract::GetRequiredRuleIds();
	TestEqual(TEXT("The Director execution whitelist is exactly twelve rules"), RuleIds.Num(), 12);
	for (int32 Index = 0; Index < RuleIds.Num(); ++Index)
	{
		const FGameplayTag RuleId = RuleIds[Index];
		TestTrue(FString::Printf(TEXT("Rule %d is a valid stable tag"), Index), RuleId.IsValid());
		TestTrue(FString::Printf(TEXT("Rule %s is accepted by the runtime whitelist"), *RuleId.ToString()), FPRDirectorRuleEffectContract::IsRequiredRuleId(RuleId));
		TestFalse(FString::Printf(TEXT("Rule %s has player-facing effect text"), *RuleId.ToString()), FPRDirectorRuleEffectContract::GetEffectDescription(RuleId, 1).IsEmpty());
		TestTrue(FString::Printf(TEXT("Rule %s has a finite counter target"), *RuleId.ToString()), FPRDirectorRuleEffectContract::GetCounterTarget(RuleId) > 0);

		FPRDirectorRuleEffectExecutor Executor;
		FPRAppliedDirectorRuleHandle Handle;
		Handle.HandleId = FGuid::NewGuid();
		Handle.RuleId = RuleId;
		Handle.Level = 1;
		Handle.ApplySequence = Index + 1;
		TestTrue(FString::Printf(TEXT("Rule %s creates a value-only runtime state"), *RuleId.ToString()),
			Executor.Apply(Handle, FText::FromString(TEXT("Reason")), FText::FromString(TEXT("Counter")), 1.0));
		FPRDirectorRuleRuntimeState State;
		TestTrue(FString::Printf(TEXT("Rule %s can be read back"), *RuleId.ToString()), Executor.GetState(RuleId, State));
		TestEqual(FString::Printf(TEXT("Rule %s preserves its handle identity"), *RuleId.ToString()), State.HandleId, Handle.HandleId);
		TestTrue(FString::Printf(TEXT("Rule %s removes only its exact handle"), *RuleId.ToString()), Executor.Remove(Handle, 2.0));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
