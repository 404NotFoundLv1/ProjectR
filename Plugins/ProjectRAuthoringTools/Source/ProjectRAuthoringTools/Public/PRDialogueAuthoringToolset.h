// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDialogueAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed, no-argument v0.3.3 asset manifest writer. It has no generic asset or package authority. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDialogueAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateV033DialogueManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RepairV033DialogueManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* SaveV033DialogueManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ValidateV033DialogueManifest();
};
