// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "QTE/PRQTEDataAsset.h"
#include "QTE/PRQTERegistryDataAsset.h"

namespace PRQTEAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRQTEFoundationContractTest, "ProjectR.QTE.Foundation.SchemaAndRegistry", PRQTEAutomation::TestFlags)

bool FPRQTEFoundationContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The v0.3.2 contract has exactly twelve QTE identifiers"), FPRQTEContract::GetExpectedQTEIds().Num(), 12);
	TestTrue(TEXT("Axiom weakness is a canonical QTE id"), FPRQTEContract::IsExpectedQTEId(TEXT("Axiom_WeaknessAxiom")));
	TestFalse(TEXT("Unknown identifiers are rejected"), FPRQTEContract::IsExpectedQTEId(TEXT("FutureQTE")));

	UPRQTEDataAsset* Asset = NewObject<UPRQTEDataAsset>();
	Asset->QTEId = TEXT("Axiom_WeaknessAxiom");
	FDataValidationContext AssetContext;
	TestEqual(TEXT("Incomplete QTE data is invalid"), Asset->IsDataValid(AssetContext), EDataValidationResult::Invalid);
	Asset->CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"));
	Asset->QTEType = FGameplayTag::RequestGameplayTag(TEXT("QTE.Type.Attack"));
	Asset->AcceptedInputTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Input.Attack")));
	Asset->WindowSeconds = 1.0f;
	Asset->SuccessEffects.AddDefaulted();
	Asset->SuccessEffects[0].EffectKind = static_cast<EPRQTEEffectKind>(255);
	FDataValidationContext IllegalEffectContext;
	TestEqual(TEXT("Illegal effect values are rejected"), Asset->IsDataValid(IllegalEffectContext), EDataValidationResult::Invalid);

	UPRQTERegistryDataAsset* Registry = NewObject<UPRQTERegistryDataAsset>();
	FDataValidationContext RegistryContext;
	TestEqual(TEXT("An absent v0.3.2 registry is invalid"), Registry->IsDataValid(RegistryContext), EDataValidationResult::Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRQTEAssetManifestRedTest, "ProjectR.QTE.Foundation.AssetManifestRed", PRQTEAutomation::TestFlags)

bool FPRQTEAssetManifestRedTest::RunTest(const FString& Parameters)
{
	UPRQTERegistryDataAsset* Registry = LoadObject<UPRQTERegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/QTE/DA_QTERegistry.DA_QTERegistry"));
	return TestNotNull(TEXT("The required registry must exist once assets are authored"), Registry);
}

#endif
