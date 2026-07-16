// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPlayerSkillStateEffectToolset.generated.h"

class UToolCallAsyncResultString;

/** Configures the fixed v0.2.0-C/D state-effect and AbilitySet manifest; callers save explicitly. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPlayerSkillStateEffectToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateCheckpointCStateEffects();

	/** Binds only the fixed checkpoint-D Guarding GE to its formal CounterProofWall GA CDO. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* ConfigureCheckpointDGuardingBinding();

	/** Appends only the two fixed checkpoint-D formal skills after the frozen four-entry default set. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* ConfigureCheckpointDAbilitySet();
};
