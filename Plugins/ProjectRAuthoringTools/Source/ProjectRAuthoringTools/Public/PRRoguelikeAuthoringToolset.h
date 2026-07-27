// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRRoguelikeAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates exactly the fixed v0.4.2 Roguelike DataAsset and Widget manifest. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRRoguelikeAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* CreateAndConfigureFixedRoguelikeManifest();
};
