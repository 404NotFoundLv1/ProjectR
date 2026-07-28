// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Memory/PRMemorySummaryBuilder.h"
#include "Roguelike/Account/PRAccountRuntimeTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRMemorySummaryBoundsTest, "ProjectR.Memory.Summary.AccountRecordBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRMemorySummaryBoundsTest::RunTest(const FString&)
{
	FPRAccountRecord Record;
	Record.RecordId = FGuid::NewGuid();
	Record.AccountId = FGuid::NewGuid();
	Record.GraveyardOrdinal = 1;
	Record.TerminationReason = EPRAccountTerminationReason::PlayerDeath;
	Record.Summary.PrimaryCompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		FPRRunDirectorRuleSummary& Rule = Record.Summary.DirectorRules.AddDefaulted_GetRef();
		Rule.RuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.DamageAmp"), false);
		Rule.Level = Index;
	}
	FPRMemorySummaryBuilder Builder;
	FPRMemorySummary Summary;
	TestTrue(TEXT("A published account record creates a value-only memory draft"), Builder.Build(Record, Summary));
	TestEqual(TEXT("Record id is the summary id"), Summary.SummaryId, Record.RecordId);
	TestEqual(TEXT("Rule list is bounded"), Summary.DirectorRules.Num(), FPRMemoryPersistenceContract::MaxDirectorRules);
	TestFalse(TEXT("No account id is persisted in the memory summary"), Summary.SummaryText.Contains(Record.AccountId.ToString()));
	return true;
}

#endif
