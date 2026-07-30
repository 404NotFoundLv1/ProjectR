// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRHeadmindChapterAutomationToolset.generated.h"

class UToolCallAsyncResultString;

/** Reads only the fixed Headmind Registry closure; it never accepts caller-controlled content identifiers. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRHeadmindChapterAutomationToolset final : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* InspectFixedHeadmindRegistry();
	/** Reads the active fixed PIE Room/Boss value snapshot; it never writes gameplay or persistence state. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* InspectFixedHeadmindPIEState();
	/** Fixed, no-argument PIE preparation; it creates only an isolated in-memory A/B save backend. */
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* PrepareFixedHeadmindPIE();
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunFixedHeadmindSelectionPIE();
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunFixedHeadmindFullPathPIE();
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* RunFixedHeadmindPersistenceRetryPIE();
	UFUNCTION(meta=(AICallable))
	static UToolCallAsyncResultString* CleanupFixedHeadmindPIE();
};
