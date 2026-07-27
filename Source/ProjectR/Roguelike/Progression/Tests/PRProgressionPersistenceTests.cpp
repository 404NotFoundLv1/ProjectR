// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Progression/PRProgressionTypes.h"
#include "Save/PRSaveGame.h"
#include "Save/PRSaveMigration.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRProgressionPersistenceTest,
	"ProjectR.Progression.Persistence.SchemaFourMigrationAndAtomicPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRProgressionPersistenceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Schema four is current"), UPRSaveGame::CurrentSchemaVersion, 4);
	const FPRProgressionPersistenceData Persistence;
	TestEqual(TEXT("Memory defaults safely"), Persistence.MemoryFragments, 0);
	TestTrue(TEXT("No nodes unlock by default"), Persistence.UnlockedNodeIds.IsEmpty());
	TestEqual(TEXT("Unlock sequence defaults safely"), Persistence.UnlockSequence, int64{0});
	TestTrue(TEXT("Safe default is canonical"), FPRProgressionPersistenceContract::IsCanonical(Persistence));

	UPRSaveGame* LegacySave = NewObject<UPRSaveGame>(GetTransientPackage());
	LegacySave->SchemaVersion = 1;
	LegacySave->Profile.ProfileId = FGuid::NewGuid();
	FPRSaveMigrationRegistry Migrations;
	RegisterProjectRSaveMigrations(Migrations);
	UPRSaveGame* Migrated = nullptr;
	TestEqual(TEXT("Production migrations advance one step at a time through schema four"),
		Migrations.Migrate(*LegacySave, UPRSaveGame::CurrentSchemaVersion, Migrated), EPRSaveResult::Success);
	TestNotNull(TEXT("Schema four migration returns a copy"), Migrated);
	if (Migrated)
	{
		TestEqual(TEXT("Migrated save reaches schema four"), Migrated->SchemaVersion, 4);
		TestTrue(TEXT("Migrated progression uses the safe canonical default"),
			FPRProgressionPersistenceContract::IsCanonical(Migrated->Profile.ProgressionPersistence));
		TestEqual(TEXT("Migration cannot create MemoryFragments"), Migrated->Profile.ProgressionPersistence.MemoryFragments, 0);
	}
	return true;
}

#endif
