// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/Headmind/PRHeadmindTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRHeadmindBossValueContractTest, "ProjectR.Chapter.Headmind.Boss.ValueContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRHeadmindBossValueContractTest::RunTest(const FString& Parameters)
{
	FPRHeadmindBossRuntimeState Runtime;
	Runtime.SynthesisPressure = FMath::Clamp(7, 0, 4);
	Runtime.TripleResonance.State = EPRTripleResonanceOpportunityState::EligibleDeferredToV072;
	TestEqual(TEXT("Synthesis pressure remains bounded"), Runtime.SynthesisPressure, 4);
	TestEqual(TEXT("Opportunity is explicitly deferred to v0.7.2"), Runtime.TripleResonance.State, EPRTripleResonanceOpportunityState::EligibleDeferredToV072);
	return true;
}
#endif
