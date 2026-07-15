// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPlayerSkillStateEffectToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates only the two v0.2.0-C state GameplayEffects at their manifest paths. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPlayerSkillStateEffectToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateCheckpointCStateEffects();
};
