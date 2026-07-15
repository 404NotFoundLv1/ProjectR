// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPlayerSkillAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Narrow editor-only toolset that exercises ProjectR's fixed v0.2.0-B player-skill sequence in PIE. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPlayerSkillAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Runs the fixed ShadowThrust, FireSlash, Burning, input, and cleanup sequence without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointBSmoke();
};
