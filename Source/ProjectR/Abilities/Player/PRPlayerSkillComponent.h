// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "Components/ActorComponent.h"

#include "PRPlayerSkillComponent.generated.h"

struct FPRDamageRequest;
class APRPlayerCharacter;
class UPRAbilitySystemComponent;
class UPRGA_CounterProofWall;
class UPRPlayerSkillDataAsset;
class UNiagaraComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;
struct FStreamableHandle;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FPRPlayerSkillPhaseExpiredNative,
	FGameplayTag,
	EPRPlayerSkillPhase);

/** Avatar-local player-skill phase and controlled-displacement state. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRPlayerSkillComponent();

	bool BeginPhase(FGameplayTag AbilityTag, EPRPlayerSkillPhase Phase, float Duration);
	void CancelExecution();
	void ClearExecution();

	FGameplayTag GetCurrentAbilityTag() const;
	EPRPlayerSkillPhase GetCurrentPhase() const;
	FGuid GetActiveDisplacementRequestId() const;
	FPRPlayerSkillPhaseExpiredNative& OnPhaseExpired();

	bool RequestOwnedDisplacement(
		const FPRAbilityDisplacementRequest& Request,
		FGameplayTag& OutFailureTag);
	void CancelOwnedDisplacement();

	EPRCombatMitigationResult EvaluateIncomingDamage(
		const FPRDamageRequest& Request,
		FGameplayTagContainer& OutResponseTags) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class APRPlayerCharacter;
	friend class UPRGA_CounterProofWall;
	friend class UPRPlayerSkillGameplayAbility;

	bool BeginCounterProofGuard(FGameplayTag AbilityTag, float HalfAngleDegrees, double PerfectTimingEndTime);
	void ClearCounterProofGuard(FGameplayTag AbilityTag);
	void PreloadSkillPresentation(UPRAbilitySystemComponent* AbilitySystemComponent);
	void PlaySkillPresentation(const UPRPlayerSkillDataAsset* SkillData);
	void StopSkillPresentation(FGameplayTag AbilityTag);
	void ClearSkillPresentation();
	void HandlePresentationPreloadCompleted(uint32 LoadGeneration);
	void WarnPresentationOnce(FName WarningKey, const FString& Message);
	bool ShouldPlayLocalPresentation() const;
	void HandlePhaseExpired();
	void HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result);
	void EnsureSubsystemBinding();

	struct FCounterProofGuardContext
	{
		FGameplayTag AbilityTag;
		float HalfAngleDegrees = 0.0f;
		double PerfectTimingEndTime = 0.0;
		bool bActive = false;
	};

	FGameplayTag CurrentAbilityTag;
	EPRPlayerSkillPhase CurrentPhase = EPRPlayerSkillPhase::Idle;
	FTimerHandle PhaseTimerHandle;
	FGuid ActiveDisplacementRequestId;
	FDelegateHandle DisplacementFinishedHandle;
	FPRPlayerSkillPhaseExpiredNative PhaseExpiredEvent;
	FCounterProofGuardContext CounterProofGuard;
	TSharedPtr<FStreamableHandle> PresentationLoadHandle;
	uint32 PresentationLoadGeneration = 0;
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UPRPlayerSkillDataAsset>> PresentationDataByTag;
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UNiagaraSystem>> LoadedPresentationVFX;
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USoundBase>> LoadedPresentationSFX;
	TMap<FGameplayTag, TWeakObjectPtr<UNiagaraComponent>> ActivePresentationVFX;
	TMap<FGameplayTag, TWeakObjectPtr<UAudioComponent>> ActivePresentationSFX;
	TSet<FName> PresentationWarningKeys;
};
