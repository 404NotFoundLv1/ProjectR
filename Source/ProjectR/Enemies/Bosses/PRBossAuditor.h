// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/PRCombatMitigationInterface.h"
#include "Enemies/PREnemyCharacter.h"

#include "PRBossAuditor.generated.h"

class UPRAuditorBossComponent;

/** Auditor Boss actor: an Enemy with the standard self-owned ASC and a narrow Boss mechanics component. */
UCLASS()
class PROJECTR_API APRBossAuditor : public APREnemyCharacter, public IPRCombatMitigationInterface
{
	GENERATED_BODY()

public:
	APRBossAuditor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual EPRCombatMitigationResult EvaluateIncomingDamage(const FPRDamageRequest& Request, FGameplayTagContainer& OutResponseTags) const override;
	UPRAuditorBossComponent* GetAuditorBossComponent() const;

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Boss") TObjectPtr<UPRAuditorBossComponent> AuditorBossComponent;
};
