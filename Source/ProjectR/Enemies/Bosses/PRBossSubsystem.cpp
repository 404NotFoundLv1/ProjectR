// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/Bosses/PRBossSubsystem.h"

bool UPRBossSubsystem::GetActiveBossState(FPRBossRuntimeState& OutState) const
{
	OutState = bHasActiveBoss ? ActiveState : FPRBossRuntimeState();
	return bHasActiveBoss;
}

FPRBossStateChangedNative& UPRBossSubsystem::OnBossStateChanged()
{
	return BossStateChanged;
}

FPRPrototypeRunCompletedNative& UPRBossSubsystem::OnPrototypeRunCompleted()
{
	return PrototypeRunCompleted;
}

void UPRBossSubsystem::PublishBossState(const FPRBossRuntimeState& InState)
{
	if (bCompletionPublished && !InState.bDefeated)
	{
		return;
	}
	ActiveState = InState;
	bHasActiveBoss = true;
	BossStateChanged.Broadcast(ActiveState);
}

void UPRBossSubsystem::PublishPrototypeRunCompleted(const FPRPrototypeRunResult& InResult)
{
	if (bCompletionPublished)
	{
		return;
	}
	bCompletionPublished = true;
	PrototypeRunCompleted.Broadcast(InResult);
}
