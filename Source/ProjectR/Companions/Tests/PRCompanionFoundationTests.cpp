// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Companions/PRCompanionTypes.h"
#include "Companions/PRCompanionDataAsset.h"
#include "Companions/PRCompanionRegistryDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"

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
	struct FExpectedCompanionAsset
	{
		const TCHAR* ObjectPath;
		const TCHAR* DisplayName;
		const TCHAR* RoleLabel;
		FGameplayTag ExpectedId;
	};

	const TArray<FGameplayTag>& CanonicalIds = FPRCompanionContract::GetCanonicalCompanionIds();
	TestEqual(TEXT("Exactly three canonical companions"), CanonicalIds.Num(), 3);
	TestTrue(TEXT("Axiom is first"), CanonicalIds.IsValidIndex(0) && CanonicalIds[0].MatchesTagExact(FPRCompanionContract::AxiomTag()));
	TestTrue(TEXT("Kindle is second"), CanonicalIds.IsValidIndex(1) && CanonicalIds[1].MatchesTagExact(FPRCompanionContract::KindleTag()));
	TestTrue(TEXT("Null is third"), CanonicalIds.IsValidIndex(2) && CanonicalIds[2].MatchesTagExact(FPRCompanionContract::NullTag()));

	const FExpectedCompanionAsset ExpectedAssets[] =
	{
		{ TEXT("/Game/ProjectR/Companions/DA_Companion_Axiom.DA_Companion_Axiom"), TEXT("律衡"), TEXT("战术分析"), FPRCompanionContract::AxiomTag() },
		{ TEXT("/Game/ProjectR/Companions/DA_Companion_Kindle.DA_Companion_Kindle"), TEXT("燧火"), TEXT("风险推进"), FPRCompanionContract::KindleTag() },
		{ TEXT("/Game/ProjectR/Companions/DA_Companion_Null.DA_Companion_Null"), TEXT("鸦零"), TEXT("漏洞观察"), FPRCompanionContract::NullTag() }
	};
	TArray<UPRCompanionDataAsset*> LoadedAssets;
	for (const FExpectedCompanionAsset& Expected : ExpectedAssets)
	{
		UPRCompanionDataAsset* Asset = LoadObject<UPRCompanionDataAsset>(nullptr, Expected.ObjectPath);
		TestNotNull(FString::Printf(TEXT("Companion asset exists: %s"), Expected.ObjectPath), Asset);
		if (!Asset)
		{
			continue;
		}

		LoadedAssets.Add(Asset);
		TestTrue(FString::Printf(TEXT("Companion id is correct: %s"), Expected.ObjectPath), Asset->CompanionId.MatchesTagExact(Expected.ExpectedId));
		TestEqual(FString::Printf(TEXT("Companion display name is correct: %s"), Expected.ObjectPath), Asset->DisplayName.ToString(), FString(Expected.DisplayName));
		TestEqual(FString::Printf(TEXT("Companion role label is correct: %s"), Expected.ObjectPath), Asset->RoleLabel.ToString(), FString(Expected.RoleLabel));
		TestFalse(FString::Printf(TEXT("Companion persona is non-empty: %s"), Expected.ObjectPath), Asset->PersonaSummary.IsEmpty());

#if WITH_EDITOR
		FDataValidationContext AssetContext;
		TestEqual(FString::Printf(TEXT("Companion asset validates: %s"), Expected.ObjectPath), Asset->IsDataValid(AssetContext), EDataValidationResult::Valid);
#endif
	}

	UPRCompanionRegistryDataAsset* Registry = LoadObject<UPRCompanionRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Companions/DA_CompanionRegistry.DA_CompanionRegistry"));
	TestNotNull(TEXT("Canonical companion registry exists"), Registry);
	if (Registry)
	{
		const TArray<TSoftObjectPtr<UPRCompanionDataAsset>>& RegistryAssets = Registry->GetCompanions();
		TestEqual(TEXT("Registry has exactly three entries"), RegistryAssets.Num(), static_cast<int32>(UE_ARRAY_COUNT(ExpectedAssets)));
		for (int32 Index = 0; Index < RegistryAssets.Num() && Index < LoadedAssets.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("Registry preserves canonical entry %d"), Index), RegistryAssets[Index].LoadSynchronous(), LoadedAssets[Index]);
		}

#if WITH_EDITOR
		FDataValidationContext RegistryContext;
		TestEqual(TEXT("Companion registry validates"), Registry->IsDataValid(RegistryContext), EDataValidationResult::Valid);
#endif
	}

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
	Subsystem->LoadRelationshipProjection();
	TestTrue(TEXT("A profile-less runtime exposes canonical default relationship snapshots"),
		FPRCompanionContract::AreCanonicalRelationshipRecords(Subsystem->RelationshipRecords));
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
