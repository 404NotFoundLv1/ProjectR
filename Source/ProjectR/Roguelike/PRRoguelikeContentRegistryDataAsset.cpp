// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"

#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"

FPrimaryAssetId UPRRoguelikeContentRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRoguelikeRegistry")), GetFName());
}

namespace
{
template <typename TAsset>
const TAsset* FindById(const TArray<TSoftObjectPtr<TAsset>>& Assets, FPrimaryAssetId Id)
{
	for (const TSoftObjectPtr<TAsset>& Reference : Assets)
	{
		const TAsset* Asset = Reference.LoadSynchronous();
		if (Asset && Asset->GetPrimaryAssetId() == Id) return Asset;
	}
	return nullptr;
}

template <typename TAsset, typename TValidate>
bool IsSortedUniqueAndValid(const TArray<TSoftObjectPtr<TAsset>>& Assets, TValidate Validate)
{
	FString Previous;
	for (const TSoftObjectPtr<TAsset>& Reference : Assets)
	{
		const TAsset* Asset = Reference.LoadSynchronous();
		if (!Asset || !Validate(*Asset)) return false;
		const FString Current = Asset->GetPrimaryAssetId().ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current;
	}
	return true;
}
}

bool UPRRoguelikeContentRegistryDataAsset::IsRegistryReady() const
{
	if (!(Rooms.Num() == 8 && Encounters.Num() == 3 && Events.Num() == 4 && RewardPolicies.Num() == 4 && Rewards.Num() == 30
		&& IsSortedUniqueAndValid(Rooms, [](const UPRRoomDataAsset& Asset) { return Asset.IsRoomDefinitionValid(); })
		&& IsSortedUniqueAndValid(Encounters, [](const UPREncounterDataAsset& Asset) { return Asset.IsEncounterDefinitionValid(); })
		&& IsSortedUniqueAndValid(Events, [](const UPRRoomEventDataAsset& Asset) { return Asset.IsEventDefinitionValid(); })
		&& IsSortedUniqueAndValid(RewardPolicies, [](const UPRRewardPolicyDataAsset& Asset) { return Asset.IsPolicyDefinitionValid(); })
		&& IsSortedUniqueAndValid(Rewards, [](const UPRRewardDataAsset& Asset) { return Asset.IsRewardDefinitionValid(); }))) return false;

	TSet<FPrimaryAssetId> BoundEventRooms;
	for (const FPRRoomEventBinding& Binding : EventRoomBindings)
	{
		const UPRRoomDataAsset* Room = FindRoom(Binding.RoomId);
		if (!Room || Room->TypeTag.ToString() != TEXT("Room.Type.Event") || !FindEvent(Binding.EventId) || BoundEventRooms.Contains(Binding.RoomId)) return false;
		BoundEventRooms.Add(Binding.RoomId);
	}
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Rooms)
	{
		const UPRRoomDataAsset* Room = Reference.LoadSynchronous();
		if (!Room || !FindEncounter(Room->EncounterId) || !FindPolicy(Room->RewardPolicyId)) return false;
		const FString LevelPath = Room->LevelAsset.ToSoftObjectPath().ToString();
		if (LevelPath != TEXT("/Game/ProjectR/Maps/L_CombatGym.L_CombatGym") && LevelPath != TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym")) return false;
		if (Room->TypeTag.ToString() == TEXT("Room.Type.Event") != BoundEventRooms.Contains(Room->RoomId)) return false;
	}
	for (const TSoftObjectPtr<UPRRewardPolicyDataAsset>& Reference : RewardPolicies)
	{
		const UPRRewardPolicyDataAsset* Policy = Reference.LoadSynchronous();
		if (!Policy || Policy->RewardIds.Num() != 30) return false;
		TSet<FPrimaryAssetId> UniqueRewardIds;
		for (const FPrimaryAssetId& RewardId : Policy->RewardIds) if (!FindReward(RewardId) || UniqueRewardIds.Contains(RewardId)) return false; else UniqueRewardIds.Add(RewardId);
	}
	for (const FPRDirectorRoomWeightAdjustment& Adjustment : DirectorRoomWeightAdjustments)
	{
		if (!Adjustment.RuleId.IsValid() || !Adjustment.RuleId.ToString().StartsWith(TEXT("Rule.")) || !Adjustment.RoomType.IsValid() || !Adjustment.RoomType.ToString().StartsWith(TEXT("Room.Type.")) || Adjustment.WeightDelta == 0) return false;
	}
	return BoundEventRooms.Num() == 4;
}

const UPRRoomDataAsset* UPRRoguelikeContentRegistryDataAsset::FindRoom(FPrimaryAssetId RoomId) const { return FindById(Rooms, RoomId); }
const UPREncounterDataAsset* UPRRoguelikeContentRegistryDataAsset::FindEncounter(FPrimaryAssetId EncounterId) const { return FindById(Encounters, EncounterId); }
const UPRRoomEventDataAsset* UPRRoguelikeContentRegistryDataAsset::FindEvent(FPrimaryAssetId EventId) const { return FindById(Events, EventId); }
const UPRRewardPolicyDataAsset* UPRRoguelikeContentRegistryDataAsset::FindPolicy(FPrimaryAssetId PolicyId) const { return FindById(RewardPolicies, PolicyId); }
const UPRRewardDataAsset* UPRRoguelikeContentRegistryDataAsset::FindReward(FPrimaryAssetId RewardId) const { return FindById(Rewards, RewardId); }

FPrimaryAssetId UPRRoguelikeContentRegistryDataAsset::FindEventForRoom(FPrimaryAssetId RoomId) const
{
	for (const FPRRoomEventBinding& Binding : EventRoomBindings)
	{
		if (Binding.RoomId == RoomId) return Binding.EventId;
	}
	return FPrimaryAssetId();
}
