// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDebugInputPreprocessor.h"

#include "InputCoreTypes.h"
#include "PRDebugSubsystem.h"

FPRDebugInputPreprocessor::FPRDebugInputPreprocessor(UPRDebugSubsystem* InOwner)
	: Owner(InOwner)
{
}

void FPRDebugInputPreprocessor::Tick(
	float DeltaTime,
	FSlateApplication& SlateApp,
	TSharedRef<ICursor> Cursor)
{
}

bool FPRDebugInputPreprocessor::HandleKeyDownEvent(
	FSlateApplication& SlateApp,
	const FKeyEvent& InKeyEvent)
{
	UPRDebugSubsystem* Subsystem = Owner.Get();
	if (Subsystem == nullptr
		|| InKeyEvent.IsRepeat()
		|| InKeyEvent.GetKey() != EKeys::F1
		|| !Subsystem->CanTogglePanelFromInput())
	{
		return false;
	}

	return Subsystem->TogglePanel();
}
