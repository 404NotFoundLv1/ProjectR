// Copyright ProjectR. All Rights Reserved.

#include "UI/PRAllocatorChapterWidget.h"

#include "Chapters/PRChapterSubsystem.h"
#include "Engine/GameInstance.h"

void UPRAllocatorChapterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRChapterSubsystem* Subsystem = GameInstance->GetSubsystem<UPRChapterSubsystem>())
		{
			ChapterSubsystem = Subsystem;
			Subsystem->GetSnapshot(ChapterSnapshot);
			StateChangedHandle = Subsystem->OnStateChanged().AddUObject(this, &UPRAllocatorChapterWidget::HandleChapterStateChanged);
		}
	}
}

void UPRAllocatorChapterWidget::NativeDestruct()
{
	if (UPRChapterSubsystem* Subsystem = ChapterSubsystem.Get()) Subsystem->OnStateChanged().Remove(StateChangedHandle);
	StateChangedHandle.Reset();
	ChapterSubsystem.Reset();
	ChapterSnapshot = FPRChapterSnapshot();
	Super::NativeDestruct();
}

const FPRChapterSnapshot& UPRAllocatorChapterWidget::GetChapterSnapshot() const { return ChapterSnapshot; }
void UPRAllocatorChapterWidget::HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot) { ChapterSnapshot = InSnapshot; }
