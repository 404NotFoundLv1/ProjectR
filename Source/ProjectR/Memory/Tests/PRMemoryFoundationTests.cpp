// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Memory/PRMemoryTypes.h"
#include "Memory/PRPostRunDialogueValidator.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRMemoryFoundationTest,
	"ProjectR.Memory.Foundation.FixedProviderWhitelist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRMemoryFoundationTest::RunTest(const FString&)
{
	FPRPostRunDialogueRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.SummaryId = FGuid::NewGuid();
	Request.SceneId = TEXT("post_run_summary");
	Request.CompanionId = TEXT("Axiom");

	FPRPostRunDialogueCandidate Candidate;
	Candidate.SceneId = TEXT("post_run_summary");
	Candidate.CompanionId = TEXT("Axiom");
	Candidate.EmotionId = TEXT("analytical");
	Candidate.Summary = TEXT("The verified record remains bounded and ready for review.");
	Candidate.PlayerOptionIds = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };

	const TArray<FName> ExactFields = {
		TEXT("scene"), TEXT("companion_id"), TEXT("emotion"), TEXT("summary"), TEXT("player_options") };
	FPRPostRunDialogueResult Result;
	TestTrue(TEXT("The exact Axiom five-field candidate is accepted"),
		FPRPostRunDialogueValidator::Validate(Request, Candidate, ExactFields, Result));
	TestEqual(TEXT("Accepted candidate preserves the fixed companion"), Result.CompanionId, TEXT("Axiom"));
	TestEqual(TEXT("Accepted candidate preserves exactly three options"), Result.PlayerOptionIds.Num(), 3);

	TArray<FName> UnknownFieldSet = ExactFields;
	UnknownFieldSet.Add(TEXT("memory_refs"));
	TestFalse(TEXT("An additional provider field is rejected"),
		FPRPostRunDialogueValidator::Validate(Request, Candidate, UnknownFieldSet, Result));

	Candidate.PlayerOptionIds[2] = TEXT("axiom_reflect");
	TestFalse(TEXT("Duplicate fixed options are rejected"),
		FPRPostRunDialogueValidator::Validate(Request, Candidate, ExactFields, Result));
	return true;
}

#endif
