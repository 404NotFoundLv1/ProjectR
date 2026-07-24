// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"

#include "PRDivergenceTypes.generated.h"

class UPRDivergenceDataAsset;

UENUM(BlueprintType)
enum class EPRDivergenceState : uint8
{
	Idle = 0,
	AwaitingChoice = 1,
	Resolving = 2,
	Completed = 3
};

UENUM(BlueprintType)
enum class EPRDivergenceChoice : uint8
{
	None = 0,
	Rescue = 1,
	Leave = 2,
	FaceChallenge = 3
};

UENUM(BlueprintType)
enum class EPRDivergenceResolution : uint8
{
	Applied = 0,
	RejectedIneligible = 1,
	RejectedAlreadyConsumed = 2,
	RejectedCombat = 3,
	RejectedInvalid = 4,
	Expired = 5,
	Cancelled = 6
};

UENUM(BlueprintType)
enum class EPRDivergenceFutureDisposition : uint8
{
	None = 0,
	RescueEvacuationRequested = 1,
	LeaveRunRequested = 2,
	ChallengeContinues = 3
};

/**
 * Value-only eligibility snapshot.  The subsystem builds this from the
 * existing player, companion and save contracts; it deliberately does not
 * retain actor, component or profile object references.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergenceEligibilityInput
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bAuthority = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bCurrentPlayerDead = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bHasPrimaryCompanion = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bHasLoadedProfile = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bRunProtectionConsumed = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") int32 Trust = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") int32 Overload = 0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergenceChoicePresentation
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceChoice Choice = EPRDivergenceChoice::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FText DisplayText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FText InputHintText;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergenceRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid DeathEventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceState State = EPRDivergenceState::Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FText SpeakerText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FText PromptText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") TArray<FPRDivergenceChoicePresentation> Choices;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") double StartTimeSeconds = 0.0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") double ExpireTimeSeconds = 0.0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bRunProtectionConsumed = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergenceResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid ResultId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid DeathEventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceChoice Choice = EPRDivergenceChoice::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceResolution Resolution = EPRDivergenceResolution::RejectedInvalid;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceFutureDisposition FutureDisposition = EPRDivergenceFutureDisposition::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") bool bReviveApplied = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") float AppliedHealthFraction = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") float AppliedShieldFraction = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FPRRelationshipState PreviousRelationship;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FPRRelationshipState CurrentRelationship;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FPRRelationshipDelta RelationshipDeltaRequest;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid SaveRequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FGuid DialogueResultId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") FName DialogueChoiceId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Divergence") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRDivergenceStateChangedNative, const FPRDivergenceRuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRDivergenceResultNative, const FPRDivergenceResult&);

class PROJECTR_API FPRDivergenceContract
{
public:
	static constexpr int32 MinimumTrust = 50;
	static constexpr int32 MaximumOverloadExclusive = 80;
	static constexpr float ChoiceWindowSeconds = 30.0f;
	static constexpr float RescueHealthFraction = 0.25f;
	static constexpr float ChallengeHealthFraction = 0.10f;
	static constexpr float ReviveShieldFraction = 0.0f;

	static bool IsEligible(const FPRDivergenceEligibilityInput& Input);
	static bool GetFixedRelationshipDelta(
		FGameplayTag CompanionId,
		EPRDivergenceChoice Choice,
		FPRRelationshipDelta& OutDelta);
	static FText GetCompanionSpeaker(FGameplayTag CompanionId);
	static FText GetCompanionPrompt(FGameplayTag CompanionId);
	static FText GetChoiceText(EPRDivergenceChoice Choice);
	static FText GetChoiceInputHint(EPRDivergenceChoice Choice);
	static EPRDivergenceFutureDisposition GetFutureDisposition(EPRDivergenceChoice Choice);
	static void ConfigureFixedDefinition(UPRDivergenceDataAsset& Asset);
};
