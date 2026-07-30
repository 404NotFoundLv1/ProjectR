// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

#include "PRTripleResonanceOverlayWidget.generated.h"

/** Presentation-only overlay for the fixed three-step sequence; it owns no gameplay state. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRTripleResonanceOverlayWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="ProjectR|TripleResonance") FPRTripleResonanceSnapshot GetTripleResonanceSnapshot() const;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshPresentation(const FPRTripleResonanceSnapshot& Snapshot);
	class UTextBlock* PresentationText = nullptr;
	FDelegateHandle StateChangedHandle;
};
