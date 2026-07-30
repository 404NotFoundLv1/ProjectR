// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/Bosses/PRAuditorBossDataAsset.h"

#include "PRHeadmindProjectionBossDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRHeadmindProjectionBossDataAsset final : public UPRAuditorBossDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Headmind") float BasiliskJudgmentWindowSeconds = 5.0f;
};
