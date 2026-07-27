// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRoomEventWidget.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"

void UPRRoomEventWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		RoomSubsystem = GameInstance->GetSubsystem<UPRRoomSubsystem>();
		if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) { StateChangedHandle = Subsystem->OnRoomStateChanged().AddUObject(this, &UPRRoomEventWidget::HandleRoomStateChanged); EventResolvedHandle = Subsystem->OnRoomEventResolved().AddUObject(this, &UPRRoomEventWidget::HandleEventResolved); FPRRoomRuntimeState State; Subsystem->GetRoomRuntimeState(State); HandleRoomStateChanged(State); }
	}
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice0")))) Button->OnClicked.AddDynamic(this, &UPRRoomEventWidget::SelectFirstEventChoice);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice1")))) Button->OnClicked.AddDynamic(this, &UPRRoomEventWidget::SelectSecondEventChoice);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice2")))) Button->OnClicked.AddDynamic(this, &UPRRoomEventWidget::SelectThirdEventChoice);
}
void UPRRoomEventWidget::NativeDestruct()
{
	if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) { Subsystem->OnRoomStateChanged().Remove(StateChangedHandle); Subsystem->OnRoomEventResolved().Remove(EventResolvedHandle); }
	RoomSubsystem.Reset();
	Super::NativeDestruct();
}
void UPRRoomEventWidget::HandleRoomStateChanged(const FPRRoomRuntimeState& State)
{
	PublishedChoices.Reset(); FString Text = TEXT("Room event");
	const bool bSelectingEvent = State.FlowStatus == EPRRoomFlowStatus::SelectingEvent;
	if (bSelectingEvent)
	{
		UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
		if (const UPRRoomEventDataAsset* Event = Registry ? Registry->FindEvent(Registry->FindEventForRoom(State.ActiveRoomId)) : nullptr) for (const FPRRoomEventChoice& Choice : Event->Choices) { PublishedChoices.Add(Choice.ChoiceId); Text += TEXT("\n") + Choice.DisplayName.ToString() + TEXT(" — ") + Choice.Description.ToString(); }
	}
	SetVisibility(bSelectingEvent && !PublishedChoices.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	const bool bEventSelectionActive = bSelectingEvent && !PublishedChoices.IsEmpty();
	if (bEventSelectionActive || State.FlowStatus == EPRRoomFlowStatus::Idle || State.FlowStatus == EPRRoomFlowStatus::Completed || State.FlowStatus == EPRRoomFlowStatus::Cancelled) SetSelectionInputMode(bEventSelectionActive);
	if (UTextBlock* Status = Cast<UTextBlock>(GetWidgetFromName(TEXT("StatusText")))) Status->SetText(FText::FromString(Text));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasChoice = PublishedChoices.IsValidIndex(Index);
		if (UButton* Button = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Choice%d"), Index)))) Button->SetVisibility(bHasChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHasChoice)
		{
			UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
			if (const UPRRoomEventDataAsset* Event = Registry ? Registry->FindEvent(Registry->FindEventForRoom(State.ActiveRoomId)) : nullptr)
			{
				if (Event->Choices.IsValidIndex(Index))
				{
					if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("ChoiceText%d"), Index)))) Label->SetText(Event->Choices[Index].DisplayName);
				}
			}
		}
	}
}
void UPRRoomEventWidget::HandleEventResolved(const FPRRoomEventResult& Result) { PublishedChoices.Reset(); }
void UPRRoomEventWidget::SetSelectionInputMode(bool bActive)
{
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (bActive)
		{
			FInputModeGameAndUI InputMode; InputMode.SetWidgetToFocus(TakeWidget()); InputMode.SetHideCursorDuringCapture(false);
			Controller->SetInputMode(InputMode); Controller->bShowMouseCursor = true; Controller->bEnableClickEvents = true; Controller->bEnableMouseOverEvents = true;
		}
		else
		{
			FInputModeGameOnly InputMode;
			Controller->SetInputMode(InputMode); Controller->bShowMouseCursor = false; Controller->bEnableClickEvents = false; Controller->bEnableMouseOverEvents = false;
		}
	}
}
void UPRRoomEventWidget::RequestEventChoice(const FName ChoiceId) { if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) Subsystem->SelectEventChoice(ChoiceId); }
void UPRRoomEventWidget::SelectFirstEventChoice() { if (PublishedChoices.IsValidIndex(0)) RequestEventChoice(PublishedChoices[0]); }
void UPRRoomEventWidget::SelectSecondEventChoice() { if (PublishedChoices.IsValidIndex(1)) RequestEventChoice(PublishedChoices[1]); }
void UPRRoomEventWidget::SelectThirdEventChoice() { if (PublishedChoices.IsValidIndex(2)) RequestEventChoice(PublishedChoices[2]); }
