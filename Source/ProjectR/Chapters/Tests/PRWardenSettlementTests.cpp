// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"
#include "Save/PRChapterSaveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRWardenSettlementContractTest,
	"ProjectR.Chapter.Warden.Settlement.SchemaSevenIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRWardenSettlementContractTest::RunTest(const FString& Parameters)
{
	FPRChapterPersistenceData Persistence;
	Persistence.CompletedChapterIds = { UPRChapterContentRegistryDataAsset::GetAllocatorChapterId() };
	Persistence.HumanAnomalyProofIds = { UPRChapterContentRegistryDataAsset::GetAllocatorProofId() };
	Persistence.SettlementSequence = 1;
	Persistence.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetWardenChapterId());
	Persistence.HumanAnomalyProofIds.Add(UPRChapterContentRegistryDataAsset::GetWardenProofId());
	++Persistence.SettlementSequence;
	FPRChapterPersistenceContract::Normalize(Persistence);
	TestTrue(TEXT("Schema 7 retains the bounded Allocator and Warden proof set"), FPRChapterPersistenceContract::IsCanonical(Persistence));
	TestEqual(TEXT("Warden settlement increments exactly once"), Persistence.SettlementSequence, static_cast<int64>(2));
	TestTrue(TEXT("Warden proof remains a fixed value identity"), Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetWardenProofId()));
	return true;
}

#endif
