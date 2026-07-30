// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/Headmind/PRHeadmindTypes.h"
#include "Core/PRRelationshipTypes.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRTripleResonanceEligibilityTest,
	"ProjectR.TripleResonance.Eligibility.FrozenOpportunityMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRTripleResonanceEligibilityTest::RunTest(const FString& Parameters)
{
	FPRTripleResonanceEligibilityInput Input;
	Input.Opportunity.State = EPRTripleResonanceOpportunityState::EligibleDeferredToV072;
	Input.Opportunity.bWindowActive = true;
	Input.bHasAuditorPrerequisite = true;
	Input.bHasFrozenRunEntitlement = true;
	Input.bHasExactHeadmindIdentity = true;
	Input.RunId = FGuid::NewGuid();
	Input.AccountId = FGuid::NewGuid();
	Input.BossSpawnId = FGuid::NewGuid();
	Input.WorldId = TEXT("HeadmindWorld");
	for (const FGameplayTag& CompanionId : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		FPRCompanionRelationshipRecord& Record = Input.Relationships.AddDefaulted_GetRef();
		Record.CompanionId = CompanionId;
		Record.State.Trust = 70;
		Record.State.Overload = 0;
	}

	const FPRTripleResonanceEligibilitySnapshot Eligible = FPRTripleResonanceEligibilityRules::Evaluate(Input);
	TestTrue(TEXT("All frozen conditions permit exactly one ready state"), Eligible.bEligible);
	TestEqual(TEXT("Eligible snapshot keeps the frozen run"), Eligible.RunId, Input.RunId);
	TestEqual(TEXT("Eligible snapshot keeps the frozen account"), Eligible.AccountId, Input.AccountId);
	TestEqual(TEXT("Eligible snapshot keeps the frozen boss"), Eligible.BossSpawnId, Input.BossSpawnId);

	Input.Relationships[2].State.Overload = 1;
	const FPRTripleResonanceEligibilitySnapshot Overloaded = FPRTripleResonanceEligibilityRules::Evaluate(Input);
	TestFalse(TEXT("Any overloaded companion blocks the opportunity"), Overloaded.bEligible);
	TestEqual(TEXT("Overload reports the fixed reason"), Overloaded.FailureReason, FName(TEXT("TripleResonance.CompanionOverload")));

	Input.Relationships[2].State.Overload = 0;
	Input.bHasExactHeadmindIdentity = false;
	const FPRTripleResonanceEligibilitySnapshot WrongIdentity = FPRTripleResonanceEligibilityRules::Evaluate(Input);
	TestFalse(TEXT("Wrong run, world, or boss identity is rejected"), WrongIdentity.bEligible);
	TestEqual(TEXT("Identity rejection is explicit"), WrongIdentity.FailureReason, FName(TEXT("TripleResonance.IdentityMismatch")));
	return true;
}

#endif
