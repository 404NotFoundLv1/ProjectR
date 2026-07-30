// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRHeadmindChapterAuthoringToolset.generated.h"

class UToolCallAsyncResultString;

/** Creates only the two fixed Headmind Blueprint packages after the caller's manifest collision audit. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRHeadmindChapterAuthoringToolset final : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CreateV071HeadmindBlueprintManifest();
	/** Configures only the fixed Headmind boss Blueprint CDO with the frozen Auditor runtime dependencies. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* ConfigureV071HeadmindBossDefaults();
};
