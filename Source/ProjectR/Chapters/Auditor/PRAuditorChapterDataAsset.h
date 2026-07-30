// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterContentRegistryDataAsset.h"

#include "PRAuditorChapterDataAsset.generated.h"

class UPRAuditorChapterWidget;

/** Fixed fourth chapter definition; it admits no caller-supplied content or story identifiers. */
UCLASS(BlueprintType)
class PROJECTR_API UPRAuditorChapterDataAsset final : public UPRChapterContentRegistryDataAsset
{
	GENERATED_BODY()

public:
	bool IsAuditorDefinitionValid() const;
	FPRAuditorStoryProjection BuildStoryProjection(
		bool bNullPrimary,
		bool bGarbageCollectionCompleted,
		bool bRememberMeCompleted,
		bool bDependenciesAvailable) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") TSoftClassPtr<UPRAuditorChapterWidget> OverlayWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FName BaseStoryBeatId = TEXT("Story.Auditor.Null.Base");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FText BaseStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FName GarbageCollectionStoryBeatId = TEXT("Story.Auditor.Null.GarbageCollection");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FText GarbageCollectionStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FName RememberMeStoryBeatId = TEXT("Story.Auditor.Null.RememberMe");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") FText RememberMeStoryText;
};
