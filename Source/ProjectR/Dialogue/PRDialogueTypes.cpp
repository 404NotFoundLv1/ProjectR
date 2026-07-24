// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/PRDialogueTypes.h"

const TArray<FName>& FPRDialogueContract::GetExpectedCompanionAssetNames()
{
	static const TArray<FName> Expected = { TEXT("DA_CompanionDialogue_Axiom"), TEXT("DA_CompanionDialogue_Kindle"), TEXT("DA_CompanionDialogue_Null") };
	return Expected;
}

int32 FPRDialogueContract::GetTriggerPriority(const EPRDialogueTriggerKind TriggerKind)
{
	switch (TriggerKind)
	{
	case EPRDialogueTriggerKind::CriticalLowHealth: return 100;
	case EPRDialogueTriggerKind::QTESuccess: return 90;
	case EPRDialogueTriggerKind::QTEFailure: return 90;
	case EPRDialogueTriggerKind::BossPhaseChanged: return 90;
	case EPRDialogueTriggerKind::PredictionBlocked: return 90;
	case EPRDialogueTriggerKind::LowHealth: return 60;
	case EPRDialogueTriggerKind::SupportRejected: return 60;
	case EPRDialogueTriggerKind::SupportApplied: return 50;
	case EPRDialogueTriggerKind::SafeCombatCleared: return 20;
	default: return 0;
	}
}

float FPRDialogueContract::GetTriggerCooldown(const EPRDialogueTriggerKind TriggerKind)
{
	switch (TriggerKind)
	{
	case EPRDialogueTriggerKind::CriticalLowHealth: return 12.0f;
	case EPRDialogueTriggerKind::QTESuccess:
	case EPRDialogueTriggerKind::QTEFailure:
	case EPRDialogueTriggerKind::BossPhaseChanged:
	case EPRDialogueTriggerKind::PredictionBlocked: return 8.0f;
	case EPRDialogueTriggerKind::LowHealth:
	case EPRDialogueTriggerKind::SupportApplied:
	case EPRDialogueTriggerKind::SupportRejected: return 10.0f;
	case EPRDialogueTriggerKind::SafeCombatCleared: return 30.0f;
	default: return 8.0f;
	}
}

bool FPRDialogueContract::IsSafeStateTrigger(const EPRDialogueTriggerKind TriggerKind)
{
	return TriggerKind == EPRDialogueTriggerKind::SafeCombatCleared;
}

bool FPRDialogueContract::IsRequestBefore(const FPRDialogueRequest& Left, const FPRDialogueRequest& Right)
{
	if (Left.Priority != Right.Priority) return Left.Priority > Right.Priority;
	if (!FMath::IsNearlyEqual(Left.WorldTimeSeconds, Right.WorldTimeSeconds)) return Left.WorldTimeSeconds < Right.WorldTimeSeconds;
	return Left.LineId.LexicalLess(Right.LineId);
}

bool FPRDialogueContract::IsWithinEventDedupWindow(const double PreviousEventTimeSeconds, const double CurrentEventTimeSeconds)
{
	const double ElapsedSeconds = CurrentEventTimeSeconds - PreviousEventTimeSeconds;
	return ElapsedSeconds >= 0.0 && ElapsedSeconds <= 10.0;
}

double FPRDialogueContract::GetDeferredStartDelay(const TArray<double>& RecentStartTimes, const double CurrentTimeSeconds)
{
	if (RecentStartTimes.IsEmpty()) return 0.0;
	double DelaySeconds = FMath::Max(0.0, 1.25 - (CurrentTimeSeconds - RecentStartTimes.Last()));
	if (RecentStartTimes.Num() >= 3)
	{
		DelaySeconds = FMath::Max(DelaySeconds, 10.0 - (CurrentTimeSeconds - RecentStartTimes[0]));
	}
	return FMath::Max(0.0, DelaySeconds);
}

bool FPRDialogueContract::IsQueuedRequestExpired(const double EnqueueTimeSeconds, const double CurrentTimeSeconds)
{
	return CurrentTimeSeconds > EnqueueTimeSeconds + 3.0;
}

FName FPRDialogueContract::GetLineCooldownKey(const FGameplayTag& CompanionId, const FName LineId)
{
	return FName(*FString::Printf(TEXT("%s.%s"), *CompanionId.ToString(), *LineId.ToString()));
}
