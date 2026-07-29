// Copyright ProjectR. All Rights Reserved.

#include "Roguelike/PRChapterRuleDataAsset.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"

bool UPRChapterRuleDataAsset::IsRuleDefinitionValid() const
{
    return ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId()
        && UPRChapterContentRegistryDataAsset::GetAllocatorDirectiveIds().Contains(DirectiveId);
}

FPrimaryAssetId UPRChapterRuleDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("ProjectRChapterRule"), GetFName());
}
