// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDialogueAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.3.3 PIE inspection. It accepts no asset, class, tag, value, map, or profile input. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDialogueAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIECompanionDialogueSmoke();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PreparePIECompanionDialogueHumanPreview();
};
