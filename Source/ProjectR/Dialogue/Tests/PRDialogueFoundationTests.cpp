// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/PRDialogueTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueSchemaTest,
	"ProjectR.Dialogue.Schema.RuntimeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDialogueSchemaTest::RunTest(const FString& Parameters)
{
	const FPRDialogueRuntimeState Defaults;
	TestEqual(TEXT("Dialogue is idle by default"), Defaults.PresentationState, EPRDialoguePresentationState::Idle);
	TestFalse(TEXT("Dialogue default request is invalid"), Defaults.RequestId.IsValid());
	return true;
}

#endif
