// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/PRCompanionDialogueDataAsset.h"

FPrimaryAssetId UPRCompanionDialogueDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CompanionDialogue"), GetFName());
}

bool UPRCompanionDialogueDataAsset::ValidateDefinition(FString& OutError) const
{
	if (!CompanionId.IsValid() || SpeakerName.IsEmpty() || Lines.Num() != 9 || SafeChoices.Num() != 2)
	{
		OutError = TEXT("Dialogue assets require one companion, nine trigger lines and two safe choices.");
		return false;
	}
	TSet<FName> LineIds;
	TSet<EPRDialogueTriggerKind> TriggerKinds;
	const TArray<FName> ExpectedLineIds = {
		TEXT("CriticalLowHealth"), TEXT("LowHealth"), TEXT("QTESuccess"), TEXT("QTEFailure"), TEXT("SupportApplied"),
		TEXT("SupportRejected"), TEXT("BossPhaseChanged"), TEXT("PredictionBlocked"), TEXT("SafeCombatCleared") };
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FPRDialogueLineDefinition& Line = Lines[Index];
		if (Line.LineId.IsNone() || Line.Text.IsEmpty() || Line.Priority != FPRDialogueContract::GetTriggerPriority(Line.TriggerKind)
			|| !FMath::IsNearlyEqual(Line.CooldownSeconds, FPRDialogueContract::GetTriggerCooldown(Line.TriggerKind))
			|| !FMath::IsNearlyEqual(Line.DisplayDurationSeconds, 2.5f) || Line.LineId != ExpectedLineIds[Index]
			|| Line.TriggerKind != static_cast<EPRDialogueTriggerKind>(Index) || LineIds.Contains(Line.LineId) || TriggerKinds.Contains(Line.TriggerKind))
		{
			OutError = TEXT("Dialogue lines must use the frozen order, identity, duration, priorities, cooldowns and trigger contract.");
			return false;
		}
		LineIds.Add(Line.LineId);
		TriggerKinds.Add(Line.TriggerKind);
	}
	if (TriggerKinds.Num() != 9)
	{
		OutError = TEXT("Dialogue assets must contain every fixed trigger exactly once.");
		return false;
	}
	for (int32 Index = 0; Index < SafeChoices.Num(); ++Index)
	{
		const FPRDialogueChoiceDefinition& Choice = SafeChoices[Index];
		const bool bAnalyze = Index == 1;
		if (Choice.ChoiceId != (bAnalyze ? TEXT("Analyze") : TEXT("Acknowledge"))
			|| Choice.Text.IsEmpty() || Choice.RelationshipDelta.CompanionId != CompanionId
			|| Choice.RelationshipDelta.SourceId != (bAnalyze ? TEXT("Dialogue.Analyze") : TEXT("Dialogue.Acknowledge"))
			|| Choice.RelationshipDelta.TrustDelta != 1
			|| Choice.RelationshipDelta.AffectionDelta != (bAnalyze ? 0 : 1)
			|| Choice.RelationshipDelta.EvaluationDelta != (bAnalyze ? 1 : 0)
			|| Choice.RelationshipDelta.OverloadDelta != 0)
		{
			OutError = TEXT("Dialogue choices must use the frozen relationship-delta contract.");
			return false;
		}
	}
	return true;
}

const FPRDialogueLineDefinition* UPRCompanionDialogueDataAsset::FindLine(const EPRDialogueTriggerKind TriggerKind) const
{
	return Lines.FindByPredicate([TriggerKind](const FPRDialogueLineDefinition& Line) { return Line.TriggerKind == TriggerKind; });
}
