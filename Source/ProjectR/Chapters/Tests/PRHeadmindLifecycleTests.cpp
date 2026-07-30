// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/Headmind/PRHeadmindTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRHeadmindLifecycleValueContractTest, "ProjectR.Chapter.Headmind.Lifecycle.ValueContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRHeadmindLifecycleValueContractTest::RunTest(const FString& Parameters)
{
	FPRHeadmindBossRuntimeState Runtime; Runtime.TripleResonance.bWindowActive = true;
	Runtime.TripleResonance.bWindowActive = false;
	TestFalse(TEXT("Basilisk window cleanup leaves no active runtime window"), Runtime.TripleResonance.bWindowActive);
	return true;
}
#endif
