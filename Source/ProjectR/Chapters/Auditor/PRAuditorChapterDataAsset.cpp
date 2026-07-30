// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Auditor/PRAuditorChapterDataAsset.h"

bool UPRAuditorChapterDataAsset::IsAuditorDefinitionValid() const
{
	return IsDefinitionValid()
		&& ChapterId == GetAuditorChapterId()
		&& ContentId == GetAuditorContentId()
		&& RoomContentRegistryId == GetAuditorRoomRegistryId()
		&& EnemyContentRegistryId == GetAuditorEnemyRegistryId()
		&& BossId == GetAuditorBossId()
		&& ProofId == GetAuditorProofId()
		&& !OverlayWidgetClass.IsNull()
		&& BaseStoryBeatId == TEXT("Story.Auditor.Null.Base")
		&& GarbageCollectionStoryBeatId == TEXT("Story.Auditor.Null.GarbageCollection")
		&& RememberMeStoryBeatId == TEXT("Story.Auditor.Null.RememberMe")
		&& !BaseStoryText.IsEmpty() && !GarbageCollectionStoryText.IsEmpty() && !RememberMeStoryText.IsEmpty();
}

FPRAuditorStoryProjection UPRAuditorChapterDataAsset::BuildStoryProjection(
	const bool bNullPrimary,
	const bool bGarbageCollectionCompleted,
	const bool bRememberMeCompleted,
	const bool bDependenciesAvailable) const
{
	FPRAuditorStoryProjection Projection;
	if (!bDependenciesAvailable || !bNullPrimary)
	{
		Projection.FallbackReason = TEXT("Auditor.StoryUnavailable");
		return Projection;
	}
	Projection.bAvailable = true;
	if (bRememberMeCompleted)
	{
		Projection.StoryBeatId = RememberMeStoryBeatId;
		Projection.Text = RememberMeStoryText;
	}
	else if (bGarbageCollectionCompleted)
	{
		Projection.StoryBeatId = GarbageCollectionStoryBeatId;
		Projection.Text = GarbageCollectionStoryText;
	}
	else
	{
		Projection.StoryBeatId = BaseStoryBeatId;
		Projection.Text = BaseStoryText;
	}
	return Projection;
}
