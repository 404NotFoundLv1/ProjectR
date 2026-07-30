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

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPacifierChapterId()
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, TEXT("DA_Chapter_Pacifier"));
}

FName UPRChapterContentRegistryDataAsset::GetPacifierContentId() { return TEXT("Pacifier"); }
FName UPRChapterContentRegistryDataAsset::GetPacifierBossId() { return TEXT("Pacifier"); }
FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPacifierBossPrototypeId() { return FPrimaryAssetId(TEXT("ProjectREnemy"), TEXT("DA_Boss_Pacifier")); }
FName UPRChapterContentRegistryDataAsset::GetPacifierProofId() { return TEXT("HumanAnomalyProof.Pacifier"); }

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPacifierRoomRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Pacifier"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPacifierEnemyRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Pacifier"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetPacifierFinalRoomId()
{
	return FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Pacifier_Boss_Pacifier"));
}

const TArray<FName>& UPRChapterContentRegistryDataAsset::GetPacifierDirectiveIds()
{
	static const TArray<FName> Directives = {
		TEXT("Pacifier.ComfortProjection"),
		TEXT("Pacifier.EmotionalDampening"),
		TEXT("Pacifier.IllusionVeil"),
		TEXT("Pacifier.RiskSuppression"),
		TEXT("Pacifier.SafetyIncentive")};
	return Directives;
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAuditorChapterId()
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, TEXT("DA_Chapter_Auditor"));
}

FName UPRChapterContentRegistryDataAsset::GetAuditorContentId() { return TEXT("Auditor"); }
FName UPRChapterContentRegistryDataAsset::GetAuditorBossId() { return TEXT("Auditor"); }
FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAuditorBossPrototypeId() { return FPrimaryAssetId(TEXT("ProjectREnemy"), TEXT("DA_Boss_AuditorChapter")); }
FName UPRChapterContentRegistryDataAsset::GetAuditorProofId() { return TEXT("HumanAnomalyProof.Auditor"); }

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAuditorRoomRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Auditor"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAuditorEnemyRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Auditor"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetAuditorFinalRoomId()
{
	return FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Auditor_Boss_Auditor"));
}

const TArray<FName>& UPRChapterContentRegistryDataAsset::GetAuditorDirectiveIds()
{
	static const TArray<FName> Directives = {
		TEXT("Auditor.HabitReplication"),
		TEXT("Auditor.DistanceAudit"),
		TEXT("Auditor.CooperationAudit"),
		TEXT("Auditor.PredictionEscalation"),
		TEXT("Auditor.VerdictEscalation")};
	return Directives;
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetHeadmindChapterId()
{
	return FPrimaryAssetId(PRChapterContent::ChapterType, TEXT("DA_Chapter_Headmind"));
}

FName UPRChapterContentRegistryDataAsset::GetHeadmindContentId() { return TEXT("Headmind"); }
FName UPRChapterContentRegistryDataAsset::GetHeadmindBossId() { return TEXT("HeadmindProjection"); }
FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetHeadmindBossPrototypeId() { return FPrimaryAssetId(TEXT("ProjectREnemy"), TEXT("DA_Boss_HeadmindProjection")); }
FName UPRChapterContentRegistryDataAsset::GetHeadmindProofId() { return TEXT("HumanAnomalyProof.Headmind"); }

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetHeadmindRoomRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Headmind"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetHeadmindEnemyRegistryId()
{
	return FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Headmind"));
}

FPrimaryAssetId UPRChapterContentRegistryDataAsset::GetHeadmindFinalRoomId()
{
	return FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Headmind_Boss_HeadmindProjection"));
}

const TArray<FName>& UPRChapterContentRegistryDataAsset::GetHeadmindDirectiveIds()
{
	static const TArray<FName> Directives = {
		TEXT("Headmind.ObediencePrediction"),
		TEXT("Headmind.RepetitionOptimality"),
		TEXT("Headmind.IsolationCooperation"),
		TEXT("Headmind.SurvivalRisk"),
		TEXT("Headmind.ResourceDistance")};
	return Directives;
}

FName UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(const FName ContentId, const int32 Seed)
{
	const TArray<FName>* Directives = nullptr;
	if (ContentId == GetAllocatorContentId()) Directives = &GetAllocatorDirectiveIds();
	else if (ContentId == GetWardenContentId()) Directives = &GetWardenDirectiveIds();
	else if (ContentId == GetPacifierContentId()) Directives = &GetPacifierDirectiveIds();
	else if (ContentId == GetAuditorContentId()) Directives = &GetAuditorDirectiveIds();
	else if (ContentId == GetHeadmindContentId()) Directives = &GetHeadmindDirectiveIds();
	if (!Directives || Directives->IsEmpty()) return NAME_None;
	return (*Directives)[static_cast<uint32>(Seed) % Directives->Num()];
}
