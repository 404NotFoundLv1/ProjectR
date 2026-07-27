// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRoomFlowWidget.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"

FText UPRRoomFlowWidget::GetChoiceLabel(const UPRRoguelikeContentRegistryDataAsset& Registry, const FPrimaryAssetId& RoomId)
{
	if (const UPRRoomDataAsset* Room = Registry.FindRoom(RoomId))
	{
		return Room->DisplayName;
	}
	return FText::FromString(RoomId.ToString());
}

void UPRRoomFlowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		RoomSubsystem = GameInstance->GetSubsystem<UPRRoomSubsystem>();
		if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) { StateChangedHandle = Subsystem->OnRoomStateChanged().AddUObject(this, &UPRRoomFlowWidget::HandleRoomStateChanged); FPRRoomRuntimeState State; Subsystem->GetRoomRuntimeState(State); HandleRoomStateChanged(State); }
	}
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice0")))) Button->OnClicked.AddDynamic(this, &UPRRoomFlowWidget::SelectFirstRoom);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice1")))) Button->OnClicked.AddDynamic(this, &UPRRoomFlowWidget::SelectSecondRoom);
}

void UPRRoomFlowWidget::NativeDestruct()
{
	if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) Subsystem->OnRoomStateChanged().Remove(StateChangedHandle);
	RoomSubsystem.Reset();
	Super::NativeDestruct();
}

void UPRRoomFlowWidget::HandleRoomStateChanged(const FPRRoomRuntimeState& State)
{
	PublishedChoices.Reset();
	if (State.FlowStatus == EPRRoomFlowStatus::SelectingRoom && State.Path.IsValidIndex(State.CurrentStepIndex + 1)) PublishedChoices = State.Path[State.CurrentStepIndex + 1].CandidateRoomIds;
	const bool bSelectingRoom = State.FlowStatus == EPRRoomFlowStatus::SelectingRoom && !PublishedChoices.IsEmpty();
	SetVisibility(bSelectingRoom ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bSelectingRoom || State.FlowStatus == EPRRoomFlowStatus::Idle || State.FlowStatus == EPRRoomFlowStatus::Completed || State.FlowStatus == EPRRoomFlowStatus::Cancelled) SetSelectionInputMode(bSelectingRoom);
	if (UTextBlock* Status = Cast<UTextBlock>(GetWidgetFromName(TEXT("StatusText")))) Status->SetText(FText::FromString(TEXT("Choose next room")));

	UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasChoice = PublishedChoices.IsValidIndex(Index);
		if (UButton* Button = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Choice%d"), Index)))) Button->SetVisibility(bHasChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHasChoice)
		{
			if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("ChoiceText%d"), Index))))
			{
				Label->SetText(Registry ? GetChoiceLabel(*Registry, PublishedChoices[Index]) : FText::FromString(PublishedChoices[Index].ToString()));
			}
		}
	}
}

void UPRRoomFlowWidget::SetSelectionInputMode(bool bActive)
{
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (bActive)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetHideCursorDuringCapture(false);
			Controller->SetInputMode(InputMode);
			Controller->bShowMouseCursor = true;
			Controller->bEnableClickEvents = true;
			Controller->bEnableMouseOverEvents = true;
		}
		else
		{
			FInputModeGameOnly InputMode;
			Controller->SetInputMode(InputMode);
			Controller->bShowMouseCursor = false;
			Controller->bEnableClickEvents = false;
			Controller->bEnableMouseOverEvents = false;
		}
	}
}
void UPRRoomFlowWidget::RequestRoomSelection(const FPrimaryAssetId RoomId) { if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) Subsystem->SelectRoom(RoomId); }
void UPRRoomFlowWidget::SelectFirstRoom() { if (PublishedChoices.IsValidIndex(0)) RequestRoomSelection(PublishedChoices[0]); }
void UPRRoomFlowWidget::SelectSecondRoom() { if (PublishedChoices.IsValidIndex(1)) RequestRoomSelection(PublishedChoices[1]); }
