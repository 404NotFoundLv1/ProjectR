// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRBalanceAutomationToolset.generated.h"

/**
 * Fixed, editor-only v0.2.4 balance evidence.  Every operation is no-argument
 * and may only inspect the formal ProjectR maps, registry, and runtime state.
 */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRBalanceAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Captures the authoritative asset snapshot without saving a UE package. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* CaptureV024BalanceSnapshot();

	/** Runs the fixed 2 Melee + 1 Ranged + 1 Shield CombatGym benchmark in active authoritative PIE. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEV024NormalEncounterBenchmark();

	/** Verifies formal Shield -> Health -> Death risk in active authoritative CombatGym PIE. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEV024PlayerDeathRiskSmoke();

	/** Runs the fixed Auditor P1/P2/P3 benchmark in active authoritative BossGym PIE. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RunPIEV024AuditorBenchmark();
};
