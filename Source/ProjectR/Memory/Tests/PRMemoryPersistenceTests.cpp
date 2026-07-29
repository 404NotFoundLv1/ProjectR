// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Save/PRMemorySaveTypes.h"
#include "Save/PRSaveGame.h"
#include "Save/PRSaveMigration.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRMemoryPersistenceTest,
	"ProjectR.Memory.Persistence.BoundedCanonicalMemoryPartition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRMemoryPersistenceTest::RunTest(const FString&)
{
	FPRMemoryPersistenceData Persistence;
	Persistence.LastProcessedGraveyardOrdinal = -1;
	Persistence.LifetimeSummaryCount = -1;
	Persistence.LifetimeMemoryFragmentsAwarded = -1;
	Persistence.SummarySequence = -1;

	FPRMemoryPersistenceContract::Normalize(Persistence);
	TestTrue(TEXT("Default normalized Memory persistence is canonical"),
		FPRMemoryPersistenceContract::IsCanonical(Persistence));
	TestEqual(TEXT("Negative graveyard watermark normalizes to zero"), Persistence.LastProcessedGraveyardOrdinal, int64{0});
	TestEqual(TEXT("Negative summary sequence normalizes to zero"), Persistence.SummarySequence, int64{0});

	FPRMemorySummary Invalid;
	Invalid.SummaryId = FGuid::NewGuid();
	Invalid.GraveyardOrdinal = 1;
	Invalid.SummarySequence = 1;
	Invalid.SceneId = TEXT("post_run_summary");
	Invalid.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
	Invalid.EmotionId = TEXT("analytical");
	Invalid.SummaryText = TEXT("bounded");
	Invalid.PlayerOptionIds = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };
	Invalid.KeyEventIds = { TEXT("Run.Termination.PlayerDeath") };
	Persistence.Summaries.Add(Invalid);
	Persistence.LastProcessedGraveyardOrdinal = Invalid.GraveyardOrdinal;
	Persistence.LifetimeSummaryCount = 1;
	TestTrue(TEXT("A fixed valid summary remains canonical"), FPRMemoryPersistenceContract::IsCanonical(Persistence));

	Persistence.Summaries.Add(Invalid);
	TestFalse(TEXT("Duplicate SummaryId is rejected"), FPRMemoryPersistenceContract::IsCanonical(Persistence));

	FPRMemoryPersistenceData LongRun;
	LongRun.LifetimeSummaryCount = 40;
	for (int32 Ordinal = 40; Ordinal >= 1; --Ordinal)
	{
		FPRMemorySummary& Summary = LongRun.Summaries.AddDefaulted_GetRef();
		Summary.SummaryId = FGuid::NewGuid();
		Summary.TerminationReason = Ordinal % 5 == 0 ? EPRAccountTerminationReason::InterruptedRecovery : EPRAccountTerminationReason::PlayerDeath;
		Summary.GraveyardOrdinal = Ordinal;
		Summary.SummarySequence = Ordinal;
		Summary.SceneId = TEXT("post_run_summary");
		Summary.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
		Summary.EmotionId = TEXT("analytical");
		Summary.SummaryText = TEXT("bounded run");
		Summary.PlayerOptionIds = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };
		Summary.KeyEventIds = { TEXT("Run.Termination.PlayerDeath") };
		Summary.MemoryFragmentsAwarded = Summary.TerminationReason == EPRAccountTerminationReason::InterruptedRecovery ? 0 : 1;
	}
	FPRMemoryPersistenceContract::Normalize(LongRun);
	TestTrue(TEXT("Forty mixed terminal records normalize to a bounded canonical history"), FPRMemoryPersistenceContract::IsCanonical(LongRun));
	TestEqual(TEXT("Long-running Memory history retains only thirty-two summaries"), LongRun.Summaries.Num(), FPRMemoryPersistenceContract::MaxSummaries);
	TestEqual(TEXT("Bounded history keeps the newest deterministic graveyard window"), LongRun.Summaries[0].GraveyardOrdinal, int64{9});
	TestEqual(TEXT("An interrupted recovery in the retained history carries zero fragments"), LongRun.Summaries[1].MemoryFragmentsAwarded, 0);
	TestEqual(TEXT("A non-interrupted retained record preserves its single fragment value"), LongRun.Summaries[4].MemoryFragmentsAwarded, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRMemorySchemaSixTest,
	"ProjectR.Memory.Persistence.SchemaSixMigrationStartsAtExistingGraveyardWatermark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRMemorySchemaSixTest::RunTest(const FString&)
{
	TestEqual(TEXT("The current Save schema retains Memory's Schema 6 migration before Chapter Schema 7"), UPRSaveGame::CurrentSchemaVersion, 7);
	UPRSaveGame* Source = NewObject<UPRSaveGame>();
	Source->SchemaVersion = 5;
	Source->SaveRevision = 1;
	Source->Profile.ProfileId = FGuid::NewGuid();
	Source->Profile.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
	FPRAccountRecord Record;
	Record.RecordId = FGuid::NewGuid();
	Record.AccountId = FGuid::NewGuid();
	Record.IdentityId = FPrimaryAssetId(TEXT("AccountIdentity"), TEXT("Technician"));
	Record.EndedUtc = 10;
	Record.GraveyardOrdinal = 7;
	Record.Summary.RunId = FGuid::NewGuid();
	Record.Summary.AccountId = Record.AccountId;
	Record.Summary.IdentityId = Record.IdentityId;
	Record.Summary.StartedUtc = 1;
	Record.Summary.EndedUtc = 10;
	Record.Summary.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
	Source->Profile.AccountPersistence.Graveyard.Add(Record);
	FPRAccountPersistenceContract::Normalize(Source->Profile.AccountPersistence);

	FPRSaveMigrationRegistry Registry;
	RegisterProjectRSaveMigrations(Registry);
	UPRSaveGame* Migrated = nullptr;
	TestEqual(TEXT("Schema five migrates to six"), Registry.Migrate(*Source, 6, Migrated), EPRSaveResult::Success);
	TestNotNull(TEXT("Schema six migration returns a copy"), Migrated);
	if (Migrated)
	{
		TestTrue(TEXT("Migration creates an empty Memory summary list"), Migrated->Profile.MemoryPersistence.Summaries.IsEmpty());
		TestEqual(TEXT("Migration uses the existing graveyard watermark without rewards"), Migrated->Profile.MemoryPersistence.LastProcessedGraveyardOrdinal, int64{7});
	}
	return true;
}

#endif
