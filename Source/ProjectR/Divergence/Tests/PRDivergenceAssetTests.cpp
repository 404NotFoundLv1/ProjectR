// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceDataAsset.h"
#include "Misc/AutomationTest.h"
#include "UI/PRDivergenceCacheWidget.h"

namespace PRDivergenceAssetAutomation
{
const EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDivergenceAssetDefinitionTest,
	"ProjectR.Divergence.Assets.FixedDefinition",
	PRDivergenceAssetAutomation::TestFlags)

bool FPRDivergenceAssetDefinitionTest::RunTest(const FString& Parameters)
{
	using namespace PRDivergenceAssetAutomation;
	(void)Parameters;

	UPRDivergenceDataAsset* Asset = NewObject<UPRDivergenceDataAsset>();
	TestNotNull(TEXT("Divergence widget native parent is registered"), UPRDivergenceCacheWidget::StaticClass());
	FString Error;
	TestFalse(TEXT("Unconfigured divergence asset is rejected"), Asset->ValidateDefinition(Error));
	TestTrue(TEXT("Unconfigured asset reports an error"), !Error.IsEmpty());

	FPRDivergenceContract::ConfigureFixedDefinition(*Asset);
	Error.Reset();
	TestTrue(TEXT("Fixed divergence asset validates"), Asset->ValidateDefinition(Error));
	TestEqual(TEXT("Fixed presentation count"), Asset->Presentations.Num(), 3);
	for (const FPRDivergencePresentationDefinition& Presentation : Asset->Presentations)
	{
		TestEqual(TEXT("Every companion has three choices"), Presentation.Choices.Num(), 3);
		for (const FPRDivergenceChoiceDefinition& Choice : Presentation.Choices)
		{
			FPRRelationshipDelta Expected;
			TestTrue(TEXT("Every configured delta is canonical"),
				FPRDivergenceContract::GetFixedRelationshipDelta(Presentation.CompanionId, Choice.Choice, Expected));
			TestEqual(TEXT("Trust delta"), Choice.RelationshipDelta.TrustDelta, Expected.TrustDelta);
			TestEqual(TEXT("Affection delta"), Choice.RelationshipDelta.AffectionDelta, Expected.AffectionDelta);
			TestEqual(TEXT("Evaluation delta"), Choice.RelationshipDelta.EvaluationDelta, Expected.EvaluationDelta);
			TestEqual(TEXT("Overload delta"), Choice.RelationshipDelta.OverloadDelta, Expected.OverloadDelta);
		}
	}
	return true;
}
