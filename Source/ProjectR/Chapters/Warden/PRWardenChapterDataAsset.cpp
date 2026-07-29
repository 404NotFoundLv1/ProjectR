// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Warden/PRWardenChapterDataAsset.h"

bool UPRWardenChapterDataAsset::IsWardenDefinitionValid() const
{
	return IsDefinitionValid()
		&& ChapterId == GetWardenChapterId()
		&& ContentId == GetWardenContentId()
		&& RoomContentRegistryId == GetWardenRoomRegistryId()
		&& EnemyContentRegistryId == GetWardenEnemyRegistryId()
		&& BossId == GetWardenBossId()
		&& ProofId == GetWardenProofId()
		&& !OverlayWidgetClass.IsNull()
		&& BaseStoryBeatId == TEXT("Story.Warden.Axiom.Base")
		&& LowProbabilityStoryBeatId == TEXT("Story.Warden.Axiom.LowProbability")
		&& ImperfectOptimumStoryBeatId == TEXT("Story.Warden.Axiom.ImperfectOptimum")
		&& !BaseStoryText.IsEmpty() && !LowProbabilityStoryText.IsEmpty() && !ImperfectOptimumStoryText.IsEmpty();
}

FPRWardenStoryProjection UPRWardenChapterDataAsset::BuildStoryProjection(const bool bAxiomPrimary, const bool bLowProbabilityCompleted, const bool bImperfectOptimumCompleted, const bool bDependenciesAvailable) const
{
	FPRWardenStoryProjection Projection;
	if (!bDependenciesAvailable || !bAxiomPrimary)
	{
		Projection.FallbackReason = TEXT("Warden.StoryUnavailable");
		return Projection;
	}
	Projection.bAvailable = true;
	if (bImperfectOptimumCompleted) { Projection.StoryBeatId = ImperfectOptimumStoryBeatId; Projection.Text = ImperfectOptimumStoryText; }
	else if (bLowProbabilityCompleted) { Projection.StoryBeatId = LowProbabilityStoryBeatId; Projection.Text = LowProbabilityStoryText; }
	else { Projection.StoryBeatId = BaseStoryBeatId; Projection.Text = BaseStoryText; }
	return Projection;
}
