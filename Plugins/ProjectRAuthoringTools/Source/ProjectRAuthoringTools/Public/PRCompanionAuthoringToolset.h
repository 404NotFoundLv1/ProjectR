// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRCompanionAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/**
 * Fixed v0.3.1 manifest writer. It accepts no caller-controlled assets,
 * classes, tags, values, Blueprint text, or save operation.
 */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRCompanionAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Creates and configures exactly the seven v0.3.1 Companion packages; callers save explicitly. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateV031CompanionManifest();

	/** Saves exactly the seven fixed v0.3.1 Companion packages and no maps. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* SaveV031CompanionManifest();
};
