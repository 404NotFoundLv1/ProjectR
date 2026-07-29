// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRWardenAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.6.1 PIE fixture. It has no caller-controlled storage, profile, asset, seed, or content input. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRWardenAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectFixedWardenRegistry();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedWardenPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedWardenSelectionPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedWardenInitialCombatPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CleanupFixedWardenPIE();
};
