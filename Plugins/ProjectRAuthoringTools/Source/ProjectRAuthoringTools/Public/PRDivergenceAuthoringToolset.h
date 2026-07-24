// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDivergenceAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed, no-argument v0.3.4 manifest writer. It has no generic asset authority. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDivergenceAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateV034DivergenceManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RepairV034DivergenceManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ValidateV034DivergenceManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* SaveV034DivergenceManifest();
};
