// Copyright ProjectR. All Rights Reserved.

#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"

namespace PRChapterRegistry
{
const FPrimaryAssetType RegistryType(TEXT("ProjectRChapterRoguelikeRegistry"));
const FString NetworkMapPath(TEXT("/Game/ProjectR/Maps/L_Network_Prototype.L_Network_Prototype"));

template <typename TAsset>
bool IsSortedUniqueAndValid(const TArray<TSoftObjectPtr<TAsset>>& Assets, bool (TAsset::*Validate)() const)
{
	FString Previous;
	for (const TSoftObjectPtr<TAsset>& Reference : Assets)
	{
		const TAsset* Asset = Reference.LoadSynchronous();
		if (!Asset || !(Asset->*Validate)()) return false;
		const FString Current = Asset->GetPrimaryAssetId().ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current;
	}
	return true;
}
}

FPrimaryAssetId UPRChapterRoguelikeContentRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PRChapterRegistry::RegistryType, GetFName());
}

bool UPRChapterRoguelikeContentRegistryDataAsset::SupportsChapterShopRooms() const
{
	return ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId();
}

bool UPRChapterRoguelikeContentRegistryDataAsset::IsKnownDirective(const FName DirectiveId) const
{
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : ChapterRules)
	{
		const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (Rule && Rule->DirectiveId == DirectiveId && Rule->IsRuleDefinitionValid()) return true;
	}
	return false;
}

bool UPRChapterRoguelikeContentRegistryDataAsset::FindPressureDelta(const FPrimaryAssetId EventId, const FName ChoiceId, int32& OutDelta) const
{
	OutDelta = 0;
	for (const FPRChapterEventPressureBinding& Binding : EventPressureBindings)
	{
		if (Binding.EventId == EventId && Binding.ChoiceId == ChoiceId && FMath::Abs(Binding.PressureDelta) <= 1)
		{
			OutDelta = Binding.PressureDelta;
			return true;
		}
	}
	return false;
}

bool UPRChapterRoguelikeContentRegistryDataAsset::IsRegistryReady() const
{
	if (!SupportsChapterShopRooms() || Rooms.Num() != 10 || Encounters.Num() != 4 || Events.Num() != 4 || RewardPolicies.Num() != 5 || Rewards.Num() != 30 || ChapterRules.Num() != 5 || EventPressureBindings.Num() != 10)
	{
		return false;
	}

	FString PreviousRuleId;
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : ChapterRules)
	{
		const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (!Rule || !Rule->IsRuleDefinitionValid() || Rule->ContentId != ContentId || (!PreviousRuleId.IsEmpty() && PreviousRuleId >= Rule->DirectiveId.ToString())) return false;
		PreviousRuleId = Rule->DirectiveId.ToString();
	}
	if (PreviousRuleId.IsEmpty()) return false;
	TSet<FString> UniqueEventChoices;
	for (const FPRChapterEventPressureBinding& Binding : EventPressureBindings)
	{
		const UPRRoomEventDataAsset* Event = FindEvent(Binding.EventId);
		if (!Event || Binding.ChoiceId.IsNone() || FMath::Abs(Binding.PressureDelta) > 1
			|| !Event->Choices.ContainsByPredicate([&Binding](const FPRRoomEventChoice& Choice) { return Choice.ChoiceId == Binding.ChoiceId; })) return false;
		const FString Key = Binding.EventId.ToString() + TEXT(".") + Binding.ChoiceId.ToString();
		if (UniqueEventChoices.Contains(Key)) return false;
		UniqueEventChoices.Add(Key);
	}

	if (!PRChapterRegistry::IsSortedUniqueAndValid(Rooms, &UPRRoomDataAsset::IsRoomDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Encounters, &UPREncounterDataAsset::IsEncounterDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Events, &UPRRoomEventDataAsset::IsEventDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(RewardPolicies, &UPRRewardPolicyDataAsset::IsPolicyDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Rewards, &UPRRewardDataAsset::IsRewardDefinitionValid)) return false;

	int32 ShopRooms = 0;
	TSet<FPrimaryAssetId> BoundEventRooms;
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Rooms)
	{
		const UPRRoomDataAsset* Room = Reference.LoadSynchronous();
		if (!Room || Room->LevelAsset.ToSoftObjectPath().ToString() != PRChapterRegistry::NetworkMapPath
			|| !FindEncounter(Room->EncounterId) || !FindPolicy(Room->RewardPolicyId)) return false;
		if (Room->TypeTag.ToString() == TEXT("Room.Type.Shop")) ++ShopRooms;
		if (Room->TypeTag.ToString() == TEXT("Room.Type.Event"))
		{
			const FPrimaryAssetId EventId = FindEventForRoom(Room->RoomId);
			if (!EventId.IsValid() || !FindEvent(EventId) || BoundEventRooms.Contains(Room->RoomId)) return false;
			BoundEventRooms.Add(Room->RoomId);
		}
	}
	if (ShopRooms != 1 || BoundEventRooms.Num() != 4 || EventRoomBindings.Num() != 4) return false;
	for (const FPRRoomEventBinding& Binding : EventRoomBindings)
	{
		const UPRRoomDataAsset* Room = FindRoom(Binding.RoomId);
		if (!Room || Room->TypeTag.ToString() != TEXT("Room.Type.Event") || !FindEvent(Binding.EventId)) return false;
	}
	for (const TSoftObjectPtr<UPRRewardPolicyDataAsset>& Reference : RewardPolicies)
	{
		const UPRRewardPolicyDataAsset* Policy = Reference.LoadSynchronous();
		if (!Policy || Policy->RewardIds.Num() != 30) return false;
		TSet<FPrimaryAssetId> UniqueRewards;
		for (const FPrimaryAssetId& RewardId : Policy->RewardIds)
		{
			if (!FindReward(RewardId) || UniqueRewards.Contains(RewardId)) return false;
			UniqueRewards.Add(RewardId);
		}
	}
	return true;
}
