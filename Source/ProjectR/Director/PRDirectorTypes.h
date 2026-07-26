// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Director/PRPlayerProfileTypes.h"
#include "GameplayTagContainer.h"

#include "PRDirectorTypes.generated.h"

enum class EPRDirectorRequestStatus : uint8 { Started, Busy, Invalid, ProviderUnavailable, ShuttingDown };
enum class EPRDirectorEvaluationResult : uint8 { Applied, FallbackApplied, RejectedSchema, RejectedRequestId, RejectedUnknownRule, RejectedReasonTags, RejectedParameters, RejectedText, TimedOut, Cancelled };
enum class EPRDirectorRuleOperationResult : uint8 { Applied, AlreadyApplied, Replaced, Removed, NotFound, Invalid };

USTRUCT()
struct PROJECTR_API FPRDirectorNumericParameter
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FName Name;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") float Value = 0.0f;
	bool operator==(const FPRDirectorNumericParameter& Other) const { return Name == Other.Name && Value == Other.Value; }
};

USTRUCT()
struct PROJECTR_API FPRDirectorParameterDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FName Name;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") float Minimum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") float Maximum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") float DefaultValue = 0.0f;
};
struct PROJECTR_API FPRDirectorRequest { int32 SchemaVersion = 1; FGuid RequestId; FPRPlayerProfileSnapshot Profile; TArray<FGameplayTag> CandidateRuleIds; int64 RequestSequence = 0; float TimeoutSeconds = 0.5f; };
struct PROJECTR_API FPRDirectorResponse { int32 SchemaVersion = 1; FGuid RequestId; FGameplayTag RuleId; int32 Level = 1; FGameplayTagContainer ReasonTags; TArray<FPRDirectorNumericParameter> Parameters; FString VisibleReason; FString ExpressionText; };
struct PROJECTR_API FPRAppliedDirectorRuleHandle { FGuid HandleId; FGameplayTag RuleId; int32 Level = 1; TArray<FPRDirectorNumericParameter> Parameters; int64 ApplySequence = 0; };
struct PROJECTR_API FPRDirectorValidationResult { EPRDirectorEvaluationResult Result = EPRDirectorEvaluationResult::RejectedSchema; FPRDirectorResponse CanonicalResponse; };

DECLARE_DELEGATE_OneParam(FPRDirectorProviderCompletion, const FPRDirectorResponse&);
DECLARE_MULTICAST_DELEGATE_FourParams(FPRDirectorEvaluationCompletedNative, const FGuid&, EPRDirectorEvaluationResult, EPRDirectorRuleOperationResult, const FPRDirectorResponse&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPRDirectorAppliedRuleChangedNative, EPRDirectorRuleOperationResult, const FPRAppliedDirectorRuleHandle&);
