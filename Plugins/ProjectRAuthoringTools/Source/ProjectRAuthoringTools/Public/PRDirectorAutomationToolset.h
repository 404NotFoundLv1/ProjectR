// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDirectorAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.4.0 PIE verifier. It only exercises the Director's value-only session seams and never saves Packages or profiles. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDirectorAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIEDirectorSmoke();
};
