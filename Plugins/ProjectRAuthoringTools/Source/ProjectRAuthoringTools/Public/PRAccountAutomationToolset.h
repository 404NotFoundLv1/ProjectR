// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRAccountAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Fixed v0.4.3 account manifest and PIE observers. It accepts no arbitrary paths, slots, JSON or spawn inputs. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRAccountAutomationToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* ValidateFixedAccountIdentityManifest();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* InspectActiveAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* VerifyAccountAutomationIsolation();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedCompletionAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedDeathAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedEvacuationAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedLeaveAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedStartOnlyAccountPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedInterruptedRecoveryPIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* PrepareFixedAccountFailurePIE();
	UFUNCTION(meta=(AICallable)) static UToolCallAsyncResultString* RunFixedPersistenceFailurePIE();
};
