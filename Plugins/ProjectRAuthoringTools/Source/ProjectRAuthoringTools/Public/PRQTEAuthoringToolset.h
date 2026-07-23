// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRQTEAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.3.2 QTE manifest writer. It accepts no caller-controlled data or save scope. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRQTEAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateV032QTEManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* SaveV032QTEManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ReconfigureV032QTEContracts();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* SaveReconfiguredV032QTEContracts();
};
