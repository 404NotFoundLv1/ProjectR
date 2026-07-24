// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDivergenceAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.3.4 PIE verifier. It uses an in-memory test profile and never saves a user profile or Package. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDivergenceAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIEDivergenceSmoke();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIEDivergenceHumanPreview();
};
