// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/PRSaveMigration.h"

#include "Save/PRSaveGame.h"

#include "UObject/UObjectGlobals.h"

bool FPRSaveMigrationRegistry::RegisterStep(
	const int32 FromVersion,
	const int32 ToVersion,
	FMigrationStep Step)
{
	if (FromVersion < 0 || ToVersion != FromVersion + 1 || !Step || Steps.Contains(FromVersion))
	{
		return false;
	}

	FRegisteredStep Registered;
	Registered.ToVersion = ToVersion;
	Registered.Step = MoveTemp(Step);
	Steps.Add(FromVersion, MoveTemp(Registered));
	return true;
}

bool FPRSaveMigrationRegistry::HasRegisteredSteps() const
{
	return !Steps.IsEmpty();
}

EPRSaveResult FPRSaveMigrationRegistry::Migrate(
	const UPRSaveGame& Source,
	const int32 TargetVersion,
	UPRSaveGame*& OutMigrated) const
{
	OutMigrated = nullptr;
	if (Source.SchemaVersion == 0)
	{
		return EPRSaveResult::MissingSchemaVersion;
	}
	if (Source.SchemaVersion < UPRSaveGame::MinimumMigratableVersion)
	{
		return EPRSaveResult::UnsupportedOldVersion;
	}
	if (Source.SchemaVersion > TargetVersion)
	{
		return EPRSaveResult::FutureSchemaVersion;
	}

	UPRSaveGame* WorkingCopy = DuplicateObject<UPRSaveGame>(&Source, GetTransientPackage());
	if (!WorkingCopy)
	{
		return EPRSaveResult::MigrationFailed;
	}

	while (WorkingCopy->SchemaVersion < TargetVersion)
	{
		const FRegisteredStep* Registered = Steps.Find(WorkingCopy->SchemaVersion);
		if (!Registered || Registered->ToVersion != WorkingCopy->SchemaVersion + 1 || !Registered->Step)
		{
			return EPRSaveResult::MigrationFailed;
		}

		const int32 ExpectedVersion = Registered->ToVersion;
		if (!Registered->Step(*WorkingCopy) || WorkingCopy->SchemaVersion != ExpectedVersion)
		{
			return EPRSaveResult::MigrationFailed;
		}
	}

	OutMigrated = WorkingCopy;
	return EPRSaveResult::Success;
}
