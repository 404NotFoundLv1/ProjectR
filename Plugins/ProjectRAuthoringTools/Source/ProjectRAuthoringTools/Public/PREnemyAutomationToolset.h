// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PREnemyAutomationToolset.generated.h"

/** Fixed, no-argument PIE inspection for the v0.2.1-A MeleeMinion vertical slice. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPREnemyAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyCheckpointASmoke();
};
