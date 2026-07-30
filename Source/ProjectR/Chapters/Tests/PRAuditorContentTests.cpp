#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAuditorContentClosedContractTest, "ProjectR.Chapter.Auditor.Content.ClosedContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRAuditorContentClosedContractTest::RunTest(const FString& Parameters)
{
	const TArray<FName>& Directives = UPRChapterContentRegistryDataAsset::GetAuditorDirectiveIds();
	TestEqual(TEXT("Auditor directive count is fixed"), Directives.Num(), 5);
	TestEqual(TEXT("Auditor has exactly the nine declared event-choice pressure bindings"),
		UPRChapterRoguelikeContentRegistryDataAsset::GetExpectedEventPressureBindingCount(UPRChapterContentRegistryDataAsset::GetAuditorContentId()), 9);
	TestEqual(TEXT("Existing chapter packs retain their ten binding contract"),
		UPRChapterRoguelikeContentRegistryDataAsset::GetExpectedEventPressureBindingCount(UPRChapterContentRegistryDataAsset::GetPacifierContentId()), 10);
	TestTrue(TEXT("Auditor proof is fixed"), UPRChapterContentRegistryDataAsset::GetAuditorProofId() == TEXT("HumanAnomalyProof.Auditor"));
	TestTrue(TEXT("Auditor final room is fixed"), UPRChapterContentRegistryDataAsset::GetAuditorFinalRoomId().ToString() == TEXT("ProjectRRoom:DA_Room_Auditor_Boss_Auditor"));
	return true;
}
#endif
