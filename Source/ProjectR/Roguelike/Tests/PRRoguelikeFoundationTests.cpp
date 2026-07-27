// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Roguelike/PRRoomSubsystem.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomTypes.h"
#include "Roguelike/PRRewardTypes.h"
#include "UI/PRRoomFlowWidget.h"

namespace PRRoguelikeAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRoguelikeFoundationRegistryAndSchemaTest,
	"ProjectR.Roguelike.Foundation.RegistryAndSchema",
	PRRoguelikeAutomation::TestFlags)

bool FPRRoguelikeFoundationRegistryAndSchemaTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Room subsystem is a GameInstance subsystem"), UPRRoomSubsystem::StaticClass());

	FPRRoomRuntimeState RuntimeState;
	TestEqual(TEXT("A new room state is idle"), RuntimeState.FlowStatus, EPRRoomFlowStatus::Idle);

	FPRRewardOffer Offer;
	TestEqual(TEXT("A new reward offer has no choices"), Offer.Choices.Num(), 0);

	UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
	TestNotNull(TEXT("The fixed registry is available through its exact manifest path"), Registry);
	if (!Registry) return false;
	TestTrue(TEXT("The fixed registry rejects missing or unknown content by validating all references"), Registry->IsRegistryReady());
	TestEqual(TEXT("The fixed registry has exactly four event bindings"), Registry->EventRoomBindings.Num(), 4);
	TestEqual(TEXT("The fixed registry exposes only three approved Director room-weight seams"), Registry->DirectorRoomWeightAdjustments.Num(), 3);
	const FPrimaryAssetId CombatRoomId(TEXT("ProjectRRoom"), TEXT("DA_Room_CombatStandard"));
	TestEqual(TEXT("Room selection projects the configured player-facing room name"), UPRRoomFlowWidget::GetChoiceLabel(*Registry, CombatRoomId).ToString(), FString(TEXT("CombatStandard")));
	return true;
}
