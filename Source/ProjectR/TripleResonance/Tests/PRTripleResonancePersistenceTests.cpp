// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/PRAccountSaveTypes.h"
#include "Save/PRTripleResonanceSaveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRTripleResonancePersistenceTest,
	"ProjectR.TripleResonance.Persistence.CanonicalLegacyAndProof",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRTripleResonancePersistenceTest::RunTest(const FString& Parameters)
{
	FPRTripleResonancePersistenceData Data;
	Data.LastProcessedGraveyardOrdinal = 9;
	Data.SkillMemory.SourceSummaryId = FGuid::NewGuid();
	Data.SkillMemory.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"));
	Data.SkillMemory.GraveyardOrdinal = 9;
	Data.SkillMemory.LegacySequence = 1;
	Data.LegacySequence = 1;
	Data.bHasHighRiskProof = true;
	Data.HighRiskProofSequence = 2;
	TestTrue(TEXT("One bounded P0 legacy skill and high-risk proof are canonical"), FPRTripleResonancePersistenceContract::IsCanonical(Data));
	Data.LastProcessedGraveyardOrdinal = 8;
	TestFalse(TEXT("A skill above the migration waterline is rejected"), FPRTripleResonancePersistenceContract::IsCanonical(Data));
	FPRTripleResonancePersistenceContract::Normalize(Data);
	TestFalse(TEXT("Invalid legacy is cleared deterministically"), Data.SkillMemory.IsValid());
	TestTrue(TEXT("The proof remains an explicit bounded flag"), Data.bHasHighRiskProof);

	TArray<FPRRunSkillSummary> Skills;
	Skills.Add({FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash")), 6, 2});
	Skills.Add({FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust")), 3, 3});
	Skills.Add({FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop")), 5, 3});
	Skills.Add({FGameplayTag::RequestGameplayTag(TEXT("Skill.TripleResonance")), 9, 9});
	TestEqual(TEXT("Legacy selection orders commit count, then use count, then stable tag"),
		FPRTripleResonancePersistenceContract::SelectLegacySkillMemory(Skills),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop")));
	return true;
}

#endif
