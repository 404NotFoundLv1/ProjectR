// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRAuditorChapterAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates only the two fixed v0.7.0 Auditor Blueprint packages. It accepts no caller input. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRAuditorChapterAuthoringToolset final : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateV070AuditorBlueprintManifest();
};
