// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Warden/PRWardenTypes.h"
#include "Components/ActorComponent.h"

#include "PRWardenBossComponent.generated.h"

UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRWardenBossComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRWardenBossComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureChapterState(int32 RiskPressure);
	const FPRWardenBossRuntimeState& GetRuntimeState() const;

private:
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void EvaluatePhases();
	void EnterPredictiveAttack();
	void EnterPlatformLockdown();
	void EnterRiskMark();
	void ResolvePredictiveAttack();
	void BeginPlatformLockdownWindow();
	void ResolvePlatformLockdown();
	void ResolveRiskMark();
	void ApplyPlayerDamage(float Damage, FName SourceId);
	void PublishCompletion();
	class APREnemyCharacter* GetBoss() const;
	class APawn* GetPlayerPawn() const;
	bool IsLegalP0Skill(FGameplayTag AbilityTag) const;
	EPRWardenArenaLane GetRelativeLane(const class AActor* Player) const;

	FPRWardenBossRuntimeState RuntimeState;
	TSet<FGameplayTag> RiskCounterSkills;
	TWeakObjectPtr<class UPRCombatSubsystem> BoundCombat;
	FDelegateHandle CombatHandle;
	FTimerHandle PredictiveTimer;
	FTimerHandle LockdownWarningTimer;
	FTimerHandle LockdownTimer;
	FTimerHandle RiskMarkTimer;
	bool bCompletionPublished = false;
};
