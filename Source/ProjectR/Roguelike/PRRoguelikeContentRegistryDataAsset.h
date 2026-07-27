// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorTypes.h"
#include "Engine/DataAsset.h"

#include "PRRoguelikeContentRegistryDataAsset.generated.h"

class UPREncounterDataAsset;
class UPRRewardDataAsset;
class UPRRewardPolicyDataAsset;
class UPRRoomDataAsset;
class UPRRoomEventDataAsset;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomEventBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RoomId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId EventId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDirectorRoomWeightAdjustment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag RuleId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag RoomType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 WeightDelta = 0;
};

UCLASS(BlueprintType)
class PROJECTR_API UPRRoguelikeContentRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsRegistryReady() const;
	const UPRRoomDataAsset* FindRoom(FPrimaryAssetId RoomId) const;
	const UPREncounterDataAsset* FindEncounter(FPrimaryAssetId EncounterId) const;
	const UPRRoomEventDataAsset* FindEvent(FPrimaryAssetId EventId) const;
	const UPRRewardPolicyDataAsset* FindPolicy(FPrimaryAssetId PolicyId) const;
	const UPRRewardDataAsset* FindReward(FPrimaryAssetId RewardId) const;
	FPrimaryAssetId FindEventForRoom(FPrimaryAssetId RoomId) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<TSoftObjectPtr<UPRRoomDataAsset>> Rooms;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<TSoftObjectPtr<UPREncounterDataAsset>> Encounters;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<TSoftObjectPtr<UPRRoomEventDataAsset>> Events;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<TSoftObjectPtr<UPRRewardPolicyDataAsset>> RewardPolicies;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<TSoftObjectPtr<UPRRewardDataAsset>> Rewards;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRoomEventBinding> EventRoomBindings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRDirectorRoomWeightAdjustment> DirectorRoomWeightAdjustments;
};
