// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRewardSelectionWidget.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Roguelike/PRRoomSubsystem.h"

void UPRRewardSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		RoomSubsystem = GameInstance->GetSubsystem<UPRRoomSubsystem>();
		if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get())
		{
			OfferChangedHandle = Subsystem->OnRewardOfferChanged().AddUObject(this, &UPRRewardSelectionWidget::HandleRewardOfferChanged);
			StateChangedHandle = Subsystem->OnRoomStateChanged().AddUObject(this, &UPRRewardSelectionWidget::HandleRoomStateChanged);
			FPRRoomRuntimeState State;
			Subsystem->GetRoomRuntimeState(State);
			HandleRoomStateChanged(State);
		}
	}
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice0")))) Button->OnClicked.AddDynamic(this, &UPRRewardSelectionWidget::SelectFirstReward);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice1")))) Button->OnClicked.AddDynamic(this, &UPRRewardSelectionWidget::SelectSecondReward);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Choice2")))) Button->OnClicked.AddDynamic(this, &UPRRewardSelectionWidget::SelectThirdReward);
}
void UPRRewardSelectionWidget::NativeDestruct()
{
	if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get())
	{
		Subsystem->OnRewardOfferChanged().Remove(OfferChangedHandle);
		Subsystem->OnRoomStateChanged().Remove(StateChangedHandle);
	}
	RoomSubsystem.Reset();
	Super::NativeDestruct();
}
void UPRRewardSelectionWidget::HandleRewardOfferChanged(const FPRRewardOffer& Offer)
{
	PublishedChoices.Reset(); FString Text = TEXT("Choose one reward");
	for (const FPRRewardOfferChoice& Choice : Offer.Choices) { PublishedChoices.Add(Choice.RewardId); Text += TEXT("\n") + Choice.DisplayName.ToString() + TEXT(" — ") + Choice.EffectText.ToString(); }
	if (UTextBlock* Status = Cast<UTextBlock>(GetWidgetFromName(TEXT("StatusText")))) Status->SetText(FText::FromString(Text));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasChoice = PublishedChoices.IsValidIndex(Index);
		if (UButton* Button = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Choice%d"), Index)))) Button->SetVisibility(bHasChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHasChoice)
		{
			if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("ChoiceText%d"), Index)))) Label->SetText(Offer.Choices[Index].DisplayName);
		}
	}
}
void UPRRewardSelectionWidget::HandleRoomStateChanged(const FPRRoomRuntimeState& State)
{
	const bool bSelectingReward = State.FlowStatus == EPRRoomFlowStatus::SelectingReward;
	SetVisibility(bSelectingReward ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bSelectingReward || State.FlowStatus == EPRRoomFlowStatus::Idle || State.FlowStatus == EPRRoomFlowStatus::Completed || State.FlowStatus == EPRRoomFlowStatus::Cancelled) SetSelectionInputMode(bSelectingReward);
}
void UPRRewardSelectionWidget::SetSelectionInputMode(bool bActive)
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
void UPRRewardSelectionWidget::RequestRewardSelection(const FPrimaryAssetId RewardId) { if (UPRRoomSubsystem* Subsystem = RoomSubsystem.Get()) { FGuid HandleId; Subsystem->SelectReward(RewardId, HandleId); } }
void UPRRewardSelectionWidget::SelectFirstReward() { if (PublishedChoices.IsValidIndex(0)) RequestRewardSelection(PublishedChoices[0]); }
void UPRRewardSelectionWidget::SelectSecondReward() { if (PublishedChoices.IsValidIndex(1)) RequestRewardSelection(PublishedChoices[1]); }
void UPRRewardSelectionWidget::SelectThirdReward() { if (PublishedChoices.IsValidIndex(2)) RequestRewardSelection(PublishedChoices[2]); }
