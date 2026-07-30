// Copyright ProjectR. All Rights Reserved.

#include "Roguelike/PRChapterRuleDataAsset.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"

bool UPRChapterRuleDataAsset::IsRuleDefinitionValid() const
{
	if (ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId())
	{
		return UPRChapterContentRegistryDataAsset::GetAllocatorDirectiveIds().Contains(DirectiveId)
			&& !RequiredDirectorRuleId.IsValid() && PreferredRoomIds.IsEmpty();
	}
	const bool bWarden = ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId();
	const bool bPacifier = ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId();
	const bool bAuditor = ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId();
	if ((!bWarden && !bPacifier && !bAuditor)
		|| !(bWarden ? UPRChapterContentRegistryDataAsset::GetWardenDirectiveIds()
			: bPacifier ? UPRChapterContentRegistryDataAsset::GetPacifierDirectiveIds()
			: UPRChapterContentRegistryDataAsset::GetAuditorDirectiveIds()).Contains(DirectiveId)
		|| !RequiredDirectorRuleId.IsValid()) return false;

	const FGameplayTag ExpectedRule =
		bWarden
			? (DirectiveId == TEXT("Warden.PredictivePatrol") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false) :
				DirectiveId == TEXT("Warden.RouteForewarning") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.OptimalPath"), false) :
				DirectiveId == TEXT("Warden.PreemptiveLockdown") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"), false) :
				DirectiveId == TEXT("Warden.RiskMarking") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.RiskReward"), false) :
				FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"), false))
			: bPacifier ? (DirectiveId == TEXT("Pacifier.ComfortProjection") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.SurvivalProtocol"), false) :
				DirectiveId == TEXT("Pacifier.EmotionalDampening") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.EmotionalInterference"), false) :
				DirectiveId == TEXT("Pacifier.IllusionVeil") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false) :
				DirectiveId == TEXT("Pacifier.RiskSuppression") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.RiskReward"), false) :
				FGameplayTag::RequestGameplayTag(TEXT("Rule.OptimalPath"), false))
			: (DirectiveId == TEXT("Auditor.HabitReplication") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"), false) :
				DirectiveId == TEXT("Auditor.DistanceAudit") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"), false) :
				DirectiveId == TEXT("Auditor.CooperationAudit") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.CooperationAudit"), false) :
				DirectiveId == TEXT("Auditor.PredictionEscalation") ? FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false) :
				FGameplayTag::RequestGameplayTag(TEXT("Rule.ObedienceTest"), false));
	if (RequiredDirectorRuleId != ExpectedRule || PreferredRoomIds.IsEmpty() || PreferredRoomIds.Num() > 2) return false;
	TArray<FPrimaryAssetId> Canonical = PreferredRoomIds;
	Canonical.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right) { return Left.ToString() < Right.ToString(); });
	for (int32 Index = 0; Index < Canonical.Num(); ++Index)
	{
		if (!Canonical[Index].IsValid() || Canonical[Index].PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRRoom"))
			|| (Index > 0 && Canonical[Index] == Canonical[Index - 1])) return false;
	}
	return Canonical == PreferredRoomIds;
}

FPrimaryAssetId UPRChapterRuleDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("ProjectRChapterRule"), GetFName());
}
