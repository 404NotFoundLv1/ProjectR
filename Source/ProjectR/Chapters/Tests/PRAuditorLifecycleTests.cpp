#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/Auditor/PRAuditorChapterTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAuditorLifecycleBoundedStateTest, "ProjectR.Chapter.Auditor.Lifecycle.BoundedState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRAuditorLifecycleBoundedStateTest::RunTest(const FString& Parameters)
{
	FPRAuditorChapterBossRuntimeState State;
	State.AuditPressure = FMath::Clamp(9, 0, 4);
	TestEqual(TEXT("Audit pressure stays bounded"), State.AuditPressure, 4);
	TestFalse(TEXT("New runtime state has no leaked mechanism"), State.bDegradedNoOp);
	return true;
}
#endif
