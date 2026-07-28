// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRRealityHubAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Configures only the fixed v0.5.0 Reality Hub widget tree and HUD manifest. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRRealityHubAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ConfigureFixedRealityHubWidgetManifest();
	/** v0.5.1-only narrow edit: Companion terminal quest display and fixed confirmation control. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ConfigureV051CompanionQuestWidget();
	/** v0.5.2-only fixed Memory registry/persona manifest and the one permitted Hub Root extension. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateV052MemoryManifest();
	/** v0.5.2-only isolated runtime verifier; it accepts no slot, profile, asset, or gameplay input. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareV052MemoryPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunV052MemorySingleDeathPIE();
	/** Fixed ten-cycle verifier chooses its termination reason only from its existing isolated history. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunV052MemoryMixedTerminationPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CleanupV052MemoryPIE();
};
