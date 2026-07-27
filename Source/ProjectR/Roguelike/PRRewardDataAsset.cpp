// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRewardDataAsset.h"

FPrimaryAssetId UPRRewardDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRReward")), GetFName());
}

bool UPRRewardDataAsset::IsRewardDefinitionValid() const
{
	return RewardId.IsValid() && RewardId.PrimaryAssetType == FPrimaryAssetType(TEXT("ProjectRReward")) && RewardId == GetPrimaryAssetId()
		&& RarityTag.IsValid() && RarityTag.ToString().StartsWith(TEXT("Reward.Rarity."))
		&& RewardTypeTag.IsValid() && RewardTypeTag.ToString().StartsWith(TEXT("Reward.Type."))
		&& !FamilyId.IsNone() && !ApplicationId.IsNone() && Tier > 0 && !DisplayName.IsEmpty() && !EffectText.IsEmpty();
}
