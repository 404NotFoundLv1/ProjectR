// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/PRChapterSaveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRChapterSettlementPersistenceTest, "ProjectR.Chapter.Settlement.CanonicalProofPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRChapterSettlementPersistenceTest::RunTest(const FString& Parameters)
{
	FPRChapterPersistenceData Persistence;
	Persistence.CompletedChapterIds = {
		FPrimaryAssetId(TEXT("ProjectRChapter"), TEXT("DA_Chapter_Allocator")),
		FPrimaryAssetId(TEXT("ProjectRChapter"), TEXT("DA_Chapter_Allocator"))};
	Persistence.HumanAnomalyProofIds = {TEXT("HumanAnomalyProof.Allocator"), TEXT("HumanAnomalyProof.Allocator")};
	Persistence.SettlementSequence = 1;
	FPRChapterPersistenceContract::Normalize(Persistence);
	TestTrue(TEXT("Only one Allocator completion survives repeated settlement data"), Persistence.CompletedChapterIds.Num() == 1);
	TestTrue(TEXT("Only one Allocator proof survives repeated settlement data"), Persistence.HumanAnomalyProofIds.Num() == 1);
	TestTrue(TEXT("Normalized Chapter persistence is canonical"), FPRChapterPersistenceContract::IsCanonical(Persistence));
	return true;
}

#endif
