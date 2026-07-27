// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRewardPolicyDataAsset.h"

FPrimaryAssetId UPRRewardPolicyDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRewardPolicy")), GetFName());
}

bool UPRRewardPolicyDataAsset::IsPolicyDefinitionValid() const
{
	return !PolicyId.IsNone() && CommonWeight >= 0 && RareWeight >= 0 && EpicWeight >= 0
		&& CommonWeight + RareWeight + EpicWeight > 0 && RewardIds.Num() >= 3;
}
