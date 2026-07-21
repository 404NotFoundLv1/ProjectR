// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PRBossAuthoringToolset.generated.h"

/** Fixed v0.2.2 manifest writer. It accepts no caller-controlled asset, class, tag, or graph input. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRBossAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* CreateAndSaveAuditorBossManifest();
	/** Repairs only the existing fixed v0.2.2 manifest in place. It never creates, deletes, moves, or renames Packages. */
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* RepairAndSaveAuditorBossManifest();
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* ConfigureAndSaveBossGymGameMode();
	/** Runs the fixed, no-argument v0.2.2 BossGym verification sequence in an already active authoritative PIE world. */
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* RunPIEAuditorBossSmoke();
};
