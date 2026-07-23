// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRCompanionAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.3.0 PIE verifier; it accepts no caller data and never saves a Package or profile. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRCompanionAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Selects Axiom in the active authoritative PIE world and verifies the run-local selection survives fixed travel to BossGym. */
	UFUNCTION(meta = (AICallable))
	static UToolCallAsyncResultString* RunPIECompanionSyncSmoke();

	/** Fixed v0.3.1 active-Companion support and lifecycle verifier. */
	UFUNCTION(meta = (AICallable))
	static UToolCallAsyncResultString* RunPIECompanionCombatSmoke();
};
