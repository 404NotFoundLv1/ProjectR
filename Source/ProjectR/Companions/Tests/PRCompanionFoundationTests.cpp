// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Companions/PRCompanionTypes.h"
#include "Companions/PRCompanionDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

namespace PRCompanionAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionFoundationTest,
	"ProjectR.Companion.Foundation.IdentityRegistryAndRelationshipClamp",
	PRCompanionAutomation::TestFlags)

bool FPRCompanionFoundationTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag>& CanonicalIds = FPRCompanionContract::GetCanonicalCompanionIds();
	TestEqual(TEXT("Exactly three canonical companions"), CanonicalIds.Num(), 3);
	TestTrue(TEXT("Axiom is first"), CanonicalIds.IsValidIndex(0) && CanonicalIds[0].MatchesTagExact(FPRCompanionContract::AxiomTag()));
	TestTrue(TEXT("Kindle is second"), CanonicalIds.IsValidIndex(1) && CanonicalIds[1].MatchesTagExact(FPRCompanionContract::KindleTag()));
	TestTrue(TEXT("Null is third"), CanonicalIds.IsValidIndex(2) && CanonicalIds[2].MatchesTagExact(FPRCompanionContract::NullTag()));

	const TArray<FPRCompanionRelationshipRecord> Defaults = FPRCompanionContract::BuildDefaultRelationshipRecords();
	TestTrue(TEXT("Default records are canonical"), FPRCompanionContract::AreCanonicalRelationshipRecords(Defaults));
	TestEqual(TEXT("Default trust"), Defaults[0].State.Trust, 50);
	TestEqual(TEXT("Default affection"), Defaults[0].State.Affection, 50);
	TestEqual(TEXT("Default evaluation"), Defaults[0].State.Evaluation, 50);
	TestEqual(TEXT("Default overload"), Defaults[0].State.Overload, 0);

	FPRRelationshipState Clamped = Defaults[0].State;
	FPRRelationshipDelta Delta;
	Delta.CompanionId = Defaults[0].CompanionId;
	Delta.TrustDelta = 1000;
	Delta.AffectionDelta = -1000;
	Delta.EvaluationDelta = 51;
	Delta.OverloadDelta = -1000;
	Delta.SourceId = TEXT("CompanionAutomation");
	TestTrue(TEXT("Valid relationship delta applies"), FPRCompanionContract::ApplyDelta(Clamped, Delta));
	TestEqual(TEXT("Trust clamps at 100"), Clamped.Trust, 100);
	TestEqual(TEXT("Affection clamps at 0"), Clamped.Affection, 0);
	TestEqual(TEXT("Evaluation clamps at 100"), Clamped.Evaluation, 100);
	TestEqual(TEXT("Overload clamps at 0"), Clamped.Overload, 0);

	Delta.SourceId = NAME_None;
	TestFalse(TEXT("Unnamed relationship source is rejected"), FPRCompanionContract::ApplyDelta(Clamped, Delta));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionRuntimeTest,
	"ProjectR.Companion.Runtime.PrimarySyncAndTravel",
	PRCompanionAutomation::TestFlags)

bool FPRCompanionRuntimeTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRCompanionSubsystem* Subsystem = NewObject<UPRCompanionSubsystem>(GameInstance);
	Subsystem->bRegistryReady = true;
	for (const FGameplayTag& Id : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		UPRCompanionDataAsset* Asset = NewObject<UPRCompanionDataAsset>(Subsystem);
		Asset->CompanionId = Id;
		Subsystem->LoadedCompanions.Add(Id, Asset);
	}

	TestEqual(TEXT("First primary selection succeeds"),
		Subsystem->SelectPrimaryCompanion(FPRCompanionContract::KindleTag()),
		EPRPrimaryCompanionSelectionResult::Selected);
	const FPRCompanionSyncState FirstState = Subsystem->GetSyncState();
	TestTrue(TEXT("Kindle is primary"), FirstState.PrimaryCompanionId.MatchesTagExact(FPRCompanionContract::KindleTag()));
	TestEqual(TEXT("Exactly two companions are background"), FirstState.BackgroundCompanionIds.Num(), 2);
	TestTrue(TEXT("Background order retains Axiom first"), FirstState.BackgroundCompanionIds[0].MatchesTagExact(FPRCompanionContract::AxiomTag()));
	TestTrue(TEXT("Background order retains Null second"), FirstState.BackgroundCompanionIds[1].MatchesTagExact(FPRCompanionContract::NullTag()));
	TestEqual(TEXT("Repeated selection is idempotent"),
		Subsystem->SelectPrimaryCompanion(FPRCompanionContract::KindleTag()),
		EPRPrimaryCompanionSelectionResult::AlreadySelected);
	TestEqual(TEXT("Unknown companion is rejected"),
		Subsystem->SelectPrimaryCompanion(FGameplayTag::RequestGameplayTag(TEXT("Skill.BasicAttack"), false)),
		EPRPrimaryCompanionSelectionResult::RejectedUnknownCompanion);
	return true;
}

#endif
