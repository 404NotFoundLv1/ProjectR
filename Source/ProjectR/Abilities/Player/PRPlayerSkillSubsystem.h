// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRPlayerSkillSubsystem.generated.h"

class ACharacter;
class UCharacterMovementComponent;
struct FPRCombatOutcomeRequest;

/** World-owned deterministic target query and controlled-displacement dispatcher. */
UCLASS()
class PROJECTR_API UPRPlayerSkillSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool QueryTargets(
		const FPRAbilityTargetQuery& Query,
		FPRAbilityTargetQueryResult& OutResult,
		FGameplayTag& OutFailureTag) const;

	bool RequestDisplacement(
		const FPRAbilityDisplacementRequest& Request,
		FGuid& OutRequestId,
		FGameplayTag& OutFailureTag);
	bool CancelDisplacement(FGuid RequestId);

	FPRAbilityDisplacementFinishedNative& OnDisplacementFinished();
	bool PublishAbilityOutcome(const FPRCombatOutcomeRequest& Request) const;

private:
	struct FActiveDisplacement
	{
		FGuid RequestId;
		FGameplayTag AbilityTag;
		TWeakObjectPtr<AActor> SourceActor;
		TWeakObjectPtr<ACharacter> TargetCharacter;
		FVector PlaneOrigin = FVector::ZeroVector;
		FVector EffectiveEnd = FVector::ZeroVector;
		uint16 RootMotionSourceId = 0;
		FTimerHandle CompletionTimerHandle;
		FTimerHandle MonitorTimerHandle;
	};

	void HandleDisplacementMonitor(FGuid RequestId);
	void HandleDisplacementDurationElapsed(FGuid RequestId);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void FinishDisplacement(
		FGuid RequestId,
		EPRAbilityDisplacementEndReason EndReason,
		bool bBroadcast);
	bool HasWorldStaticObstruction(
		const ACharacter& Character,
		FVector Start,
		FVector End,
		const AActor* SourceActor) const;
	void CleanupAllDisplacements();

	TMap<FGuid, FActiveDisplacement> ActiveDisplacements;
	FPRAbilityDisplacementFinishedNative DisplacementFinishedEvent;
	FDelegateHandle WorldCleanupHandle;
};
