// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "QTE/PRQTETypes.h"

namespace PRQTELifecycleAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRQTELifecycleTransientContractTest, "ProjectR.QTE.Lifecycle.TransientStateContract", PRQTELifecycleAutomation::TestFlags)

bool FPRQTELifecycleTransientContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("QTE idle state retains its serialized ordinal"), static_cast<uint8>(EPRQTEState::Idle), static_cast<uint8>(0));
	TestEqual(TEXT("QTE active state retains its serialized ordinal"), static_cast<uint8>(EPRQTEState::Active), static_cast<uint8>(1));
	TestEqual(TEXT("QTE resolving state retains its serialized ordinal"), static_cast<uint8>(EPRQTEState::Resolving), static_cast<uint8>(2));
	TestEqual(TEXT("QTE cooldown state retains its serialized ordinal"), static_cast<uint8>(EPRQTEState::Cooldown), static_cast<uint8>(3));

	const FPRQTERuntimeState IdleState;
	TestEqual(TEXT("A new QTE runtime state is idle"), IdleState.State, EPRQTEState::Idle);
	TestFalse(TEXT("A new QTE runtime state has no request id"), IdleState.RequestId.IsValid());
	TestFalse(TEXT("A new QTE runtime state has no QTE asset id"), IdleState.QTEId.IsValid());
	TestTrue(TEXT("A new QTE runtime state has no display name"), IdleState.DisplayName.IsEmpty());
	TestTrue(TEXT("A new QTE runtime state has no prompt text"), IdleState.PromptText.IsEmpty());
	TestTrue(TEXT("A new QTE runtime state has no accepted inputs"), IdleState.AcceptedInputTags.IsEmpty());

	const FPRQTEResult Result;
	TestFalse(TEXT("A default QTE result has no runtime result id"), Result.ResultId.IsValid());
	TestFalse(TEXT("A default QTE result has no save request id"), Result.SaveRequestId.IsValid());
	TestTrue(TEXT("A default QTE result has no profile samples"), Result.ProfileSampleTags.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
