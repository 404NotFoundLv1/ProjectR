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
};
