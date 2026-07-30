#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Save/PRChapterSaveTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAuditorSettlementSchemaSevenTest, "ProjectR.Chapter.Auditor.Settlement.SchemaSevenIdempotency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRAuditorSettlementSchemaSevenTest::RunTest(const FString& Parameters)
{
	FPRChapterPersistenceData Data;
	Data.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetAllocatorChapterId());
	Data.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetWardenChapterId());
	Data.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetPacifierChapterId());
	Data.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetAuditorChapterId());
	Data.HumanAnomalyProofIds = { UPRChapterContentRegistryDataAsset::GetAllocatorProofId(), UPRChapterContentRegistryDataAsset::GetWardenProofId(), UPRChapterContentRegistryDataAsset::GetPacifierProofId(), UPRChapterContentRegistryDataAsset::GetAuditorProofId() };
	FPRChapterPersistenceContract::Normalize(Data);
	TestEqual(TEXT("Fourth proof remains bounded"), Data.HumanAnomalyProofIds.Num(), 4);
	TestTrue(TEXT("Fourth proof is canonical"), Data.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId()));
	return true;
}
#endif
