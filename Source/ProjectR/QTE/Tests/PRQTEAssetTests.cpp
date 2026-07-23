// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PRTagLibrary.h"
#include "EnhancedActionKeyMapping.h"
#include "Input/PRInputConfigDataAsset.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "QTE/PRQTEDataAsset.h"
#include "QTE/PRQTERegistryDataAsset.h"
#include "UI/PRQTEPromptWidget.h"
#include "Blueprint/UserWidget.h"

namespace PRQTEAssetAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

const TCHAR* RegistryPath = TEXT("/Game/ProjectR/QTE/DA_QTERegistry.DA_QTERegistry");
const TCHAR* RejectPath = TEXT("/Game/ProjectR/Input/Actions/IA_QTE_Reject.IA_QTE_Reject");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRQTEAssetsConfiguredTest, "ProjectR.QTE.Assets.ConfiguredManifest", PRQTEAssetAutomation::TestFlags)

bool FPRQTEAssetsConfiguredTest::RunTest(const FString& Parameters)
{
	using namespace PRQTEAssetAutomation;
	UPRQTERegistryDataAsset* Registry = LoadObject<UPRQTERegistryDataAsset>(nullptr, RegistryPath);
	if (!TestNotNull(TEXT("Fixed QTE registry exists"), Registry)) return false;
	FDataValidationContext RegistryContext;
	TestEqual(TEXT("Registry is valid"), Registry->IsDataValid(RegistryContext), EDataValidationResult::Valid);
	TestEqual(TEXT("Registry has exactly twelve ordered entries"), Registry->GetEntries().Num(), 12);
	for (int32 Index = 0; Index < Registry->GetEntries().Num(); ++Index)
	{
		UPRQTEDataAsset* Asset = Registry->GetEntries()[Index].LoadSynchronous();
		if (!TestNotNull(*FString::Printf(TEXT("Entry %d loads"), Index), Asset)) continue;
		TestEqual(*FString::Printf(TEXT("Entry %d canonical identifier"), Index), Asset->QTEId, FPRQTEContract::GetExpectedQTEIds()[Index]);
		FDataValidationContext AssetContext;
		TestEqual(*FString::Printf(TEXT("Entry %d validates"), Index), Asset->IsDataValid(AssetContext), EDataValidationResult::Valid);
		TestFalse(*FString::Printf(TEXT("Entry %d has a player-facing display name"), Index), Asset->DisplayName.IsEmpty());
		TestFalse(*FString::Printf(TEXT("Entry %d has a player-facing prompt"), Index), Asset->PromptText.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Entry %d owns a finite effect definition"), Index), Asset->SuccessEffects.Num() == 1 && Asset->SuccessEffects[0].MaxTargets >= 0);
	}

	UPRInputConfigDataAsset* Config = LoadObject<UPRInputConfigDataAsset>(nullptr, TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig.DA_PlayerInputConfig"));
	UInputAction* RejectAction = LoadObject<UInputAction>(nullptr, RejectPath);
	TestNotNull(TEXT("QTE reject InputAction exists"), RejectAction);
	if (Config && RejectAction && TestNotNull(TEXT("QTE InputConfig retains IMC_Player"), Config->DefaultMappingContext.Get()))
	{
		TestEqual(TEXT("QTE reject resolves through the frozen InputConfig"), Config->FindInputActionForTag(UPRTagLibrary::GetInputQTERejectTag()), static_cast<const UInputAction*>(RejectAction));
		int32 RCount = 0;
		int32 DPadCount = 0;
		for (const FEnhancedActionKeyMapping& Mapping : Config->DefaultMappingContext->GetMappings())
		{
			if (Mapping.Action != RejectAction) continue;
			RCount += Mapping.Key == EKeys::R ? 1 : 0;
			DPadCount += Mapping.Key == EKeys::Gamepad_DPad_Left ? 1 : 0;
		}
		TestEqual(TEXT("Reject has exactly one R mapping"), RCount, 1);
		TestEqual(TEXT("Reject has exactly one gamepad D-pad-left mapping"), DPadCount, 1);
	}

	UClass* PromptClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ProjectR/UI/QTE/WBP_QTEPrompt.WBP_QTEPrompt_C"));
	TestNotNull(TEXT("QTE prompt generated class exists"), PromptClass);
	TestTrue(TEXT("Prompt derives from the C++ read-only prompt widget"), PromptClass && PromptClass->IsChildOf(UPRQTEPromptWidget::StaticClass()));
	FGameplayTagContainer DisplayInput;
	DisplayInput.AddTag(UPRTagLibrary::GetInputSkillShadowThrustTag());
	TestEqual(TEXT("Prompt displays the configured keyboard control instead of an internal input tag"),
		UPRQTEPromptWidget::FormatAcceptedInputTags(DisplayInput).ToString(), FString(TEXT("U")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
