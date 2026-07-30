// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

#include "PRTripleResonanceLegacyWidget.generated.h"

/** Read-only RealityHub legacy projection; wording and visual layout are authored by its Widget Blueprint. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRTripleResonanceLegacyWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="ProjectR|TripleResonance") FPRTripleResonanceLegacySnapshot GetTripleResonanceLegacySnapshot() const;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshPresentation(const FPRTripleResonanceOperationEvent& Event);
	class UTextBlock* PresentationText = nullptr;
	FDelegateHandle OperationHandle;
};
