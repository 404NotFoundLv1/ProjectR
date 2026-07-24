// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceComponent.h"

#include "Characters/PRPlayerCharacter.h"
#include "Core/PRTagLibrary.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Input/PRInputTypes.h"
#include "UI/PRDivergenceCacheWidget.h"

void UPRDivergenceComponent::InitializeForSubsystem(UPRDivergenceSubsystem* InSubsystem)
{
	if (DivergenceSubsystem.Get() == InSubsystem)
	{
		return;
	}
	if (UPRDivergenceSubsystem* Previous = DivergenceSubsystem.Get(); Previous && StateChangedHandle.IsValid())
	{
		Previous->OnDivergenceStateChanged().Remove(StateChangedHandle);
	}
	DivergenceSubsystem = InSubsystem;
	StateChangedHandle.Reset();
	if (InSubsystem)
	{
		StateChangedHandle = InSubsystem->OnDivergenceStateChanged().AddUObject(this, &UPRDivergenceComponent::HandleStateChanged);
		HandleStateChanged(InSubsystem->GetRuntimeState());
	}
}

void UPRDivergenceComponent::RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn)
{
	ClearBinding();
	if (!InPlayerPawn)
	{
		return;
	}
	BoundPlayerPawn = InPlayerPawn;
	SemanticInputHandle = InPlayerPawn->OnSemanticInputEvent().AddUObject(this, &UPRDivergenceComponent::HandleSemanticInput);
}

void UPRDivergenceComponent::SetWidgetClass(TSubclassOf<UPRDivergenceCacheWidget> InWidgetClass)
{
	WidgetClass = InWidgetClass;
}

void UPRDivergenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBinding();
	ClearWidget();
	if (UPRDivergenceSubsystem* Subsystem = DivergenceSubsystem.Get(); Subsystem && StateChangedHandle.IsValid())
	{
		Subsystem->OnDivergenceStateChanged().Remove(StateChangedHandle);
	}
	StateChangedHandle.Reset();
	DivergenceSubsystem.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRDivergenceComponent::HandleSemanticInput(const FPRSemanticInputEvent& Event)
{
	if (!Event.bPressed)
	{
		return;
	}
	if (UPRDivergenceSubsystem* Subsystem = DivergenceSubsystem.Get())
	{
		if (Event.InputTag.MatchesTagExact(UPRTagLibrary::GetInputInteractTag()))
		{
			Subsystem->SubmitChoice(EPRDivergenceChoice::Rescue);
		}
		else if (Event.InputTag.MatchesTagExact(UPRTagLibrary::GetInputQTERejectTag()))
		{
			Subsystem->SubmitChoice(EPRDivergenceChoice::Leave);
		}
		else if (Event.InputTag.MatchesTagExact(UPRTagLibrary::GetInputExecuteTag()))
		{
			Subsystem->SubmitChoice(EPRDivergenceChoice::FaceChallenge);
		}
	}
}

void UPRDivergenceComponent::HandleStateChanged(const FPRDivergenceRuntimeState& State)
{
	if (State.State != EPRDivergenceState::AwaitingChoice)
	{
		ClearWidget();
		return;
	}
	if (!Widget.IsValid() && WidgetClass)
	{
		Widget = CreateWidget<UPRDivergenceCacheWidget>(GetWorld(), WidgetClass);
		if (UPRDivergenceCacheWidget* CacheWidget = Widget.Get())
		{
			CacheWidget->AddToViewport(40);
		}
	}
	if (UPRDivergenceCacheWidget* CacheWidget = Widget.Get())
	{
		CacheWidget->ApplyRuntimeState(State);
	}
}

void UPRDivergenceComponent::ClearBinding()
{
	if (APRPlayerCharacter* Pawn = BoundPlayerPawn.Get(); Pawn && SemanticInputHandle.IsValid())
	{
		Pawn->OnSemanticInputEvent().Remove(SemanticInputHandle);
	}
	SemanticInputHandle.Reset();
	BoundPlayerPawn.Reset();
}

void UPRDivergenceComponent::ClearWidget()
{
	if (UPRDivergenceCacheWidget* CacheWidget = Widget.Get())
	{
		CacheWidget->RemoveFromParent();
	}
	Widget.Reset();
}
