// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Headmind/PRHeadmindTypes.h"

#include "PRHeadmindChapterDataAsset.generated.h"

class UPRHeadmindChapterWidget;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRHeadmindEndingParagraph
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindRelationshipBand RelationshipBand = EPRHeadmindRelationshipBand::Distant;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindCounterproofBand CounterproofBand = EPRHeadmindCounterproofBand::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindObedienceBand ObedienceBand = EPRHeadmindObedienceBand::Unobserved;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") FName ParagraphId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") FText Text;
};

UCLASS(BlueprintType)
class PROJECTR_API UPRHeadmindChapterDataAsset final : public UPRChapterContentRegistryDataAsset
{
	GENERATED_BODY()
public:
	bool IsHeadmindDefinitionValid() const;
	bool ResolveEndingParagraph(const FPRHeadmindEndingInputSnapshot& Input, FPRHeadmindEndingResult& OutResult, FText& OutText) const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") TSoftClassPtr<UPRHeadmindChapterWidget> OverlayWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") FName NullStoryBeatId = TEXT("Story.Headmind.Null.MemoryCorridor");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") FText NullStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") TArray<FPRHeadmindEndingParagraph> EndingParagraphs;
};
