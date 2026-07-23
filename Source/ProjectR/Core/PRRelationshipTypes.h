// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRRelationshipTypes.generated.h"

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRelationshipState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 Trust = 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 Affection = 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 Evaluation = 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 Overload = 0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionRelationshipRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	FGameplayTag CompanionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	FPRRelationshipState State;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRelationshipDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	FGameplayTag CompanionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 TrustDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 AffectionDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 EvaluationDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	int32 OverloadDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProjectR|Relationship")
	FName SourceId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRelationshipChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Relationship")
	FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Relationship")
	FPRRelationshipState PreviousState;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Relationship")
	FPRRelationshipState CurrentState;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Relationship")
	FPRRelationshipDelta AppliedDelta;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Relationship")
	double WorldTimeSeconds = 0.0;
};

/** Core-only canonical relationship rules shared by Save and Companion runtime code. */
class PROJECTR_API FPRCompanionContract
{
public:
	static constexpr int32 MinimumRelationshipValue = 0;
	static constexpr int32 MaximumRelationshipValue = 100;

	static const FGameplayTag& AxiomTag();
	static const FGameplayTag& KindleTag();
	static const FGameplayTag& NullTag();
	static const TArray<FGameplayTag>& GetCanonicalCompanionIds();
	static TArray<FPRCompanionRelationshipRecord> BuildDefaultRelationshipRecords();
	static bool AreCanonicalRelationshipRecords(const TArray<FPRCompanionRelationshipRecord>& Records);
	static bool ApplyDelta(FPRRelationshipState& InOutState, const FPRRelationshipDelta& Delta);
	static bool IsStateInRange(const FPRRelationshipState& State);
};
