// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRSaveTypes.h"

class UPRSaveGame;

/** Strict, copy-on-write, adjacent-schema migration registry. */
class PROJECTR_API FPRSaveMigrationRegistry
{
public:
	using FMigrationStep = TFunction<bool(UPRSaveGame&)>;

	bool RegisterStep(int32 FromVersion, int32 ToVersion, FMigrationStep Step);
	bool HasRegisteredSteps() const;
	EPRSaveResult Migrate(const UPRSaveGame& Source, int32 TargetVersion, UPRSaveGame*& OutMigrated) const;

private:
	struct FRegisteredStep
	{
		int32 ToVersion = 0;
		FMigrationStep Step;
	};

	TMap<int32, FRegisteredStep> Steps;
};

/** Registers every shipped ProjectR migration, currently Schema 1 -> 2. */
PROJECTR_API void RegisterProjectRSaveMigrations(FPRSaveMigrationRegistry& Registry);
