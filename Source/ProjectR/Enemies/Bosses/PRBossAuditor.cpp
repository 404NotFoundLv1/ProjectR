// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/Bosses/PRBossAuditor.h"

#include "Enemies/Bosses/PRAuditorBossComponent.h"

APRBossAuditor::APRBossAuditor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AuditorBossComponent = CreateDefaultSubobject<UPRAuditorBossComponent>(TEXT("AuditorBossComponent"));
}

EPRCombatMitigationResult APRBossAuditor::EvaluateIncomingDamage(const FPRDamageRequest& Request, FGameplayTagContainer& OutResponseTags) const
{
	return AuditorBossComponent && AuditorBossComponent->IsPredictionBlocking(Request, OutResponseTags)
		? EPRCombatMitigationResult::Blocked : EPRCombatMitigationResult::NotHandled;
}

UPRAuditorBossComponent* APRBossAuditor::GetAuditorBossComponent() const
{
	return AuditorBossComponent;
}
