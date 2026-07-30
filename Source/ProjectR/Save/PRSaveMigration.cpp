// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/PRSaveMigration.h"

#include "Save/PRSaveGame.h"
#include "Core/PRRelationshipTypes.h"

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

void RegisterProjectRSaveMigrations(FPRSaveMigrationRegistry& Registry)
{
	Registry.RegisterStep(1, 2, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 1 || !Save.Profile.ProfileId.IsValid()
			|| !Save.Profile.CompanionRelationships.IsEmpty())
		{
			return false;
		}
		Save.Profile.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
		Save.SchemaVersion = 2;
		return true;
	});
	Registry.RegisterStep(2, 3, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 2 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships))
		{
			return false;
		}
		Save.Profile.AccountPersistence = FPRAccountPersistenceContract::MakeDefault();
		Save.SchemaVersion = 3;
		return true;
	});
	Registry.RegisterStep(3, 4, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 3 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships)
			|| !FPRAccountPersistenceContract::IsCanonical(Save.Profile.AccountPersistence))
		{
			return false;
		}
		Save.Profile.ProgressionPersistence = FPRProgressionPersistenceContract::MakeDefault();
		Save.SchemaVersion = 4;
		return true;
	});
	Registry.RegisterStep(4, 5, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 4 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships)
			|| !FPRAccountPersistenceContract::IsCanonical(Save.Profile.AccountPersistence)
			|| !FPRProgressionPersistenceContract::IsCanonical(Save.Profile.ProgressionPersistence)) return false;
		Save.Profile.CompanionQuestPersistence = FPRCompanionQuestPersistenceContract::MakeDefault();
		Save.SchemaVersion = 5;
		return true;
	});
	Registry.RegisterStep(5, 6, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 5 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships)
			|| !FPRAccountPersistenceContract::IsCanonical(Save.Profile.AccountPersistence)
			|| !FPRProgressionPersistenceContract::IsCanonical(Save.Profile.ProgressionPersistence)
			|| !FPRCompanionQuestPersistenceContract::IsCanonical(Save.Profile.CompanionQuestPersistence)) return false;
		Save.Profile.MemoryPersistence = FPRMemoryPersistenceContract::MakeDefault();
		for (const FPRAccountRecord& Record : Save.Profile.AccountPersistence.Graveyard)
		{
			Save.Profile.MemoryPersistence.LastProcessedGraveyardOrdinal = FMath::Max(
				Save.Profile.MemoryPersistence.LastProcessedGraveyardOrdinal, Record.GraveyardOrdinal);
		}
		Save.SchemaVersion = 6;
		return true;
	});
	Registry.RegisterStep(6, 7, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 6 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships)
			|| !FPRAccountPersistenceContract::IsCanonical(Save.Profile.AccountPersistence)
			|| !FPRProgressionPersistenceContract::IsCanonical(Save.Profile.ProgressionPersistence)
			|| !FPRCompanionQuestPersistenceContract::IsCanonical(Save.Profile.CompanionQuestPersistence)
			|| !FPRMemoryPersistenceContract::IsCanonical(Save.Profile.MemoryPersistence)) return false;
		Save.Profile.ChapterPersistence = FPRChapterPersistenceContract::MakeDefault();
		Save.SchemaVersion = 7;
		return true;
	});
	Registry.RegisterStep(7, 8, [](UPRSaveGame& Save)
	{
		if (Save.SchemaVersion != 7 || !Save.Profile.ProfileId.IsValid()
			|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Save.Profile.CompanionRelationships)
			|| !FPRAccountPersistenceContract::IsCanonical(Save.Profile.AccountPersistence)
			|| !FPRProgressionPersistenceContract::IsCanonical(Save.Profile.ProgressionPersistence)
			|| !FPRCompanionQuestPersistenceContract::IsCanonical(Save.Profile.CompanionQuestPersistence)
			|| !FPRMemoryPersistenceContract::IsCanonical(Save.Profile.MemoryPersistence)
			|| !FPRChapterPersistenceContract::IsCanonical(Save.Profile.ChapterPersistence)) return false;
		Save.Profile.TripleResonancePersistence = FPRTripleResonancePersistenceContract::MakeDefault();
		for (const FPRAccountRecord& Record : Save.Profile.AccountPersistence.Graveyard)
		{
			Save.Profile.TripleResonancePersistence.LastProcessedGraveyardOrdinal = FMath::Max(
				Save.Profile.TripleResonancePersistence.LastProcessedGraveyardOrdinal, Record.GraveyardOrdinal);
		}
		Save.SchemaVersion = 8;
		return true;
	});
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
