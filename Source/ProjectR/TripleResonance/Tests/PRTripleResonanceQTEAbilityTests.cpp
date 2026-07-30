// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "TripleResonance/PRTripleResonanceTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRTripleResonanceQTEAbilityTest,
	"ProjectR.TripleResonance.QTEAbility.FixedThreeStepContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRTripleResonanceQTEAbilityTest::RunTest(const FString& Parameters)
{
	const FPRTripleResonanceQTEStepDefinition Axiom = FPRTripleResonanceContract::GetStepDefinition(EPRTripleResonanceStep::Axiom);
	const FPRTripleResonanceQTEStepDefinition Kindle = FPRTripleResonanceContract::GetStepDefinition(EPRTripleResonanceStep::Kindle);
	const FPRTripleResonanceQTEStepDefinition Null = FPRTripleResonanceContract::GetStepDefinition(EPRTripleResonanceStep::Null);
	TestEqual(TEXT("Axiom is the first external QTE"), Axiom.QTEId, FName(TEXT("TripleResonance_Axiom")));
	TestEqual(TEXT("Kindle is the second external QTE"), Kindle.QTEId, FName(TEXT("TripleResonance_Kindle")));
	TestEqual(TEXT("Null is the third external QTE"), Null.QTEId, FName(TEXT("TripleResonance_Null")));
	TestEqual(TEXT("Every external QTE uses the fixed 1.25 second window"), Axiom.WindowSeconds, 1.25f);
	TestTrue(TEXT("Axiom uses existing Interact input"), Axiom.AcceptedInputTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Input.Interact"))));
	TestTrue(TEXT("Kindle uses existing Attack input"), Kindle.AcceptedInputTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Input.Attack"))));
	TestTrue(TEXT("Null uses existing Execute input"), Null.AcceptedInputTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Input.Execute"))));
	TestTrue(TEXT("No fourth external QTE exists"), FPRTripleResonanceContract::GetStepDefinition(EPRTripleResonanceStep::None).QTEId.IsNone());
	return true;
}

#endif
