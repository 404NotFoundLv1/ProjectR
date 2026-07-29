// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Pacifier/PRPacifierChapterDataAsset.h"

bool UPRPacifierChapterDataAsset::IsPacifierDefinitionValid() const
{
	return IsDefinitionValid()
		&& ChapterId == GetPacifierChapterId()
		&& ContentId == GetPacifierContentId()
		&& RoomContentRegistryId == GetPacifierRoomRegistryId()
		&& EnemyContentRegistryId == GetPacifierEnemyRegistryId()
		&& BossId == GetPacifierBossId()
		&& ProofId == GetPacifierProofId()
		&& !OverlayWidgetClass.IsNull()
		&& BaseStoryBeatId == TEXT("Story.Pacifier.Kindle.Base")
		&& NoRetreatStoryBeatId == TEXT("Story.Pacifier.Kindle.NoRetreatLine")
		&& LearnToRetreatStoryBeatId == TEXT("Story.Pacifier.Kindle.LearnToRetreat")
		&& !BaseStoryText.IsEmpty() && !NoRetreatStoryText.IsEmpty() && !LearnToRetreatStoryText.IsEmpty();
}

FPRPacifierStoryProjection UPRPacifierChapterDataAsset::BuildStoryProjection(
	const bool bKindlePrimary,
	const bool bNoRetreatCompleted,
	const bool bLearnToRetreatCompleted,
	const bool bDependenciesAvailable) const
{
	FPRPacifierStoryProjection Projection;
	if (!bDependenciesAvailable || !bKindlePrimary)
	{
		Projection.FallbackReason = TEXT("Pacifier.StoryUnavailable");
		return Projection;
	}
	Projection.bAvailable = true;
	if (bLearnToRetreatCompleted)
	{
		Projection.StoryBeatId = LearnToRetreatStoryBeatId;
		Projection.Text = LearnToRetreatStoryText;
	}
	else if (bNoRetreatCompleted)
	{
		Projection.StoryBeatId = NoRetreatStoryBeatId;
		Projection.Text = NoRetreatStoryText;
	}
	else
	{
		Projection.StoryBeatId = BaseStoryBeatId;
		Projection.Text = BaseStoryText;
	}
	return Projection;
}
