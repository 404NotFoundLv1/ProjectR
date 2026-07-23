// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PRRelationshipTypes.h"
#include "Save/PRSaveGame.h"
#include "Save/PRSaveMigration.h"
#include "Save/PRSaveStorage.h"

#include "Misc/AutomationTest.h"

namespace PRCompanionPersistenceAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionPersistenceTest,
	"ProjectR.Companion.Persistence.SchemaTwoMigrationAndRoundTrip",
	PRCompanionPersistenceAutomation::TestFlags)

bool FPRCompanionPersistenceTest::RunTest(const FString& Parameters)
{
	UPRSaveGame* SchemaOne = NewObject<UPRSaveGame>();
	SchemaOne->SchemaVersion = 1;
	SchemaOne->SaveRevision = 7;
	SchemaOne->Profile.ProfileId = FGuid::NewGuid();

	FPRSaveMigrationRegistry Registry;
	RegisterProjectRSaveMigrations(Registry);
	UPRSaveGame* Migrated = nullptr;
	TestEqual(TEXT("Schema one migrates to schema two"), Registry.Migrate(*SchemaOne, UPRSaveGame::CurrentSchemaVersion, Migrated), EPRSaveResult::Success);
	TestNotNull(TEXT("Migration produces a copy"), Migrated);
	if (Migrated)
	{
		TestEqual(TEXT("Migration reaches schema two"), Migrated->SchemaVersion, 2);
		TestEqual(TEXT("Migration preserves profile id"), Migrated->Profile.ProfileId, SchemaOne->Profile.ProfileId);
		TestEqual(TEXT("Migration preserves revision"), Migrated->SaveRevision, SchemaOne->SaveRevision);
		TestTrue(TEXT("Migration adds canonical default companion relationships"), FPRCompanionContract::AreCanonicalRelationshipRecords(Migrated->Profile.CompanionRelationships));
	}
	TestEqual(TEXT("Source remains schema one"), SchemaOne->SchemaVersion, 1);
	TestTrue(TEXT("Source does not receive migration records"), SchemaOne->Profile.CompanionRelationships.IsEmpty());

	UPRSaveGame* NewSave = NewObject<UPRSaveGame>();
	NewSave->SchemaVersion = UPRSaveGame::CurrentSchemaVersion;
	NewSave->SaveRevision = 1;
	NewSave->Profile.ProfileId = FGuid::NewGuid();
	NewSave->Profile.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
	TestTrue(TEXT("Schema two defaults are canonical"), FPRCompanionContract::AreCanonicalRelationshipRecords(NewSave->Profile.CompanionRelationships));

	TArray<uint8> Envelope;
	TestEqual(TEXT("Schema two serializes into the PRSV envelope"), FPRSaveEnvelope::Serialize(*NewSave, Envelope), EPRSaveResult::Success);
	FPRSaveDecodedEnvelope Decoded;
	TestEqual(TEXT("Schema two deserializes without a migration step"), FPRSaveEnvelope::Deserialize(Envelope, Decoded), EPRSaveResult::Success);
	TestNotNull(TEXT("Round trip returns a save object"), Decoded.SaveGame);
	if (Decoded.SaveGame)
	{
		TestEqual(TEXT("Round trip preserves profile id"), Decoded.SaveGame->Profile.ProfileId, NewSave->Profile.ProfileId);
		TestEqual(TEXT("Round trip preserves revision"), Decoded.SaveGame->SaveRevision, NewSave->SaveRevision);
		TestTrue(TEXT("Round trip preserves canonical companion relationships"), FPRCompanionContract::AreCanonicalRelationshipRecords(Decoded.SaveGame->Profile.CompanionRelationships));
	}
	return true;
}

#endif
