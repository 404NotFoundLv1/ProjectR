// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/Bosses/PRAuditorBossDataAsset.h"

#include "PRAuditorChapterBossDataAsset.generated.h"

/** Data-only fourth chapter extension of the frozen Auditor prototype tuning. */
UCLASS(BlueprintType)
class PROJECTR_API UPRAuditorChapterBossDataAsset final : public UPRAuditorBossDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") float RepeatedBuildWindowSeconds = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Auditor") float VerdictWindowSeconds = 6.0f;
};
