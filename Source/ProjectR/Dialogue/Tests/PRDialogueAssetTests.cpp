// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/PRCompanionDialogueDataAsset.h"

#include "Core/PRRelationshipTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueAssetContractTest,
	"ProjectR.Dialogue.Assets.FixedThreeCompanionSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRDialogueAssetContractTest::RunTest(const FString& Parameters)
{
	UPRCompanionDialogueDataAsset* Asset = NewObject<UPRCompanionDialogueDataAsset>();
	Asset->CompanionId = FPRCompanionContract::AxiomTag();
	Asset->SpeakerName = FText::FromString(TEXT("Axiom"));
	const TArray<FName> ExpectedLineIds = {
		TEXT("CriticalLowHealth"), TEXT("LowHealth"), TEXT("QTESuccess"), TEXT("QTEFailure"), TEXT("SupportApplied"),
		TEXT("SupportRejected"), TEXT("BossPhaseChanged"), TEXT("PredictionBlocked"), TEXT("SafeCombatCleared") };
	for (int32 Index = 0; Index < 9; ++Index)
	{
		FPRDialogueLineDefinition& Line = Asset->Lines.AddDefaulted_GetRef();
		Line.LineId = ExpectedLineIds[Index];
		Line.TriggerKind = static_cast<EPRDialogueTriggerKind>(Index);
		Line.Priority = FPRDialogueContract::GetTriggerPriority(Line.TriggerKind);
		Line.CooldownSeconds = FPRDialogueContract::GetTriggerCooldown(Line.TriggerKind);
		Line.Text = FText::FromString(TEXT("Line"));
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FPRDialogueChoiceDefinition& Choice = Asset->SafeChoices.AddDefaulted_GetRef();
		Choice.ChoiceId = Index == 0 ? TEXT("Acknowledge") : TEXT("Analyze");
		Choice.Text = FText::FromString(TEXT("Choice"));
		Choice.RelationshipDelta.CompanionId = Asset->CompanionId;
		Choice.RelationshipDelta.SourceId = Index == 0 ? TEXT("Dialogue.Acknowledge") : TEXT("Dialogue.Analyze");
		Choice.RelationshipDelta.TrustDelta = 1;
		Choice.RelationshipDelta.AffectionDelta = Index == 0 ? 1 : 0;
		Choice.RelationshipDelta.EvaluationDelta = Index == 0 ? 0 : 1;
	}
	FString Error;
	TestTrue(TEXT("Fixed dialogue data schema validates"), Asset->ValidateDefinition(Error));
	Asset->Lines[0].LineId = TEXT("Unexpected");
	TestFalse(TEXT("Wrong fixed line identity fails validation"), Asset->ValidateDefinition(Error));
	Asset->Lines[0].LineId = ExpectedLineIds[0];
	Asset->SafeChoices[1].RelationshipDelta.TrustDelta = 0;
	TestFalse(TEXT("Wrong fixed choice delta fails validation"), Asset->ValidateDefinition(Error));
	Asset->SafeChoices[1].RelationshipDelta.TrustDelta = 1;
	Asset->Lines.Pop();
	TestFalse(TEXT("Missing fixed trigger line fails validation"), Asset->ValidateDefinition(Error));
	return true;
}

#endif
