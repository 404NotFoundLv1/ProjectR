// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRHeadmindTypes.generated.h"

UENUM(BlueprintType)
enum class EPRHeadmindRelationshipBand : uint8 { Distant, Connected, Resonant };
UENUM(BlueprintType)
enum class EPRHeadmindCounterproofBand : uint8 { None, Established, Abundant };
UENUM(BlueprintType)
enum class EPRHeadmindObedienceBand : uint8 { Unobserved, Contested, Accepted };
UENUM(BlueprintType)
enum class EPRTripleResonanceOpportunityState : uint8 { Unavailable, EligibleDeferredToV072, DegradedNoOp };
UENUM(BlueprintType)
enum class EPRHeadmindBossPhase : uint8 { Dormant, DirectiveFusion, BasiliskJudgment, Defeated };

USTRUCT(BlueprintType)
struct PROJECTR_API FPRHeadmindEndingInputSnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindRelationshipBand RelationshipBand = EPRHeadmindRelationshipBand::Distant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindCounterproofBand CounterproofBand = EPRHeadmindCounterproofBand::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindObedienceBand ObedienceBand = EPRHeadmindObedienceBand::Unobserved;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") bool bAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FName FallbackReason;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRHeadmindEndingResult
{
	GENERATED_BODY()
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FName EndingId = TEXT("Ending.Headmind.RewriteUnderlyingProposition");
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindRelationshipBand RelationshipBand = EPRHeadmindRelationshipBand::Distant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindCounterproofBand CounterproofBand = EPRHeadmindCounterproofBand::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindObedienceBand ObedienceBand = EPRHeadmindObedienceBand::Unobserved;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FName ParagraphId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") bool bAvailable = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceOpportunitySnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRTripleResonanceOpportunityState State = EPRTripleResonanceOpportunityState::Unavailable;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGameplayTag PredictedSkillTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FName FallbackReason;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") bool bWindowActive = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGuid FrozenRunId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGuid FrozenAccountId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGuid FrozenBossSpawnId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FName FrozenWorldId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRHeadmindBossRuntimeState
{
	GENERATED_BODY()
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") EPRHeadmindBossPhase Phase = EPRHeadmindBossPhase::Dormant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") int32 SynthesisPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGameplayTag PrimaryRuleId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FGameplayTag SecondaryRuleId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") bool bDirectiveFusionAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") bool bDegradedNoOp = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Headmind") FPRTripleResonanceOpportunitySnapshot TripleResonance;
};
