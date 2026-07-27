// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "PRAccountAutomationToolset.h"

#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRAccountAutomationTests
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

struct FRunState
{
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAtSeconds = FPlatformTime::Seconds();
	bool bStarted = false;
};

bool WaitForResult(
	FAutomationTestBase& Test,
	const TSharedRef<FRunState>& State,
	TFunctionRef<UToolCallAsyncResultString*()> Start)
{
	UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
	if (!IsValid(PlayWorld))
	{
		if (FPlatformTime::Seconds() - State->StartedAtSeconds > 30.0)
		{
			Test.AddError(TEXT("Fixed account PIE did not start within thirty seconds."));
			return true;
		}
		return false;
	}
	if (!State->bStarted)
	{
		State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(Start());
		State->bStarted = true;
		Test.TestNotNull(TEXT("Fixed account lifecycle returns a result."), State->Result.Get());
		return false;
	}
	if (!State->Result.IsValid() || !State->Result->bIsComplete)
	{
		if (FPlatformTime::Seconds() - State->StartedAtSeconds > 30.0)
		{
			Test.AddError(TEXT("Fixed account lifecycle did not complete within thirty seconds."));
			return true;
		}
		return false;
	}
	if (!State->Result->Error.IsEmpty())
	{
		Test.AddError(State->Result->Error);
	}
	Test.TestTrue(
		TEXT("Fixed account lifecycle reports PASS."),
		State->Result->Value.Contains(TEXT("\"status\":\"PASS\"")));
	return true;
}

bool Prepare(
	FAutomationTestBase& Test,
	UToolCallAsyncResultString* Result)
{
	Test.TestNotNull(TEXT("Fixed account preparation returns a result."), Result);
	return Test.TestTrue(
		TEXT("Fixed account preparation succeeds."),
		Result && Result->Error.IsEmpty());
}
} // namespace PRAccountAutomationTests

#define PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(TestClass, TestPath, PrepareCall, RunCall) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		TestClass, TestPath, PRAccountAutomationTests::TestFlags) \
	bool TestClass::RunTest(const FString& Parameters) \
	{ \
		using namespace PRAccountAutomationTests; \
		if (!Prepare(*this, PrepareCall)) \
		{ \
			return true; \
		} \
		const TSharedRef<FRunState> State = MakeShared<FRunState>(); \
		AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_RealityHub"))); \
		AddCommand(new FStartPIECommand(false)); \
		AddCommand(new FFunctionLatentCommand([this, State]() \
		{ \
			return WaitForResult(*this, State, []() { return RunCall; }); \
		})); \
		AddCommand(new FEndPlayMapCommand()); \
		return true; \
	}

PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(
	FPRAccountCompletionPIETest,
	"ProjectRAuthoringTools.Account.PIE.Completion",
	UPRAccountAutomationToolset::PrepareFixedAccountPIE(),
	UPRAccountAutomationToolset::RunFixedCompletionAccountPIE())

PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(
	FPRAccountDeathPIETest,
	"ProjectRAuthoringTools.Account.PIE.Death",
	UPRAccountAutomationToolset::PrepareFixedAccountPIE(),
	UPRAccountAutomationToolset::RunFixedDeathAccountPIE())

PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(
	FPRAccountEvacuationPIETest,
	"ProjectRAuthoringTools.Account.PIE.Evacuation",
	UPRAccountAutomationToolset::PrepareFixedAccountPIE(),
	UPRAccountAutomationToolset::RunFixedEvacuationAccountPIE())

PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(
	FPRAccountLeavePIETest,
	"ProjectRAuthoringTools.Account.PIE.Leave",
	UPRAccountAutomationToolset::PrepareFixedAccountPIE(),
	UPRAccountAutomationToolset::RunFixedLeaveAccountPIE())

PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST(
	FPRAccountPersistenceFailurePIETest,
	"ProjectRAuthoringTools.Account.PIE.PersistenceFailure",
	UPRAccountAutomationToolset::PrepareFixedAccountFailurePIE(),
	UPRAccountAutomationToolset::RunFixedPersistenceFailurePIE())

#undef PR_IMPLEMENT_FIXED_ACCOUNT_PIE_TEST

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAccountInterruptedRecoveryPIETest,
	"ProjectRAuthoringTools.Account.PIE.InterruptedRecovery",
	PRAccountAutomationTests::TestFlags)

bool FPRAccountInterruptedRecoveryPIETest::RunTest(const FString& Parameters)
{
	using namespace PRAccountAutomationTests;
	if (!Prepare(*this, UPRAccountAutomationToolset::PrepareFixedAccountPIE()))
	{
		return true;
	}

	const TSharedRef<FRunState> StartState = MakeShared<FRunState>();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_RealityHub")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, StartState]()
	{
		return WaitForResult(
			*this,
			StartState,
			[]() { return UPRAccountAutomationToolset::RunFixedStartOnlyAccountPIE(); });
	}));
	AddCommand(new FEndPlayMapCommand());

	const TSharedRef<FRunState> RecoveryState = MakeShared<FRunState>();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_RealityHub")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, RecoveryState]()
	{
		return WaitForResult(
			*this,
			RecoveryState,
			[]() { return UPRAccountAutomationToolset::RunFixedInterruptedRecoveryPIE(); });
	}));
	AddCommand(new FEndPlayMapCommand());
	return true;
}

#endif // WITH_AUTOMATION_TESTS
