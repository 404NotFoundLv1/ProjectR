// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRProgressionAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.4.4 writer. It accepts no caller-controlled paths, classes, values, or payloads. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRProgressionAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateAndSaveFixedProgressionManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ValidateFixedProgressionManifest();
};
