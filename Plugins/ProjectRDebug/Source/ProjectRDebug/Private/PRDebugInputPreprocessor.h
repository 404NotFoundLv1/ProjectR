// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "UObject/WeakObjectPtr.h"

class UPRDebugSubsystem;

/** Focus-scoped F1 handler that never changes formal Enhanced Input mappings. */
class FPRDebugInputPreprocessor final : public IInputProcessor
{
public:
	explicit FPRDebugInputPreprocessor(UPRDebugSubsystem* InOwner);

	virtual void Tick(
		float DeltaTime,
		FSlateApplication& SlateApp,
		TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(
		FSlateApplication& SlateApp,
		const FKeyEvent& InKeyEvent) override;

private:
	TWeakObjectPtr<UPRDebugSubsystem> Owner;
};
