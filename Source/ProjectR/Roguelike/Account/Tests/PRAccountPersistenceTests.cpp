// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Save/PRAccountSaveTypes.h"

#include "Misc/AutomationTest.h"

namespace PRAccountAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAccountPersistenceSchemaMigrationAndDeletionTransactionTest,
	"ProjectR.Account.Persistence.SchemaMigrationAndDeletionTransaction",
	PRAccountAutomation::TestFlags)

bool FPRAccountPersistenceSchemaMigrationAndDeletionTransactionTest::RunTest(const FString& Parameters)
{
	FPRAccountPersistenceData Persistence = FPRAccountPersistenceContract::MakeDefault();
	Persistence.CounterproofFragments = -1;
	TestFalse(TEXT("Negative counterproof fragments are rejected"), FPRAccountPersistenceContract::IsCanonical(Persistence));

	FPRAccountPersistenceContract::Normalize(Persistence);
	TestTrue(TEXT("Normalization restores a canonical persistence value"), FPRAccountPersistenceContract::IsCanonical(Persistence));
	TestEqual(TEXT("Normalization clamps fragments at zero"), Persistence.CounterproofFragments, int32{0});

	FPRRunSummary Summary;
	Summary.RunId = FGuid::NewGuid();
	Summary.AccountId = FGuid::NewGuid();
	Summary.IdentityId = FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Blank"));
	Summary.StartedUtc = 10;
	Summary.EndedUtc = 12;
	Summary.RoomIds = { FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("A")), FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("A")) };
	Summary.RewardIds = { FPrimaryAssetId(TEXT("ProjectRReward"), TEXT("A")), FPrimaryAssetId(TEXT("ProjectRReward"), TEXT("A")) };
	Summary.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
	TestFalse(TEXT("Duplicate stable IDs are not canonical"), FPRAccountPersistenceContract::IsSummaryCanonical(Summary));
	FPRAccountPersistenceContract::NormalizeSummary(Summary);
	TestTrue(TEXT("Normalization de-duplicates bounded stable IDs"), FPRAccountPersistenceContract::IsSummaryCanonical(Summary));
	TestEqual(TEXT("Duplicate room IDs collapse"), Summary.RoomIds.Num(), 1);
	TestEqual(TEXT("Duplicate reward IDs collapse"), Summary.RewardIds.Num(), 1);

	FPRAccountPersistenceData GraveyardPersistence = FPRAccountPersistenceContract::MakeDefault();
	for (int32 Index = 0; Index < FPRAccountPersistenceContract::MaxGraveyardRecords + 2; ++Index)
	{
		FPRAccountRecord& Record = GraveyardPersistence.Graveyard.AddDefaulted_GetRef();
		Record.RecordId = FGuid::NewGuid();
		Record.AccountId = FGuid::NewGuid();
		Record.IdentityId = Summary.IdentityId;
		Record.EndedUtc = 100 + Index;
		Record.TerminationReason = EPRAccountTerminationReason::PlayerDeath;
		Record.Summary = Summary;
		Record.Summary.RunId = FGuid::NewGuid();
		Record.Summary.AccountId = Record.AccountId;
		Record.Summary.IdentityId = Record.IdentityId;
		Record.Summary.EndedUtc = Record.EndedUtc;
		Record.Summary.StartedUtc = Record.EndedUtc - 1;
	}
	GraveyardPersistence.LifetimeDeletedAccountCount = FPRAccountPersistenceContract::MaxGraveyardRecords + 2;
	FPRAccountPersistenceContract::Normalize(GraveyardPersistence);
	TestEqual(TEXT("Graveyard retains only its bounded newest records"), GraveyardPersistence.Graveyard.Num(), FPRAccountPersistenceContract::MaxGraveyardRecords);
	TestEqual(TEXT("Lifetime deletion count never falls when records are trimmed"), GraveyardPersistence.LifetimeDeletedAccountCount, int64{FPRAccountPersistenceContract::MaxGraveyardRecords + 2});
	TestTrue(TEXT("Trimmed graveyard remains canonical"), FPRAccountPersistenceContract::IsCanonical(GraveyardPersistence));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
