// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPlayerSkillAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Narrow editor-only toolset that exercises ProjectR's fixed v0.2.0 player-skill sequences in PIE. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPlayerSkillAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Runs the fixed ShadowThrust, FireSlash, Burning, input, and cleanup sequence without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointBSmoke();

	/** Runs the fixed ThunderDrop, AfterimageDodge, status, decoy, and cleanup sequence without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointCSmoke();

	/** Runs the unchanged C sequence against six P0 skills plus the formal BasicAttack spec without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointECRegressionSmoke();

	/** Runs the fixed VectorHook and CounterProofWall sequence without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointDSmoke();

	/** Runs the unchanged D sequence with an in-call-stack CounterProofWall perfect-block assertion. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointEDRegressionSmoke();

	/** Runs the fixed six-skill presentation preload while also validating the formal BasicAttack spec, without saving a Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunPIEPlayerSkillCheckpointESmoke();
};
