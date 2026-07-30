// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRHeadmindContentClosedContractTest, "ProjectR.Chapter.Headmind.Content.ClosedContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRHeadmindContentClosedContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Headmind has five fixed directives"), UPRChapterContentRegistryDataAsset::GetHeadmindDirectiveIds().Num(), 5);
	TestEqual(TEXT("Headmind exposes exactly eight event choices"), UPRChapterRoguelikeContentRegistryDataAsset::GetExpectedEventPressureBindingCount(UPRChapterContentRegistryDataAsset::GetHeadmindContentId()), 8);
	TestEqual(TEXT("Headmind final room is closed"), UPRChapterContentRegistryDataAsset::GetHeadmindFinalRoomId().ToString(), FString(TEXT("ProjectRRoom:DA_Room_Headmind_Boss_HeadmindProjection")));
	return true;
}
#endif
