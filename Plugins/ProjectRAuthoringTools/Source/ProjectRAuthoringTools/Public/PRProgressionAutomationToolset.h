// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRProgressionAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.4.4 PIE acceptance flow. It has no caller-supplied save, node, value, path, or spawn inputs. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRProgressionAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedProgressionPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedProgressionPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* VerifyFixedProgressionCleanup();
};
