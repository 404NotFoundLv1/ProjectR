// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRDirectorRuleAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates only the fixed v0.4.1 Director Rule manifest; callers save exact dirty packages separately. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRDirectorRuleAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateAndConfigureFixedDirectorRuleManifest();
};
