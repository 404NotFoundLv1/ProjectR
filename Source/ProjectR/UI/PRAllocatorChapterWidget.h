// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Chapters/PRChapterTypes.h"

#include "PRAllocatorChapterWidget.generated.h"

class UPRChapterSubsystem;

/** Read-only transient overlay for the active Allocator chapter snapshot. */
UCLASS(Abstract)
class PROJECTR_API UPRAllocatorChapterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	const FPRChapterSnapshot& GetChapterSnapshot() const;

private:
	void HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot);
	FPRChapterSnapshot ChapterSnapshot;
	TWeakObjectPtr<UPRChapterSubsystem> ChapterSubsystem;
	FDelegateHandle StateChangedHandle;
};
