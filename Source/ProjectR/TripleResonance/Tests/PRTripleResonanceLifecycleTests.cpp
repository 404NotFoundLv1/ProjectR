// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRTripleResonanceLifecycleTest,
	"ProjectR.TripleResonance.Lifecycle.FixedIdentityRejectsStaleWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRTripleResonanceLifecycleTest::RunTest(const FString& Parameters)
{
	FPRTripleResonanceEligibilityInput Input;
	Input.Opportunity.State = EPRTripleResonanceOpportunityState::EligibleDeferredToV072;
	Input.Opportunity.bWindowActive = true;
	Input.bHasAuditorPrerequisite = true;
	Input.bHasFrozenRunEntitlement = true;
	Input.bHasExactHeadmindIdentity = false;
	Input.RunId = FGuid::NewGuid(); Input.AccountId = FGuid::NewGuid(); Input.BossSpawnId = FGuid::NewGuid(); Input.WorldId = TEXT("OldWorld");
	for (const FGameplayTag& Id : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		FPRCompanionRelationshipRecord& Record = Input.Relationships.AddDefaulted_GetRef();
		Record.CompanionId = Id; Record.State.Trust = 100; Record.State.Overload = 0;
	}
	const FPRTripleResonanceEligibilitySnapshot Result = FPRTripleResonanceEligibilityRules::Evaluate(Input);
	TestFalse(TEXT("A stale world identity is never eligible after travel"), Result.bEligible);
	TestEqual(TEXT("Stale identities use the fixed rejection reason"), Result.FailureReason, FName(TEXT("TripleResonance.IdentityMismatch")));
	return true;
}

#endif
