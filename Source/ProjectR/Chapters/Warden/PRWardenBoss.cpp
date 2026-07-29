// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Warden/PRWardenBoss.h"

#include "Chapters/Warden/PRWardenBossComponent.h"

APRWardenBoss::APRWardenBoss()
{
	WardenBossComponent = CreateDefaultSubobject<UPRWardenBossComponent>(TEXT("WardenBossComponent"));
}

UPRWardenBossComponent* APRWardenBoss::GetWardenBossComponent() const
{
	return WardenBossComponent;
}
