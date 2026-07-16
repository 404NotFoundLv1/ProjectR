// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRPlayerSkillPresentationToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates only the fixed v0.2.0-E placeholder SoundWave manifest; callers save explicitly. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRPlayerSkillPresentationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Creates six deterministic, non-looping PCM placeholder SoundWaves without saving any Package. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateCheckpointEPlaceholderSounds();
};
