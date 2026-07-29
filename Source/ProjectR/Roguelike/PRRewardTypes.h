// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"

#include "PRRewardTypes.generated.h"

UENUM(BlueprintType)
enum class EPRRewardApplyResult : uint8
{
	Applied,
	Replaced,
	Removed,
	NotFound,
	Invalid,
	Rejected,
	ApplyFailed
};

UENUM(BlueprintType)
enum class EPRRewardAttribute : uint8
{
	MaxHealth,
	MaxShield,
	MaxEnergy,
	AttackPower,
	MoveSpeed,
	CritChance,
	Health,
	Shield,
	Energy,
	Resonance
};

UENUM(BlueprintType)
enum class EPRRewardEffectDuration : uint8
{
	Session,
	Instant
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRewardEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") EPRRewardAttribute Attribute = EPRRewardAttribute::MaxHealth;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") EPRRewardEffectDuration Duration = EPRRewardEffectDuration::Session;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") float Magnitude = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRewardOfferChoice
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RewardId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag RarityTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName FamilyId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FText DisplayName;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FText EffectText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FText CostText;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRewardOffer
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid OfferId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRewardOfferChoice> Choices;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bResolved = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRewardApplicationHandle
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid HandleId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RewardId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName FamilyId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 Tier = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName ApplicationId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bPersistent = false;
};

/** Read-only, value-only projection of a chosen reward for chapter mechanics. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRAppliedRewardSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RewardId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName FamilyId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 Tier = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPRRewardEffectSpec EffectSpec;
};

/** Pure value rules shared by offer generation and native automation. */
class PROJECTR_API FPRRewardContract
{
public:
	static bool CanSelectFamilyTier(const TArray<FPRRewardApplicationHandle>& AppliedRewards, const FName FamilyId, const int32 Tier)
	{
		if (FamilyId.IsNone() || Tier <= 0) return false;
		for (const FPRRewardApplicationHandle& Handle : AppliedRewards)
		{
			if (Handle.FamilyId == FamilyId && Tier <= Handle.Tier) return false;
		}
		return true;
	}

	static int32 GetRarityWeight(const FGameplayTag RarityTag, const int32 CommonWeight, const int32 RareWeight, const int32 EpicWeight, const bool bEpicWeightBoosted)
	{
		const FString Name = RarityTag.ToString();
		if (Name == TEXT("Reward.Rarity.Common")) return FMath::Max(0, CommonWeight);
		if (Name == TEXT("Reward.Rarity.Rare")) return FMath::Max(0, RareWeight);
		if (Name == TEXT("Reward.Rarity.Epic")) return FMath::Max(0, EpicWeight) * (bEpicWeightBoosted ? 2 : 1);
		return 0;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRRewardOfferChangedNative, const FPRRewardOffer&);
