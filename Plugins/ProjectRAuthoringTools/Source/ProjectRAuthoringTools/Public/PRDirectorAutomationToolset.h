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
	/** Fixed v0.4.1 verifier: applies and removes only the twelve registry rules in an active authoritative PIE world. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIEDirectorRulesSmoke();
	/** Fixed v0.4.1 verifier for ResourceBalance's real energy threshold, delayed effect, and counter cleanup. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunPIEDirectorResourceBalanceSmoke();
	/** Fixed v0.4.1 human-preview seam. It applies only RepetitionPenalty level 2 and leaves the PIE-local state visible until PIE stops. */
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* StartPIEDirectorRulePreview();
};
