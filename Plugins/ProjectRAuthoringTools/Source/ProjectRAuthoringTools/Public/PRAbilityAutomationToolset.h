// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRAbilityAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Narrow editor-only toolset that exercises the fixed v0.1.3 AbilitySet lifecycle in PIE. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRAbilityAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Configures only the fixed v0.1.3 validation assets already created at their manifest paths. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* ConfigureAbilityValidationAssets();

	/** Runs the fixed grant, input, cost, cooldown, death, revive, and removal smoke. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEAbilitySmoke();
};
