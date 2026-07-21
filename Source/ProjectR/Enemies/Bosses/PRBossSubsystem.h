// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Enemies/Bosses/PRBossTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRBossSubsystem.generated.h"

/** World-owned read-only Boss state and one-shot prototype completion publisher. */
UCLASS()
class PROJECTR_API UPRBossSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool GetActiveBossState(FPRBossRuntimeState& OutState) const;
	FPRBossStateChangedNative& OnBossStateChanged();
	FPRPrototypeRunCompletedNative& OnPrototypeRunCompleted();
	void PublishBossState(const FPRBossRuntimeState& InState);
	void PublishPrototypeRunCompleted(const FPRPrototypeRunResult& InResult);

private:
	FPRBossRuntimeState ActiveState;
	bool bHasActiveBoss = false;
	bool bCompletionPublished = false;
	FPRBossStateChangedNative BossStateChanged;
	FPRPrototypeRunCompletedNative PrototypeRunCompleted;
};
