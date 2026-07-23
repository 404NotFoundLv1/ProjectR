// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRQTEAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.3.2 PIE QTE smoke checks. They accept no caller data, save no package and never touch a profile directly. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRQTEAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEAxiomSmoke();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEAxiomProbabilityShield();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEAxiomPhaseCut();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEAxiomCooperativeRefutation();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEKindleSmoke();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEKindleBreakout();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEKindleBurnoutTornado();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTEKindleReverseBurnRescue();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTENullSmoke();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTENullDecoyJudgment();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTENullGarbageCollection();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* RunPIEQTENullVerdictTamper();
	/** Fixed human-only previews: they raise one formal QTE and never synthesize its accepted input. */
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* PreparePIEQTEAxiomHumanPreview();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* PreparePIEQTEKindleHumanPreview();
	UFUNCTION(meta = (AICallable)) static UToolCallAsyncResultString* PreparePIEQTENullHumanPreview();
};
