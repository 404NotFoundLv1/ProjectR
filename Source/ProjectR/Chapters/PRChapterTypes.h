// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chapters/Warden/PRWardenTypes.h"
#include "Engine/AssetManagerTypes.h"

#include "PRChapterTypes.generated.h"

UENUM(BlueprintType)
enum class EPRChapterLifecycleState : uint8
{
	Inactive,
	Configured,
	RunActive,
	ReadyToRetry,
	Completed,
	Rejected
};

UENUM(BlueprintType)
enum class EPRChapterOperationResult : uint8
{
	Succeeded,
	NotReady,
	RejectedInvalidState,
	RejectedNoProfile,
	RegistryUnavailable,
	PersistenceFailed,
	AlreadyCompleted,
	NotFound
};

UENUM(BlueprintType)
enum class EPRAllocatorBossPhase : uint8
{
	Dormant,
	ResourceLock,
	RewardDeprivation,
	PriceAudit,
	Defeated
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRAllocatorBossRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") EPRAllocatorBossPhase Phase = EPRAllocatorBossPhase::Dormant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 AllocationPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 AuditUnits = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bResourceLockCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bRewardDeprivationCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bPriceAuditBroken = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRChapterSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") EPRChapterLifecycleState State = EPRChapterLifecycleState::Inactive;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId ChapterId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName ContentId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName DirectiveId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 AllocationPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 RiskPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FPRAllocatorBossRuntimeState Boss;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FPRWardenBossRuntimeState WardenBoss;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FPRWardenStoryProjection WardenStory;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName FallbackReason;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bHasHumanAnomalyProof = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRChapterCompletionResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FGuid CompletionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId ChapterId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName ProofId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int64 SettlementSequence = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bProofAwarded = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRChapterStateChangedNative, const FPRChapterSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRChapterCompletionNative, const FPRChapterCompletionResult&);
