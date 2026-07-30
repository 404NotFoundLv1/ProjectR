// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRAuditorAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/**
 * Fixed v0.7.0 PIE fixture. It accepts no asset, profile, seed, class, path,
 * provider, or user-controlled storage input and only runs on isolated memory storage.
 */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRAuditorAutomationToolset final : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectFixedAuditorRegistry();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedAuditorPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedAuditorSelectionPIE();
	/** Drives the complete fixed Seed-61302 chapter through public Room/Combat APIs. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedAuditorFullPathPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedAuditorSettlementPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectFixedAuditorPIEState();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CleanupFixedAuditorPIE();
};
