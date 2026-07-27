// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRRoguelikeAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Read-only verifier for the fixed v0.4.2 content manifest. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRRoguelikeAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ValidateFixedRoguelikeContent();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIERoguelikeSeed1101Smoke();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIERoguelikeSeed2202Smoke();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIERoguelikeSeed3303Smoke();
	/** Reads only the fixed v0.4.2 Room Flow input projection in an active PIE session. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectActiveRoomFlowInputPIE();
	/** Activates Choice0 through the fixed Room Flow widget delegate and validates its formal travel handoff. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIERoguelikeRoomFlowChoice0();
	/** Executes the fixed 1101 room sequence entirely through formal Room, Combat, Enemy, and Boss seams. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIERoguelikeFullPath1101();
};
