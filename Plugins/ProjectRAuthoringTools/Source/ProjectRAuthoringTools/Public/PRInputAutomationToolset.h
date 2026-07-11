// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRInputAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Narrow editor-only toolset that injects the v0.1.0 input acceptance sequence into an active PIE session. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRInputAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Runs deterministic keyboard and gamepad input through the active PIE viewport.
	 * The tool never saves or modifies a Package and releases every simulated input on all exit paths.
	 */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEInputSmoke(
		float MoveDurationSeconds = 0.30f,
		float JumpTimeoutSeconds = 2.0f,
		float MaxYDriftCm = 0.1f);
};
