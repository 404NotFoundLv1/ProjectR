// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Headmind/PRHeadmindChapterDataAsset.h"

bool UPRHeadmindChapterDataAsset::IsHeadmindDefinitionValid() const
{
	if (!IsDefinitionValid() || ChapterId != GetHeadmindChapterId() || ContentId != GetHeadmindContentId()
		|| RoomContentRegistryId != GetHeadmindRoomRegistryId() || EnemyContentRegistryId != GetHeadmindEnemyRegistryId()
		|| BossId != GetHeadmindBossId() || ProofId != GetHeadmindProofId() || OverlayWidgetClass.IsNull()
		|| NullStoryBeatId != TEXT("Story.Headmind.Null.MemoryCorridor") || NullStoryText.IsEmpty()
		|| EndingParagraphs.Num() != 27) return false;
	TSet<FString> Keys;
	for (const FPRHeadmindEndingParagraph& Paragraph : EndingParagraphs)
	{
		const FString Key = FString::Printf(TEXT("%d.%d.%d"), static_cast<int32>(Paragraph.RelationshipBand), static_cast<int32>(Paragraph.CounterproofBand), static_cast<int32>(Paragraph.ObedienceBand));
		if (Paragraph.ParagraphId.IsNone() || Paragraph.Text.IsEmpty() || Keys.Contains(Key)) return false;
		Keys.Add(Key);
	}
	return Keys.Num() == 27;
}

bool UPRHeadmindChapterDataAsset::ResolveEndingParagraph(const FPRHeadmindEndingInputSnapshot& Input, FPRHeadmindEndingResult& OutResult, FText& OutText) const
{
	OutResult = FPRHeadmindEndingResult(); OutText = FText();
	if (!Input.bAvailable) return false;
	const FPRHeadmindEndingParagraph* Paragraph = EndingParagraphs.FindByPredicate([&Input](const FPRHeadmindEndingParagraph& Candidate)
	{
		return Candidate.RelationshipBand == Input.RelationshipBand && Candidate.CounterproofBand == Input.CounterproofBand && Candidate.ObedienceBand == Input.ObedienceBand;
	});
	if (!Paragraph) return false;
	OutResult.RelationshipBand = Input.RelationshipBand;
	OutResult.CounterproofBand = Input.CounterproofBand;
	OutResult.ObedienceBand = Input.ObedienceBand;
	OutResult.ParagraphId = Paragraph->ParagraphId;
	OutResult.bAvailable = true;
	OutText = Paragraph->Text;
	return true;
}
