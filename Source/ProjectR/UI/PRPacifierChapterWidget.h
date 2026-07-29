// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Chapters/PRChapterTypes.h"

#include "PRPacifierChapterWidget.generated.h"

UCLASS(Abstract)
class PROJECTR_API UPRPacifierChapterWidget : public UUserWidget
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
