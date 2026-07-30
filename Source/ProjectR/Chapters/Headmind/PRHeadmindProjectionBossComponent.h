// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Headmind/PRHeadmindTypes.h"
#include "Components/ActorComponent.h"

#include "PRHeadmindProjectionBossComponent.generated.h"

UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRHeadmindProjectionBossComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UPRHeadmindProjectionBossComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureChapterState(int32 InSynthesisPressure, FGameplayTag InPrimaryRuleId, FGameplayTag InSecondaryRuleId, bool bFusionAvailable);
	const FPRHeadmindBossRuntimeState& GetRuntimeState() const;
private:
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void EvaluateBasePhase();
	void StartBasiliskJudgment();
	void EndBasiliskJudgment();
	void FreezeOpportunity();
	void ClearTransientState();
	void PublishRuntimeState() const;
	bool ReadTripleEligibility(FPRTripleResonanceOpportunitySnapshot& OutSnapshot) const;
	class APRHeadmindProjectionBoss* GetBoss() const;
	class APawn* GetPlayerPawn() const;
	FPRHeadmindBossRuntimeState RuntimeState;
	TWeakObjectPtr<class UPRCombatSubsystem> BoundCombat;
	FDelegateHandle CombatHandle;
	FTimerHandle BasiliskTimer;
	bool bBasiliskStarted = false;
};
