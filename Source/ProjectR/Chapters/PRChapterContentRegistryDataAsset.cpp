// Copyright Epic Games, Inc. All Rights Reserved.

#include "Chapters/PRChapterContentRegistryDataAsset.h"

namespace PRChapterContent
{
const FPrimaryAssetType ChapterType(TEXT("ProjectRChapter"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, GetFName());
}

bool UPRChapterContentRegistryDataAsset::IsDefinitionValid() const
{
	if (!ChapterId.IsValid() || ChapterId.PrimaryAssetType != PRChapterContent::ChapterType || ChapterId != GetPrimaryAssetId()) return false;
	if (ContentId.IsNone() || BossId.IsNone() || ProofId.IsNone()) return false;
	return RoomContentRegistryId.IsValid() && EnemyContentRegistryId.IsValid();
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAllocatorChapterId()
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, TEXT("DA_Chapter_Allocator"));
}

FName UPRChapterContentRegistryDataAsset::GetAllocatorContentId() { return TEXT("Allocator"); }
FName UPRChapterContentRegistryDataAsset::GetAllocatorBossId() { return TEXT("Allocator"); }
FName UPRChapterContentRegistryDataAsset::GetAllocatorProofId() { return TEXT("HumanAnomalyProof.Allocator"); }

const TArray<FName>& UPRChapterContentRegistryDataAsset::GetAllocatorDirectiveIds()
{
	static const TArray<FName> Directives = {
		TEXT("Allocator.ResourceLock"),
		TEXT("Allocator.RewardWithholding"),
		TEXT("Allocator.PriceAudit"),
		TEXT("Allocator.ScarcityMarkup"),
		TEXT("Allocator.EqualizationQuota")};
	return Directives;
}

FName UPRChapterContentRegistryDataAsset::GetDirectiveForSeed(const int32 Seed)
{
	const TArray<FName>& Directives = GetAllocatorDirectiveIds();
	return Directives[static_cast<uint32>(Seed) % Directives.Num()];
}
