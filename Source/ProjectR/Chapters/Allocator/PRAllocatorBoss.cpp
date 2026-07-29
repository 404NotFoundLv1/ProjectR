// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Allocator/PRAllocatorBoss.h"

#include "Chapters/Allocator/PRAllocatorBossComponent.h"

APRAllocatorBoss::APRAllocatorBoss()
{
	AllocatorBossComponent = CreateDefaultSubobject<UPRAllocatorBossComponent>(TEXT("AllocatorBossComponent"));
}

UPRAllocatorBossComponent* APRAllocatorBoss::GetAllocatorBossComponent() const { return AllocatorBossComponent; }
