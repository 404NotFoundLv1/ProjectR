// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Save/PRCompanionQuestSaveTypes.h"

#include "PRCompanionQuestTypes.generated.h"

UENUM(BlueprintType)
enum class EPRCompanionQuestState : uint8 { Locked, Available, Active, PersistencePending, ReadyToRetry, Completed };
UENUM(BlueprintType)
enum class EPRCompanionQuestOperationResult : uint8 { Started, Succeeded, AlreadyActive, AlreadyCompleted, RejectedNoProfile, RejectedRegistryUnavailable, RejectedUnknownQuest, RejectedNotEligible, RejectedBusy, RejectedNoPendingPersistence, PersistenceFailed, Invalid };

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionQuestEntrySnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient) FName QuestId;
	UPROPERTY(Transient) FGameplayTag CompanionId;
	UPROPERTY(Transient) EPRCompanionQuestState State = EPRCompanionQuestState::Locked;
	UPROPERTY(Transient) int32 Stage = 0;
	UPROPERTY(Transient) int32 Progress = 0;
	UPROPERTY(Transient) FName EntitlementId;
	UPROPERTY(Transient) FText DisplayName;
	UPROPERTY(Transient) FText ObjectiveText;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionQuestSnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient) bool bRegistryReady = false;
	UPROPERTY(Transient) bool bProfileLoaded = false;
	UPROPERTY(Transient) TArray<FPRCompanionQuestEntrySnapshot> Entries;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionQuestEntitlementSnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient) TArray<FName> EntitlementIds;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionQuestOperationEvent
{
	GENERATED_BODY()
	UPROPERTY(Transient) FGuid RequestId;
	UPROPERTY(Transient) FName QuestId;
	UPROPERTY(Transient) EPRCompanionQuestOperationResult Result = EPRCompanionQuestOperationResult::Invalid;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionQuestDialogueLine
{
	GENERATED_BODY()
	UPROPERTY(Transient) FName LineId;
	UPROPERTY(Transient) FGameplayTag CompanionId;
	UPROPERTY(Transient) FText Text;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionQuestStateChangedNative, const FPRCompanionQuestSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionQuestOperationNative, const FPRCompanionQuestOperationEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionQuestCompletedNative, FName);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionQuestEntitlementsChangedNative, const FPRCompanionQuestEntitlementSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionQuestDialogueNative, const FPRCompanionQuestDialogueLine&);
