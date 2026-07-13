// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Save/PRSaveTypes.h"

#include "GameFramework/SaveGame.h"

#include "PRSaveGame.generated.h"

/** Minimal versioned root object for ProjectR profile persistence. */
UCLASS()
class PROJECTR_API UPRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSchemaVersion = 1;
	static constexpr int32 MinimumMigratableVersion = 1;

	UPROPERTY(SaveGame)
	int32 SchemaVersion = 0;

	UPROPERTY(SaveGame)
	int64 SaveRevision = 0;

	UPROPERTY(SaveGame)
	FPRProfileSaveData Profile;
};
