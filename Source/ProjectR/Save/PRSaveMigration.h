// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRSaveTypes.h"

class UPRSaveGame;

/** Ordered registry for exact one-version-at-a-time business schema migrations. */
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
