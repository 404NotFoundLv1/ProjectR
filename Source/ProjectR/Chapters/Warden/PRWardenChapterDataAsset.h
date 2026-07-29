// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterContentRegistryDataAsset.h"

#include "PRWardenChapterDataAsset.generated.h"

class UPRWardenChapterWidget;

UCLASS(BlueprintType)
class PROJECTR_API UPRWardenChapterDataAsset final : public UPRChapterContentRegistryDataAsset
{
	GENERATED_BODY()

public:
	bool IsWardenDefinitionValid() const;
	FPRWardenStoryProjection BuildStoryProjection(bool bAxiomPrimary, bool bLowProbabilityCompleted, bool bImperfectOptimumCompleted, bool bDependenciesAvailable) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") TSoftClassPtr<UPRWardenChapterWidget> OverlayWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FName BaseStoryBeatId = TEXT("Story.Warden.Axiom.Base");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FText BaseStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FName LowProbabilityStoryBeatId = TEXT("Story.Warden.Axiom.LowProbability");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FText LowProbabilityStoryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FName ImperfectOptimumStoryBeatId = TEXT("Story.Warden.Axiom.ImperfectOptimum");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Warden") FText ImperfectOptimumStoryText;
};
