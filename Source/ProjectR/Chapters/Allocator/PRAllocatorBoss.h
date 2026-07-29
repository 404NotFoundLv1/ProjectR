// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyCharacter.h"

#include "PRAllocatorBoss.generated.h"

class UPRAllocatorBossComponent;

/** Allocator remains an Enemy actor; the component owns only Chapter-local mechanics. */
UCLASS()
class PROJECTR_API APRAllocatorBoss : public APREnemyCharacter
{
	GENERATED_BODY()

public:
	APRAllocatorBoss();
	UPRAllocatorBossComponent* GetAllocatorBossComponent() const;

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Allocator") TObjectPtr<UPRAllocatorBossComponent> AllocatorBossComponent;
};
