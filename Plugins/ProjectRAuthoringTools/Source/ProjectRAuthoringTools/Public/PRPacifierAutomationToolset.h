// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPacifierAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/**
 * Fixed v0.6.2 PIE fixture. It accepts no caller-controlled storage, profile,
 * asset, seed, content, class, path, tag, text, or numeric input.
 */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPacifierAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectFixedPacifierRegistry();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedPacifierPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedPacifierSelectionPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedPacifierFullPathPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedPacifierPersistenceRetryPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CleanupFixedPacifierPIE();
};
