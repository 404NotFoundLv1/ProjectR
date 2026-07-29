// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterContentRegistryDataAsset.h"

#include "PRPacifierChapterDataAsset.generated.h"

class UPRPacifierChapterWidget;

UCLASS(BlueprintType)
class PROJECTR_API UPRPacifierChapterDataAsset final : public UPRChapterContentRegistryDataAsset
{
	GENERATED_BODY()

public:
	bool IsPacifierDefinitionValid() const;
	FPRPacifierStoryProjection BuildStoryProjection(bool bKindlePrimary, bool bNoRetreatCompleted, bool bLearnToRetreatCompleted, bool bDependenciesAvailable) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") TSoftClassPtr<UPRPacifierChapterWidget> OverlayWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FName BaseStoryBeatId = TEXT("Story.Pacifier.Kindle.Base");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FText BaseStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FName NoRetreatStoryBeatId = TEXT("Story.Pacifier.Kindle.NoRetreatLine");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FText NoRetreatStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FName LearnToRetreatStoryBeatId = TEXT("Story.Pacifier.Kindle.LearnToRetreat");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Pacifier") FText LearnToRetreatStoryText;
};
