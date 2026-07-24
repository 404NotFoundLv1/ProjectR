// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRRelationshipTypes.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Misc/AutomationTest.h"

namespace PRDivergenceFoundationAutomation
{
const EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDivergenceFoundationContractTest,
	"ProjectR.Divergence.Foundation.FixedValueContract",
	PRDivergenceFoundationAutomation::TestFlags)

bool FPRDivergenceFoundationContractTest::RunTest(const FString& Parameters)
{
	using namespace PRDivergenceFoundationAutomation;
	(void)Parameters;

	TestEqual(TEXT("Idle enum value"), static_cast<uint8>(EPRDivergenceState::Idle), static_cast<uint8>(0));
	TestEqual(TEXT("Awaiting enum value"), static_cast<uint8>(EPRDivergenceState::AwaitingChoice), static_cast<uint8>(1));
	TestEqual(TEXT("Rescue enum value"), static_cast<uint8>(EPRDivergenceChoice::Rescue), static_cast<uint8>(1));
	TestEqual(TEXT("Leave enum value"), static_cast<uint8>(EPRDivergenceChoice::Leave), static_cast<uint8>(2));
	TestEqual(TEXT("Challenge enum value"), static_cast<uint8>(EPRDivergenceChoice::FaceChallenge), static_cast<uint8>(3));
	TestEqual(TEXT("Minimum trust"), FPRDivergenceContract::MinimumTrust, 50);
	TestEqual(TEXT("Maximum overload is exclusive"), FPRDivergenceContract::MaximumOverloadExclusive, 80);
	TestEqual(TEXT("Choice window"), FPRDivergenceContract::ChoiceWindowSeconds, 30.0f);
	TestEqual(TEXT("Rescue health fraction"), FPRDivergenceContract::RescueHealthFraction, 0.25f);
	TestEqual(TEXT("Challenge health fraction"), FPRDivergenceContract::ChallengeHealthFraction, 0.10f);
	TestEqual(TEXT("Revive shield fraction"), FPRDivergenceContract::ReviveShieldFraction, 0.0f);

	struct FExpectedDelta { FGameplayTag Companion; EPRDivergenceChoice Choice; int32 Trust; int32 Affection; int32 Evaluation; int32 Overload; };
	const FExpectedDelta Expected[] = {
		{FPRCompanionContract::AxiomTag(), EPRDivergenceChoice::Rescue, 2, 1, 1, 20},
		{FPRCompanionContract::AxiomTag(), EPRDivergenceChoice::Leave, -1, -1, 1, 0},
		{FPRCompanionContract::AxiomTag(), EPRDivergenceChoice::FaceChallenge, 0, 0, 2, 0},
		{FPRCompanionContract::KindleTag(), EPRDivergenceChoice::Rescue, 2, 2, 0, 25},
		{FPRCompanionContract::KindleTag(), EPRDivergenceChoice::Leave, -2, -1, 0, 0},
		{FPRCompanionContract::KindleTag(), EPRDivergenceChoice::FaceChallenge, 2, 1, 2, 0},
		{FPRCompanionContract::NullTag(), EPRDivergenceChoice::Rescue, 1, 2, 1, 20},
		{FPRCompanionContract::NullTag(), EPRDivergenceChoice::Leave, -1, -2, 1, 0},
		{FPRCompanionContract::NullTag(), EPRDivergenceChoice::FaceChallenge, 1, 1, 2, 0}};
	for (const FExpectedDelta& Expectation : Expected)
	{
		FPRRelationshipDelta Delta;
		TestTrue(TEXT("Canonical divergence delta is defined"), FPRDivergenceContract::GetFixedRelationshipDelta(Expectation.Companion, Expectation.Choice, Delta));
		TestEqual(TEXT("Canonical trust delta"), Delta.TrustDelta, Expectation.Trust);
		TestEqual(TEXT("Canonical affection delta"), Delta.AffectionDelta, Expectation.Affection);
		TestEqual(TEXT("Canonical evaluation delta"), Delta.EvaluationDelta, Expectation.Evaluation);
		TestEqual(TEXT("Canonical overload delta"), Delta.OverloadDelta, Expectation.Overload);
	}
	FPRRelationshipDelta Delta;
	TestFalse(TEXT("None has no relationship delta"),
		FPRDivergenceContract::GetFixedRelationshipDelta(FPRCompanionContract::AxiomTag(), EPRDivergenceChoice::None, Delta));
	return true;
}
