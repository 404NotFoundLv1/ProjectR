// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/Headmind/PRHeadmindEndingEvaluator.h"
#include "Save/PRAccountSaveTypes.h"
#include "Roguelike/Progression/PRProgressionTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRHeadmindSettlementValueContractTest, "ProjectR.Chapter.Headmind.Settlement.ValueContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRHeadmindSettlementValueContractTest::RunTest(const FString& Parameters)
{
	FPRRunSummary Summary; Summary.RunId = FGuid::NewGuid(); Summary.AccountId = FGuid::NewGuid(); Summary.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
	Summary.DirectorRules.Add({FGameplayTag::RequestGameplayTag(TEXT("Rule.ObedienceTest"), false), 3});
	FPRProgressionSnapshot Progression; Progression.CounterproofFragments = 4;
	FPRHeadmindEndingInputSnapshot Input;
	TestTrue(TEXT("Ending uses only bounded public run and progression values"), FPRHeadmindEndingEvaluator::BuildInput(Summary, Progression, Input));
	TestEqual(TEXT("Counterproof band is derived once"), Input.CounterproofBand, EPRHeadmindCounterproofBand::Abundant);
	TestEqual(TEXT("Obedience band is derived from whitelist rule"), Input.ObedienceBand, EPRHeadmindObedienceBand::Accepted);
	return true;
}
#endif
