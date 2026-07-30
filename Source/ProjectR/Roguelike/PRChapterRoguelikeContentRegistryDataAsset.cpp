// Copyright ProjectR. All Rights Reserved.

#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "ProjectR.h"

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
	return ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId()
		|| ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
		|| ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
		|| ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId();
}

int32 UPRChapterRoguelikeContentRegistryDataAsset::GetExpectedEventPressureBindingCount(const FName InContentId)
{
	// Auditor's four fixed events expose 2 + 2 + 2 + 3 declared choices.
	return InContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId() ? 9 : 10;
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

const UPRChapterRuleDataAsset* UPRChapterRoguelikeContentRegistryDataAsset::FindChapterRule(const FName DirectiveId) const
{
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : ChapterRules)
	{
		const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (Rule && Rule->DirectiveId == DirectiveId && Rule->IsRuleDefinitionValid()) return Rule;
	}
	return nullptr;
}

bool UPRChapterRoguelikeContentRegistryDataAsset::FindPressureDelta(const FPrimaryAssetId EventId, const FName ChoiceId, int32& OutDelta) const
{
	FPRChapterEventPressureBinding Binding;
	if (!FindEventPressureBinding(EventId, ChoiceId, Binding)) { OutDelta = 0; return false; }
	OutDelta = Binding.PressureDelta;
	return true;
}

bool UPRChapterRoguelikeContentRegistryDataAsset::FindEventPressureBinding(const FPrimaryAssetId EventId, const FName ChoiceId, FPRChapterEventPressureBinding& OutBinding) const
{
	OutBinding = FPRChapterEventPressureBinding();
	for (const FPRChapterEventPressureBinding& Binding : EventPressureBindings)
	{
		if (Binding.EventId == EventId && Binding.ChoiceId == ChoiceId && FMath::Abs(Binding.PressureDelta) <= 1)
		{
			OutBinding = Binding;
			return true;
		}
	}
	return false;
}

bool UPRChapterRoguelikeContentRegistryDataAsset::IsRegistryReady() const
{
	auto Fail = [this](const TCHAR* Reason) { UE_LOG(LogProjectR, Warning, TEXT("Chapter registry %s rejected: %s"), *GetPathName(), Reason); return false; };
	if (!SupportsChapterShopRooms() || Rooms.Num() != 10 || Encounters.Num() != 4 || Events.Num() != 4 || RewardPolicies.Num() != 5 || Rewards.Num() != 30 || ChapterRules.Num() != 5 || EventPressureBindings.Num() != GetExpectedEventPressureBindingCount(ContentId))
	{
		return Fail(TEXT("manifest-count-or-content"));
	}

	FString PreviousRuleId;
	TArray<FName> ActualRuleIds;
	for (const TSoftObjectPtr<UPRChapterRuleDataAsset>& Reference : ChapterRules)
	{
		const UPRChapterRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (!Rule || !Rule->IsRuleDefinitionValid() || Rule->ContentId != ContentId || (!PreviousRuleId.IsEmpty() && PreviousRuleId >= Rule->DirectiveId.ToString())) return Fail(TEXT("rule-validity-or-order"));
		for (const FPrimaryAssetId& PreferredRoomId : Rule->PreferredRoomIds) if (!FindRoom(PreferredRoomId)) return Fail(TEXT("rule-preferred-room"));
		PreviousRuleId = Rule->DirectiveId.ToString();
		ActualRuleIds.Add(Rule->DirectiveId);
	}
	if (PreviousRuleId.IsEmpty()) return Fail(TEXT("missing-rules"));
	TArray<FName> ExpectedRuleIds =
		ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId()
			? UPRChapterContentRegistryDataAsset::GetAllocatorDirectiveIds()
			: ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
				? UPRChapterContentRegistryDataAsset::GetWardenDirectiveIds()
				: ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
					? UPRChapterContentRegistryDataAsset::GetPacifierDirectiveIds()
					: UPRChapterContentRegistryDataAsset::GetAuditorDirectiveIds();
	ExpectedRuleIds.Sort([](const FName& Left, const FName& Right) { return Left.LexicalLess(Right); });
	if (ActualRuleIds != ExpectedRuleIds) return Fail(TEXT("rule-whitelist"));
	TSet<FString> UniqueEventChoices;
	for (const FPRChapterEventPressureBinding& Binding : EventPressureBindings)
	{
		const UPRRoomEventDataAsset* Event = FindEvent(Binding.EventId);
		if (!Event || Binding.ChoiceId.IsNone() || FMath::Abs(Binding.PressureDelta) > 1
			|| !Event->Choices.ContainsByPredicate([&Binding](const FPRRoomEventChoice& Choice) { return Choice.ChoiceId == Binding.ChoiceId; })) return Fail(TEXT("pressure-choice"));
		if (ContentId == UPRChapterContentRegistryDataAsset::GetAllocatorContentId() && !Binding.ExcludedFutureRoomIds.IsEmpty()) return Fail(TEXT("allocator-exclusion"));
		FString PreviousExcluded;
		for (const FPrimaryAssetId& ExcludedRoomId : Binding.ExcludedFutureRoomIds)
		{
			if (!ExcludedRoomId.IsValid() || ExcludedRoomId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRRoom"))
				|| !FindRoom(ExcludedRoomId)
				|| (!PreviousExcluded.IsEmpty() && PreviousExcluded >= ExcludedRoomId.ToString())) return Fail(TEXT("excluded-room"));
			PreviousExcluded = ExcludedRoomId.ToString();
		}
		const FString Key = Binding.EventId.ToString() + TEXT(".") + Binding.ChoiceId.ToString();
		if (UniqueEventChoices.Contains(Key)) return Fail(TEXT("duplicate-pressure-choice"));
		UniqueEventChoices.Add(Key);
	}

	if (!PRChapterRegistry::IsSortedUniqueAndValid(Rooms, &UPRRoomDataAsset::IsRoomDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Encounters, &UPREncounterDataAsset::IsEncounterDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Events, &UPRRoomEventDataAsset::IsEventDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(RewardPolicies, &UPRRewardPolicyDataAsset::IsPolicyDefinitionValid)
		|| !PRChapterRegistry::IsSortedUniqueAndValid(Rewards, &UPRRewardDataAsset::IsRewardDefinitionValid)) return Fail(TEXT("asset-sort-or-validity"));

	int32 ShopRooms = 0;
	TSet<FPrimaryAssetId> BoundEventRooms;
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Rooms)
	{
		const UPRRoomDataAsset* Room = Reference.LoadSynchronous();
		if (!Room || Room->LevelAsset.ToSoftObjectPath().ToString() != PRChapterRegistry::NetworkMapPath
			|| !FindEncounter(Room->EncounterId) || !FindPolicy(Room->RewardPolicyId)) return Fail(TEXT("room-closure"));
		if (Room->TypeTag.ToString() == TEXT("Room.Type.Shop")) ++ShopRooms;
		if (Room->TypeTag.ToString() == TEXT("Room.Type.Event"))
		{
			const FPrimaryAssetId EventId = FindEventForRoom(Room->RoomId);
			if (!EventId.IsValid() || !FindEvent(EventId) || BoundEventRooms.Contains(Room->RoomId)) return Fail(TEXT("event-room-closure"));
			BoundEventRooms.Add(Room->RoomId);
		}
	}
	if (ShopRooms != 1 || BoundEventRooms.Num() != 4 || EventRoomBindings.Num() != 4) return Fail(TEXT("room-coverage"));
	for (const FPRRoomEventBinding& Binding : EventRoomBindings)
	{
		const UPRRoomDataAsset* Room = FindRoom(Binding.RoomId);
		if (!Room || Room->TypeTag.ToString() != TEXT("Room.Type.Event") || !FindEvent(Binding.EventId)) return Fail(TEXT("event-binding"));
	}
	for (const TSoftObjectPtr<UPRRewardPolicyDataAsset>& Reference : RewardPolicies)
	{
		const UPRRewardPolicyDataAsset* Policy = Reference.LoadSynchronous();
		if (!Policy || Policy->RewardIds.Num() != 30) return Fail(TEXT("policy-count"));
		TSet<FPrimaryAssetId> UniqueRewards;
		for (const FPrimaryAssetId& RewardId : Policy->RewardIds)
		{
			if (!FindReward(RewardId) || UniqueRewards.Contains(RewardId)) return Fail(TEXT("policy-reward-closure"));
			UniqueRewards.Add(RewardId);
		}
	}
	return true;
}
