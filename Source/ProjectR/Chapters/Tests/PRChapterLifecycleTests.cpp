// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRChapterLifecycleDefaultsTest, "ProjectR.Chapter.Lifecycle.DefaultReadOnlySnapshot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRChapterLifecycleDefaultsTest::RunTest(const FString& Parameters)
{
	const UPRChapterSubsystem* Subsystem = GetDefault<UPRChapterSubsystem>();
	FPRChapterSnapshot Snapshot;
	TestTrue(TEXT("A Chapter subsystem exposes a value snapshot without a runtime session"), Subsystem->GetSnapshot(Snapshot));
	TestEqual(TEXT("No session begins in the inactive state"), Snapshot.State, EPRChapterLifecycleState::Inactive);
	FPRChapterCompletionResult Completion;
	TestFalse(TEXT("No completion is exposed before a verified chapter settlement"), Subsystem->GetLatestCompletion(Completion));
	return true;
}

#endif
