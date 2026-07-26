// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Director/PRDirectorRuleEffectExecutor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorRuleRuntimeStateTest,
	"ProjectR.Director.Rules.Mechanics.ValidatedHandlePublishesSingleActiveRuntimeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorRuleRuntimeStateTest::RunTest(const FString& Parameters)
{
	FPRDirectorRuleEffectExecutor Executor;
	FPRAppliedDirectorRuleHandle Handle;
	Handle.HandleId = FGuid::NewGuid();
	Handle.RuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"));
	Handle.Level = 2;
	Handle.ApplySequence = 1;

	TestTrue(TEXT("A validated handle creates a runtime state"), Executor.Apply(Handle, FText::FromString(TEXT("Repeated skill")), FText::FromString(TEXT("Switch skills")), 1.0));
	FPRDirectorRuleRuntimeState State;
	TestTrue(TEXT("The state is queryable by its exact RuleId"), Executor.GetState(Handle.RuleId, State));
	TestEqual(TEXT("The state retains the validated handle id"), State.HandleId, Handle.HandleId);
	TestEqual(TEXT("A conditional rule starts suspended until its stable triggering event"), State.Status, EPRDirectorRuleRuntimeStatus::Suspended);
	TestTrue(TEXT("A stable event can activate the validated runtime rule"), Executor.SetRuleStatus(Handle.RuleId, EPRDirectorRuleRuntimeStatus::Active, 1.1));
	TestTrue(TEXT("The activated runtime rule remains queryable"), Executor.GetState(Handle.RuleId, State));
	TestEqual(TEXT("The activation state is retained"), State.Status, EPRDirectorRuleRuntimeStatus::Active);
	TestTrue(TEXT("The runtime rule tracks deterministic counter progress"), Executor.AdvanceCounter(Handle.RuleId, 1, 1.2));
	TestTrue(TEXT("A completed counter follows the explicit countered path"), Executor.GetState(Handle.RuleId, State));
	TestEqual(TEXT("The countered state removes the active effect deterministically"), State.Status, EPRDirectorRuleRuntimeStatus::Countered);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDirectorRuleResourceBalanceContractTest,
	"ProjectR.Director.Rules.Mechanics.ResourceBalanceUsesBoundedSessionCounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDirectorRuleResourceBalanceContractTest::RunTest(const FString& Parameters)
{
	FPRDirectorRuleEffectExecutor Executor;
	FPRAppliedDirectorRuleHandle Handle;
	Handle.HandleId = FGuid::NewGuid();
	Handle.RuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"));
	Handle.Level = 3;
	Handle.ApplySequence = 2;

	TestTrue(TEXT("ResourceBalance creates a validated session rule"),
		Executor.Apply(Handle, FText::FromString(TEXT("High energy")), FText::FromString(TEXT("Spend energy")), 2.0));
	FPRDirectorRuleRuntimeState State;
	TestTrue(TEXT("ResourceBalance state is available"), Executor.GetState(Handle.RuleId, State));
	TestEqual(TEXT("ResourceBalance starts suspended before a stable high-energy window"), State.Status, EPRDirectorRuleRuntimeStatus::Suspended);
	TestTrue(TEXT("ResourceBalance may enter its explicitly marked degraded session effect"),
		Executor.SetRuleStatus(Handle.RuleId, EPRDirectorRuleRuntimeStatus::Degraded, 7.0));
	TestTrue(TEXT("ResourceBalance bounded counter accepts an exact target"),
		Executor.AdvanceCounter(Handle.RuleId, State.CounterTarget, 8.0));
	TestTrue(TEXT("ResourceBalance state remains queryable after countering"), Executor.GetState(Handle.RuleId, State));
	TestEqual(TEXT("ResourceBalance counter path removes the session effect"), State.Status, EPRDirectorRuleRuntimeStatus::Countered);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
