// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"
#include "Save/PRChapterSaveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPacifierSettlementContractTest,
	"ProjectR.Chapter.Pacifier.Settlement.SchemaSevenThirdProofIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPacifierSettlementContractTest::RunTest(const FString& Parameters)
{
	FPRChapterPersistenceData Persistence;
	Persistence.CompletedChapterIds = {
		UPRChapterContentRegistryDataAsset::GetAllocatorChapterId(),
		UPRChapterContentRegistryDataAsset::GetWardenChapterId()};
	Persistence.HumanAnomalyProofIds = {
		UPRChapterContentRegistryDataAsset::GetAllocatorProofId(),
		UPRChapterContentRegistryDataAsset::GetWardenProofId()};
	Persistence.SettlementSequence = 2;
	Persistence.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetPacifierChapterId());
	Persistence.HumanAnomalyProofIds.Add(UPRChapterContentRegistryDataAsset::GetPacifierProofId());
	++Persistence.SettlementSequence;
	FPRChapterPersistenceContract::Normalize(Persistence);

	TestTrue(TEXT("Schema 7 retains a bounded canonical three-proof chain"), FPRChapterPersistenceContract::IsCanonical(Persistence));
	TestEqual(TEXT("Pacifier settlement increments exactly once"), Persistence.SettlementSequence, static_cast<int64>(3));
	TestEqual(TEXT("Exactly three chapter completions are retained"), Persistence.CompletedChapterIds.Num(), 3);
	TestEqual(TEXT("Exactly three anomaly proofs are retained"), Persistence.HumanAnomalyProofIds.Num(), 3);

	Persistence.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetPacifierChapterId());
	Persistence.HumanAnomalyProofIds.Add(UPRChapterContentRegistryDataAsset::GetPacifierProofId());
	FPRChapterPersistenceContract::Normalize(Persistence);
	TestEqual(TEXT("Duplicate Pacifier completion remains idempotent"), Persistence.CompletedChapterIds.Num(), 3);
	TestEqual(TEXT("Duplicate Pacifier proof remains idempotent"), Persistence.HumanAnomalyProofIds.Num(), 3);
	TestEqual(TEXT("Normalization never compensates settlement sequence"), Persistence.SettlementSequence, static_cast<int64>(3));
	return true;
}

#endif
