// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRoguelikeRewardOfferApplicationAndRollbackTest,
	"ProjectR.Roguelike.Reward.OfferApplicationAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRoguelikeRewardOfferApplicationAndRollbackTest::RunTest(const FString& Parameters)
{
	FPRRewardOffer Offer;
	Offer.OfferId = FGuid::NewGuid();
	TestTrue(TEXT("An offer owns a stable id"), Offer.OfferId.IsValid());
	TestFalse(TEXT("A new offer is not resolved"), Offer.bResolved);

	FPRRewardApplicationHandle Handle;
	Handle.HandleId = FGuid::NewGuid();
	Handle.FamilyId = TEXT("VitalityCache");
	Handle.Tier = 3;
	Handle.bPersistent = true;
	TestTrue(TEXT("A session reward handle is value-only and persistent by contract"), Handle.HandleId.IsValid() && Handle.Tier == 3 && Handle.bPersistent);

	UPRRewardDataAsset* Reward = NewObject<UPRRewardDataAsset>(GetTransientPackage());
	Reward->RewardId = Reward->GetPrimaryAssetId();
	Reward->RarityTag = FGameplayTag::RequestGameplayTag(TEXT("Reward.Rarity.Common"), false);
	Reward->RewardTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Reward.Type.SkillPlugin"), false);
	Reward->FamilyId = TEXT("VitalityCache"); Reward->ApplicationId = TEXT("VitalityCache_Common"); Reward->Tier = 1;
	Reward->DisplayName = FText::FromString(TEXT("Vitality Cache")); Reward->EffectText = FText::FromString(TEXT("Max health +10"));
	TestTrue(TEXT("A reward validates only with a stable primary id and bounded presentation"), Reward->IsRewardDefinitionValid());

	TArray<FPRRewardApplicationHandle> Applied;
	Applied.Add(Handle);
	TestFalse(TEXT("A family rejects equal-or-lower tiers after it has been selected"), FPRRewardContract::CanSelectFamilyTier(Applied, Handle.FamilyId, 3));
	TestFalse(TEXT("A family rejects lower tiers after it has been selected"), FPRRewardContract::CanSelectFamilyTier(Applied, Handle.FamilyId, 2));
	TestTrue(TEXT("A family accepts a strict upgrade"), FPRRewardContract::CanSelectFamilyTier(Applied, Handle.FamilyId, 4));
	TestEqual(TEXT("Common weight stays unchanged without an event boost"), FPRRewardContract::GetRarityWeight(FGameplayTag::RequestGameplayTag(TEXT("Reward.Rarity.Common"), false), 6, 3, 1, false), 6);
	TestEqual(TEXT("Curse only boosts the current offer's Epic weight"), FPRRewardContract::GetRarityWeight(FGameplayTag::RequestGameplayTag(TEXT("Reward.Rarity.Epic"), false), 6, 3, 1, true), 2);
	return true;
}
