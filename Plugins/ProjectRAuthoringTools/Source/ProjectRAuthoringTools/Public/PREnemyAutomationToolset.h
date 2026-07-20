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

	/** Runs the fixed v0.2.1-B RangedMinion vertical slice without accepting caller data or saving a Package. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyCheckpointBSmoke();

	/** Runs the fixed v0.2.1-C ShieldMinion slice without accepting caller data or saving a Package. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyCheckpointCSmoke();

	/** Runs the fixed v0.2.1-D EliteAuditGuard slice without accepting caller data or saving a Package. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyCheckpointDSmoke();

	/** Runs the fixed v0.2.1-E four-archetype integration sequence without accepting caller data or saving a Package. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyIntegrationSmoke();

	/** Prepares the fixed v0.2.1-E human-feel sequence; it reports readiness only and never reports a human verdict. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEEnemyHumanFeelSequence();
};
