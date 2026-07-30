// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"

#include "PRChapterRoguelikeContentRegistryDataAsset.generated.h"

class UPRChapterRuleDataAsset;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRChapterEventPressureBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId EventId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ChoiceId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") int32 PressureDelta = 0;
	/** Closed Warden-only future candidates to remove atomically after this choice. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") TArray<FPrimaryAssetId> ExcludedFutureRoomIds;
};

/**
 * Closed Allocator-only room content pack.  It deliberately derives from the
 * base registry so the Room subsystem can retain its v0.4.2 default path while
 * accepting an explicitly configured Chapter registry.
 */
UCLASS(BlueprintType)
class PROJECTR_API UPRChapterRoguelikeContentRegistryDataAsset final : public UPRRoguelikeContentRegistryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual bool IsRegistryReady() const override;
	/** Fixed event-choice binding count for each closed chapter content pack. */
	static int32 GetExpectedEventPressureBindingCount(FName InContentId);
	bool SupportsChapterShopRooms() const;
	bool IsKnownDirective(FName DirectiveId) const;
	const UPRChapterRuleDataAsset* FindChapterRule(FName DirectiveId) const;
	bool FindPressureDelta(FPrimaryAssetId EventId, FName ChoiceId, int32& OutDelta) const;
	bool FindEventPressureBinding(FPrimaryAssetId EventId, FName ChoiceId, FPRChapterEventPressureBinding& OutBinding) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ContentId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") TArray<TSoftObjectPtr<UPRChapterRuleDataAsset>> ChapterRules;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") TArray<FPRChapterEventPressureBinding> EventPressureBindings;
};
