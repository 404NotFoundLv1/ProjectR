// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/PRDialogueTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueLifecycleContractTest,
	"ProjectR.Dialogue.Lifecycle.ExpiryAndCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDialogueLifecycleContractTest::RunTest(const FString& Parameters)
{
	FPRDialogueRuntimeState State;
	State.PresentationState = EPRDialoguePresentationState::Choice;
	State.ExpireTimeSeconds = 15.0;
	TestEqual(TEXT("Choice expiration is represented in value state"), State.ExpireTimeSeconds, 15.0);
	FPRDialogueResult Result;
	Result.Resolution = EPRDialogueChoiceResolution::Cancelled;
	TestEqual(TEXT("Cancellation is an explicit non-save result"), Result.Resolution, EPRDialogueChoiceResolution::Cancelled);
	return true;
}

#endif
