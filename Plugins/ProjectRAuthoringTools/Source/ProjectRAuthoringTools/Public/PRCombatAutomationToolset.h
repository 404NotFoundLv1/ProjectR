// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRCombatAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Narrow editor-only toolset that exercises ProjectR's fixed v0.1.2 combat acceptance sequence in PIE. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRCombatAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Runs the fixed shield, spill-over, rejection, death, and revive sequence without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIECombatSmoke(
		float StepDelaySeconds = 0.35f,
		float HitReactionToleranceSeconds = 0.03f);
};
