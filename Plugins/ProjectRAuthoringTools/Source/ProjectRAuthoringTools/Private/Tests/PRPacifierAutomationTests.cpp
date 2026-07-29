// Copyright ProjectR. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "PRPacifierAutomationToolset.h"

#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRPacifierAutomationTests
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
		if (FPlatformTime::Seconds() - State->StartedAtSeconds > 180.0)
		{
			Test.AddError(TEXT("Fixed Pacifier PIE did not start within three minutes."));
			return true;
		}
		return false;
	}
	if (!State->bStarted)
	{
		State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(Start());
		State->bStarted = true;
		Test.TestNotNull(TEXT("Fixed Pacifier PIE returns a result."), State->Result.Get());
		return false;
	}
	if (!State->Result.IsValid() || !State->Result->bIsComplete)
	{
		if (FPlatformTime::Seconds() - State->StartedAtSeconds > 180.0)
		{
			Test.AddError(TEXT("Fixed Pacifier PIE did not complete within three minutes."));
			return true;
		}
		return false;
	}
	if (!State->Result->Error.IsEmpty()) Test.AddError(State->Result->Error);
	Test.TestTrue(
		TEXT("Fixed Pacifier PIE reports PASS."),
		State->Result->Value.Contains(TEXT("\"status\":\"PASS\"")));
	return true;
}

bool Prepare(FAutomationTestBase& Test)
{
	UToolCallAsyncResultString* Result = UPRPacifierAutomationToolset::PrepareFixedPacifierPIE();
	Test.TestNotNull(TEXT("Fixed Pacifier preparation returns a result."), Result);
	return Test.TestTrue(
		TEXT("Fixed Pacifier preparation succeeds."),
		Result && Result->Error.IsEmpty());
}

bool Cleanup(FAutomationTestBase& Test)
{
	if (GEditor && GEditor->PlayWorld) return false;
	UToolCallAsyncResultString* Result = UPRPacifierAutomationToolset::CleanupFixedPacifierPIE();
	Test.TestNotNull(TEXT("Fixed Pacifier cleanup returns a result."), Result);
	Test.TestTrue(TEXT("Fixed Pacifier cleanup succeeds."), Result && Result->Error.IsEmpty());
	return true;
}
} // namespace PRPacifierAutomationTests

#define PR_IMPLEMENT_FIXED_PACIFIER_PIE_TEST(TestClass, TestPath, RunCall) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		TestClass, TestPath, PRPacifierAutomationTests::TestFlags) \
	bool TestClass::RunTest(const FString& Parameters) \
	{ \
		using namespace PRPacifierAutomationTests; \
		if (!Prepare(*this)) return true; \
		const TSharedRef<FRunState> State = MakeShared<FRunState>(); \
		AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_RealityHub"))); \
		AddCommand(new FStartPIECommand(false)); \
		AddCommand(new FFunctionLatentCommand([this, State]() \
		{ \
			return WaitForResult(*this, State, []() { return RunCall; }); \
		})); \
		AddCommand(new FEndPlayMapCommand()); \
		AddCommand(new FFunctionLatentCommand([this]() { return Cleanup(*this); })); \
		return true; \
	}

PR_IMPLEMENT_FIXED_PACIFIER_PIE_TEST(
	FPRPacifierSelectionPIETest,
	"ProjectRAuthoringTools.Pacifier.PIE.Selection",
	UPRPacifierAutomationToolset::RunFixedPacifierSelectionPIE())

PR_IMPLEMENT_FIXED_PACIFIER_PIE_TEST(
	FPRPacifierFullPathPIETest,
	"ProjectRAuthoringTools.Pacifier.PIE.FullPath",
	UPRPacifierAutomationToolset::RunFixedPacifierFullPathPIE())

PR_IMPLEMENT_FIXED_PACIFIER_PIE_TEST(
	FPRPacifierPersistenceRetryPIETest,
	"ProjectRAuthoringTools.Pacifier.PIE.PersistenceRetry",
	UPRPacifierAutomationToolset::RunFixedPacifierPersistenceRetryPIE())

#undef PR_IMPLEMENT_FIXED_PACIFIER_PIE_TEST

#endif // WITH_AUTOMATION_TESTS
