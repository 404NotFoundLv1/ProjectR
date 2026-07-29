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
FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAllocatorBossPrototypeId() { return FPrimaryAssetId(TEXT("ProjectREnemy"), TEXT("DA_Boss_Allocator")); }
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
	return GetDirectiveForContentAndSeed(GetAllocatorContentId(), Seed);
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetWardenChapterId()
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, TEXT("DA_Chapter_Warden"));
}

FName UPRChapterContentRegistryDataAsset::GetWardenContentId() { return TEXT("Warden"); }
FName UPRChapterContentRegistryDataAsset::GetWardenBossId() { return TEXT("Warden"); }
FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetWardenBossPrototypeId() { return FPrimaryAssetId(TEXT("ProjectREnemy"), TEXT("DA_Boss_Warden")); }
FName UPRChapterContentRegistryDataAsset::GetWardenProofId() { return TEXT("HumanAnomalyProof.Warden"); }

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetWardenRoomRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Warden"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Warden"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetWardenFinalRoomId()
{
	return FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Warden_Boss_Warden"));
}

const TArray<FName>& UPRChapterContentRegistryDataAsset::GetWardenDirectiveIds()
{
	static const TArray<FName> Directives = {
		TEXT("Warden.PredictivePatrol"),
		TEXT("Warden.RouteForewarning"),
		TEXT("Warden.PreemptiveLockdown"),
		TEXT("Warden.RiskMarking"),
		TEXT("Warden.TrapEscalation")};
	return Directives;
}

FName UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(const FName ContentId, const int32 Seed)
{
	const TArray<FName>* Directives = nullptr;
	if (ContentId == GetAllocatorContentId()) Directives = &GetAllocatorDirectiveIds();
	else if (ContentId == GetWardenContentId()) Directives = &GetWardenDirectiveIds();
	if (!Directives || Directives->IsEmpty()) return NAME_None;
	return (*Directives)[static_cast<uint32>(Seed) % Directives->Num()];
}
