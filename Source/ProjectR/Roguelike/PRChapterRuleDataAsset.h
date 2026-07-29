// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRChapterRuleDataAsset.generated.h"

/**
 * A Chapter-local, declarative instruction.  These assets are deliberately
 * independent from the Director rule pipeline: the owning Chapter subsystem
 * interprets only the fixed Allocator whitelist.
 */
UCLASS(BlueprintType)
class PROJECTR_API UPRChapterRuleDataAsset final : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chapter")
    FName ContentId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chapter")
    FName DirectiveId;

    /** Returns true only for one of the fixed, Allocator chapter directives. */
    bool IsRuleDefinitionValid() const;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
