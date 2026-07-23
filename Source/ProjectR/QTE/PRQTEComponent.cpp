// Copyright Epic Games, Inc. All Rights Reserved.

#include "QTE/PRQTEComponent.h"

#include "Characters/PRPlayerCharacter.h"
#include "Input/PRInputTypes.h"
#include "QTE/PRQTESubsystem.h"
#include "UI/PRQTEPromptWidget.h"
#include "GameFramework/PlayerController.h"

void UPRQTEComponent::InitializeForSubsystem(UPRQTESubsystem* InSubsystem)
{
	QTESubsystem = InSubsystem;
	if (InSubsystem)
	{
		QTEStateHandle = InSubsystem->OnQTEStateChanged().AddUObject(this, &UPRQTEComponent::HandleQTEStateChanged);
	}
}

void UPRQTEComponent::RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn)
{
	if (BoundPlayerPawn.Get() == InPlayerPawn)
	{
		return;
	}
	ClearBinding();
	if (!IsValid(InPlayerPawn))
	{
		return;
	}
	BoundPlayerPawn = InPlayerPawn;
	SemanticInputHandle = InPlayerPawn->OnSemanticInputEvent().AddUObject(this, &UPRQTEComponent::HandleSemanticInput);
}

void UPRQTEComponent::SetPromptWidgetClass(TSubclassOf<UPRQTEPromptWidget> InPromptWidgetClass)
{
	PromptWidgetClass = InPromptWidgetClass;
}

void UPRQTEComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBinding();
	ClearPrompt();
	if (UPRQTESubsystem* Subsystem = QTESubsystem.Get(); Subsystem && QTEStateHandle.IsValid())
	{
		Subsystem->OnQTEStateChanged().Remove(QTEStateHandle);
	}
	QTEStateHandle.Reset();
	QTESubsystem.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRQTEComponent::HandleQTEStateChanged(const FPRQTERuntimeState& State)
{
	if (State.State == EPRQTEState::Active || State.State == EPRQTEState::Resolving)
	{
		UPRQTEPromptWidget* Widget = PromptWidget.Get();
		if (!Widget && PromptWidgetClass)
		{
			Widget = CreateWidget<UPRQTEPromptWidget>(Cast<APlayerController>(GetOwner()), PromptWidgetClass);
			if (Widget)
			{
				Widget->AddToViewport();
				PromptWidget = Widget;
			}
		}
		if (Widget) Widget->ApplyRuntimeState(State);
		return;
	}
	ClearPrompt();
}

void UPRQTEComponent::HandleSemanticInput(const FPRSemanticInputEvent& Event)
{
	if (Event.bPressed)
	{
		if (UPRQTESubsystem* Subsystem = QTESubsystem.Get())
		{
			Subsystem->SubmitSemanticInput(Event.InputTag, Event.WorldTimeSeconds);
		}
	}
}

void UPRQTEComponent::ClearBinding()
{
	if (APRPlayerCharacter* PlayerPawn = BoundPlayerPawn.Get(); PlayerPawn && SemanticInputHandle.IsValid())
	{
		PlayerPawn->OnSemanticInputEvent().Remove(SemanticInputHandle);
	}
	SemanticInputHandle.Reset();
	BoundPlayerPawn.Reset();
}

void UPRQTEComponent::ClearPrompt()
{
	if (UPRQTEPromptWidget* Widget = PromptWidget.Get())
	{
		Widget->RemoveFromParent();
	}
	PromptWidget.Reset();
}
