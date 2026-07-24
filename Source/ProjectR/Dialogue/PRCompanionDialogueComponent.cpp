// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/PRCompanionDialogueComponent.h"

#include "Characters/PRPlayerCharacter.h"
#include "Core/PRTagLibrary.h"
#include "Dialogue/PRCompanionDialogueSubsystem.h"
#include "Input/PRInputTypes.h"
#include "UI/PRCompanionDialogueWidget.h"

void UPRCompanionDialogueComponent::InitializeForSubsystem(UPRCompanionDialogueSubsystem* InSubsystem)
{
	if (DialogueSubsystem.Get() == InSubsystem)
	{
		return;
	}
	if (UPRCompanionDialogueSubsystem* Previous = DialogueSubsystem.Get(); Previous && StateChangedHandle.IsValid())
	{
		Previous->OnDialogueStateChanged().Remove(StateChangedHandle);
	}
	DialogueSubsystem = InSubsystem;
	StateChangedHandle.Reset();
	if (InSubsystem)
	{
		StateChangedHandle = InSubsystem->OnDialogueStateChanged().AddUObject(this, &UPRCompanionDialogueComponent::HandleDialogueStateChanged);
		HandleDialogueStateChanged(InSubsystem->GetRuntimeState());
	}
}

void UPRCompanionDialogueComponent::RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn)
{
	ClearBinding();
	if (!InPlayerPawn)
	{
		return;
	}
	BoundPlayerPawn = InPlayerPawn;
	SemanticInputHandle = InPlayerPawn->OnSemanticInputEvent().AddUObject(this, &UPRCompanionDialogueComponent::HandleSemanticInput);
}

void UPRCompanionDialogueComponent::SetDialogueWidgetClass(TSubclassOf<UPRCompanionDialogueWidget> InWidgetClass)
{
	DialogueWidgetClass = InWidgetClass;
}

void UPRCompanionDialogueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBinding();
	ClearWidget();
	if (UPRCompanionDialogueSubsystem* Subsystem = DialogueSubsystem.Get(); Subsystem && StateChangedHandle.IsValid())
	{
		Subsystem->OnDialogueStateChanged().Remove(StateChangedHandle);
	}
	StateChangedHandle.Reset();
	DialogueSubsystem.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRCompanionDialogueComponent::HandleSemanticInput(const FPRSemanticInputEvent& Event)
{
	if (!Event.bPressed)
	{
		return;
	}
	if (UPRCompanionDialogueSubsystem* Subsystem = DialogueSubsystem.Get())
	{
		if (Event.InputTag.MatchesTagExact(UPRTagLibrary::GetInputInteractTag()))
		{
			Subsystem->SubmitChoice(false);
		}
		else if (Event.InputTag.MatchesTagExact(UPRTagLibrary::GetInputExecuteTag()))
		{
			Subsystem->SubmitChoice(true);
		}
	}
}

void UPRCompanionDialogueComponent::HandleDialogueStateChanged(const FPRDialogueRuntimeState& State)
{
	if (State.PresentationState == EPRDialoguePresentationState::Idle)
	{
		ClearWidget();
		return;
	}
	if (!DialogueWidget.IsValid() && DialogueWidgetClass)
	{
		DialogueWidget = CreateWidget<UPRCompanionDialogueWidget>(GetWorld(), DialogueWidgetClass);
		if (UPRCompanionDialogueWidget* Widget = DialogueWidget.Get())
		{
			Widget->AddToViewport(25);
		}
	}
	if (UPRCompanionDialogueWidget* Widget = DialogueWidget.Get())
	{
		Widget->ApplyRuntimeState(State);
	}
}

void UPRCompanionDialogueComponent::ClearBinding()
{
	if (APRPlayerCharacter* Pawn = BoundPlayerPawn.Get(); Pawn && SemanticInputHandle.IsValid())
	{
		Pawn->OnSemanticInputEvent().Remove(SemanticInputHandle);
	}
	SemanticInputHandle.Reset();
	BoundPlayerPawn.Reset();
}

void UPRCompanionDialogueComponent::ClearWidget()
{
	if (UPRCompanionDialogueWidget* Widget = DialogueWidget.Get())
	{
		Widget->RemoveFromParent();
	}
	DialogueWidget.Reset();
}
