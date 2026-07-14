// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/PRCombatTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRCombatSubsystem.generated.h"

/** World-owned deterministic entry point for ProjectR damage, death, revive, and combat facts. */
UCLASS()
class PROJECTR_API UPRCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	EPRCombatRequestStatus ApplyDamage(const FPRDamageRequest& Request);
	EPRCombatRequestStatus Revive(const FPRReviveRequest& Request);
	bool PublishAbilityOutcome(const FPRCombatOutcomeRequest& Request);
	FPRCombatEventNative& OnCombatEvent();

private:
	void BroadcastCombatEvent(const FPRCombatEvent& Event);

	FPRCombatEventNative CombatEvent;
};
