// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"

#include "PRAbilityTypes.generated.h"

class UPRAbilitySetDataAsset;
class UPRGameplayAbility;
class UPrimaryDataAsset;

UENUM(BlueprintType)
enum class EPRAbilityActivationPolicy : uint8
{
	OnInputTriggered = 0,
	WhileInputActive = 1,
	Passive = 2,
	ServerTriggered = 3
};

UENUM(BlueprintType)
enum class EPRAbilitySetGrantMode : uint8
{
	InitializationOnly,
	AllEntries
};

UENUM()
enum class EPRAbilitySetOperationStatus : uint8
{
	Applied,
	AlreadyApplied,
	Removed,
	NotFound,
	NotAuthoritative,
	Invalid
};

UENUM()
enum class EPRAbilityLifecycleEventType : uint8
{
	Granted,
	Removed,
	Activated,
	ActivationFailed,
	Committed,
	CommitFailed,
	Ended,
	Cancelled
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRAbilitySetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	TSubclassOf<UPRGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities", meta=(ClampMin="1"))
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	bool bGrantOnInitialization = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	FGameplayTagContainer GrantedSpecTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	TObjectPtr<UPrimaryDataAsset> AbilityData = nullptr;
};

struct PROJECTR_API FPRAbilitySetGrantHandle
{
	FGuid GrantId;
	TWeakObjectPtr<UPRAbilitySetDataAsset> AbilitySet;
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

struct PROJECTR_API FPRAbilityRuntimeState
{
	FGameplayAbilitySpecHandle SpecHandle;
	FGameplayTag AbilityTag;
	FGameplayTag InputTag;
	EPRAbilityActivationPolicy ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
	int32 AbilityLevel = 1;
	bool bActive = false;
	bool bInputHeld = false;
	bool bCanActivate = false;
	float EnergyCost = 0.0f;
	float CooldownDuration = 0.0f;
	float CooldownRemaining = 0.0f;
	FGameplayTagContainer FailureTags;
};

struct PROJECTR_API FPRAbilityLifecycleEvent
{
	EPRAbilityLifecycleEventType EventType = EPRAbilityLifecycleEventType::Granted;
	FPRAbilityRuntimeState AbilityState;
	FGameplayTagContainer FailureTags;
	double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FPRAbilityLifecycleEventNative,
	const FPRAbilityLifecycleEvent&);
