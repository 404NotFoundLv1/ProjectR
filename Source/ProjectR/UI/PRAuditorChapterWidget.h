// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Chapters/PRChapterTypes.h"

#include "PRAuditorChapterWidget.generated.h"

/** Presentation-only overlay for the fixed Auditor chapter snapshot. */
UCLASS(Abstract)
class PROJECTR_API UPRAuditorChapterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintPure, Category="ProjectR|Chapter") const FPRChapterSnapshot& GetChapterSnapshot() const;

private:
	void HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot);
	void RefreshPresentation();
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UTextBlock> StoryText;
	FPRChapterSnapshot Snapshot;
	TWeakObjectPtr<class UPRChapterSubsystem> ChapterSubsystem;
	FDelegateHandle StateChangedHandle;
};
