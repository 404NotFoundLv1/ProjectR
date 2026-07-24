// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"
#include "GameplayTagContainer.h"

#include "PRDialogueTypes.generated.h"

UENUM(BlueprintType)
enum class EPRDialogueTriggerKind : uint8
{
	CriticalLowHealth,
	LowHealth,
	QTESuccess,
	QTEFailure,
	SupportApplied,
	SupportRejected,
	BossPhaseChanged,
	PredictionBlocked,
	SafeCombatCleared
};

UENUM(BlueprintType)
enum class EPRDialoguePresentationState : uint8
{
	Idle,
	Bark,
	Choice
};

UENUM(BlueprintType)
enum class EPRDialogueChoiceResolution : uint8
{
	Applied,
	NoRelationshipChange,
	RejectedNoProfile,
	RejectedInvalid,
	RejectedExpired,
	Cancelled
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDialogueLineDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FName LineId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") EPRDialogueTriggerKind TriggerKind = EPRDialogueTriggerKind::CriticalLowHealth;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") int32 Priority = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") float DisplayDurationSeconds = 2.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") float CooldownSeconds = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FText Text;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDialogueChoiceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FName ChoiceId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FText Text;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FPRRelationshipDelta RelationshipDelta;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDialogueRequest
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FName LineId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") EPRDialogueTriggerKind TriggerKind = EPRDialogueTriggerKind::CriticalLowHealth;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") int32 Priority = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid SourceEventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") double WorldTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDialogueRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FName LineId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") EPRDialogueTriggerKind TriggerKind = EPRDialogueTriggerKind::CriticalLowHealth;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") EPRDialoguePresentationState PresentationState = EPRDialoguePresentationState::Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FText SpeakerText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FText DialogueText;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") TArray<FPRDialogueChoiceDefinition> Choices;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") double StartTimeSeconds = 0.0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") double ExpireTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDialogueResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid ResultId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid RequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FName ChoiceId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") EPRDialogueChoiceResolution Resolution = EPRDialogueChoiceResolution::RejectedInvalid;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FPRRelationshipDelta RelationshipDeltaRequest;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") FGuid SaveRequestId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Dialogue") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRDialogueStateChangedNative, const FPRDialogueRuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRDialogueResultNative, const FPRDialogueResult&);

class PROJECTR_API FPRDialogueContract
{
public:
	static const TArray<FName>& GetExpectedCompanionAssetNames();
	static int32 GetTriggerPriority(EPRDialogueTriggerKind TriggerKind);
	static float GetTriggerCooldown(EPRDialogueTriggerKind TriggerKind);
	static bool IsSafeStateTrigger(EPRDialogueTriggerKind TriggerKind);
	static bool IsRequestBefore(const FPRDialogueRequest& Left, const FPRDialogueRequest& Right);
	static bool IsWithinEventDedupWindow(double PreviousEventTimeSeconds, double CurrentEventTimeSeconds);
	static double GetDeferredStartDelay(const TArray<double>& RecentStartTimes, double CurrentTimeSeconds);
	static bool IsQueuedRequestExpired(double EnqueueTimeSeconds, double CurrentTimeSeconds);
	static FName GetLineCooldownKey(const FGameplayTag& CompanionId, FName LineId);
};
