// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRCombatAttackProxyInterface.h"
#include "Core/PRCombatEventSubjectInterface.h"
#include "GameFramework/Actor.h"

#include "PRSkillDecoyActor.generated.h"

/** One-shot, non-combat proxy created by AfterimageDodge. */
UCLASS(NotBlueprintable, NotPlaceable)
class PROJECTR_API APRSkillDecoyActor final : public AActor,
	public IPRCombatAttackProxyInterface,
	public IPRCombatEventSubjectInterface
{
	GENERATED_BODY()

public:
	APRSkillDecoyActor();
	void InitializeProxy(AActor* InOwner, FName InProxyId, float InPerfectTimingWindow, float InLifetime);

	virtual FName GetAttackProxyId() const override;
	virtual AActor* GetAttackProxyOwner() const override;
	virtual bool ConsumeAttackProxy(AActor* Attacker) override;
	virtual FName GetCombatEventSubjectId() const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Expire();
	UFUNCTION()
	void HandleProxyOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	void PublishConsumptionOutcome() const;
	TWeakObjectPtr<AActor> ProxyOwner;
	FName ProxyId;
	float PerfectTimingEndTime = 0.0f;
	bool bConsumed = false;
	FTimerHandle LifetimeTimer;
};
