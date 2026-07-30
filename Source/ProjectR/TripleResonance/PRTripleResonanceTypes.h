// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Headmind/PRHeadmindTypes.h"
#include "Core/PRRelationshipTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRTripleResonanceTypes.generated.h"

UENUM(BlueprintType)
enum class EPRTripleResonanceState : uint8
{
	Unavailable,
	Ready,
	QTEActive,
	AbilityPending,
	Executing,
	Resolved,
	Failed,
	ReadyToRetry
};

UENUM(BlueprintType)
enum class EPRTripleResonanceOperationResult : uint8
{
	Invalid,
	Rejected,
	Started,
	Succeeded,
	Failed,
	ReadyToRetry
};

UENUM(BlueprintType)
enum class EPRTripleResonanceStep : uint8
{
	None,
	Axiom,
	Kindle,
	Null
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceEligibilitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") bool bEligible = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FName FailureReason;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid RunId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid AccountId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid BossSpawnId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FName WorldId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") EPRTripleResonanceState State = EPRTripleResonanceState::Unavailable;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") EPRTripleResonanceStep ActiveStep = EPRTripleResonanceStep::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FPRTripleResonanceEligibilitySnapshot Eligibility;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") float RemainingSeconds = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FName FailureReason;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceExecutionResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid RunId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid AccountId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGuid BossSpawnId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") bool bBasiliskCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") bool bExecuted = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") float AppliedDamage = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceLegacySnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") bool bHasSkillMemory = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FGameplayTag AbilityTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") bool bHasHighRiskProof = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRTripleResonanceOperationEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") EPRTripleResonanceOperationResult Result = EPRTripleResonanceOperationResult::Invalid;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|TripleResonance") FName Reason;
};

struct PROJECTR_API FPRTripleResonanceQTEStepDefinition
{
	FName QTEId;
	FGameplayTag CompanionId;
	FGameplayTag QTEType;
	FGameplayTag AcceptedInputTag;
	float WindowSeconds = 0.0f;
};

/** Value-only eligibility input used by the subsystem after it has consumed frozen public snapshots. */
struct PROJECTR_API FPRTripleResonanceEligibilityInput
{
	FPRTripleResonanceOpportunitySnapshot Opportunity;
	TArray<FPRCompanionRelationshipRecord> Relationships;
	bool bHasAuditorPrerequisite = false;
	bool bHasFrozenRunEntitlement = false;
	bool bHasExactHeadmindIdentity = false;
	FGuid RunId;
	FGuid AccountId;
	FGuid BossSpawnId;
	FName WorldId;
};

struct PROJECTR_API FPRTripleResonanceEligibilityRules
{
	static FPRTripleResonanceEligibilitySnapshot Evaluate(const FPRTripleResonanceEligibilityInput& Input)
	{
		FPRTripleResonanceEligibilitySnapshot Result;
		Result.RunId = Input.RunId;
		Result.AccountId = Input.AccountId;
		Result.BossSpawnId = Input.BossSpawnId;
		Result.WorldId = Input.WorldId;
		if (Input.Opportunity.State != EPRTripleResonanceOpportunityState::EligibleDeferredToV072 || !Input.Opportunity.bWindowActive)
		{
			Result.FailureReason = TEXT("TripleResonance.WindowUnavailable"); return Result;
		}
		if (!Input.bHasAuditorPrerequisite)
		{
			Result.FailureReason = TEXT("TripleResonance.AuditorPrerequisiteMissing"); return Result;
		}
		if (!Input.bHasFrozenRunEntitlement)
		{
			Result.FailureReason = TEXT("TripleResonance.EntitlementMissing"); return Result;
		}
		if (!Input.bHasExactHeadmindIdentity || !Input.RunId.IsValid() || !Input.AccountId.IsValid() || !Input.BossSpawnId.IsValid() || Input.WorldId.IsNone())
		{
			Result.FailureReason = TEXT("TripleResonance.IdentityMismatch"); return Result;
		}
		for (const FGameplayTag& CompanionId : FPRCompanionContract::GetCanonicalCompanionIds())
		{
			const FPRCompanionRelationshipRecord* Record = Input.Relationships.FindByPredicate([CompanionId](const FPRCompanionRelationshipRecord& Candidate)
			{
				return Candidate.CompanionId.MatchesTagExact(CompanionId);
			});
			if (!Record)
			{
				Result.FailureReason = TEXT("TripleResonance.CompanionUnavailable"); return Result;
			}
			if (Record->State.Trust < 70)
			{
				Result.FailureReason = TEXT("TripleResonance.CompanionTrustInsufficient"); return Result;
			}
			if (Record->State.Overload != 0)
			{
				Result.FailureReason = TEXT("TripleResonance.CompanionOverload"); return Result;
			}
		}
		Result.bEligible = true;
		return Result;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRTripleResonanceStateChangedNative, const FPRTripleResonanceSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRTripleResonanceResultNative, const FPRTripleResonanceExecutionResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRTripleResonanceOperationNative, const FPRTripleResonanceOperationEvent&);

/** Compiled-in identifiers for the three result-only external QTEs; the P0 registry remains unchanged. */
struct PROJECTR_API FPRTripleResonanceContract
{
	static const TArray<FName>& GetExternalQTEIds()
	{
		static const TArray<FName> Ids = {TEXT("TripleResonance_Axiom"), TEXT("TripleResonance_Kindle"), TEXT("TripleResonance_Null")};
		return Ids;
	}

	static bool IsExternalQTEId(const FName Id)
	{
		return GetExternalQTEIds().Contains(Id);
	}

	static bool IsP0SkillTag(const FGameplayTag& AbilityTag)
	{
		static const TArray<FGameplayTag> P0 = {
			FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust")), FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash")),
			FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop")), FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge")),
			FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook")), FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"))};
		return P0.Contains(AbilityTag);
	}

	static FPRTripleResonanceQTEStepDefinition GetStepDefinition(const EPRTripleResonanceStep Step)
	{
		FPRTripleResonanceQTEStepDefinition Definition;
		Definition.WindowSeconds = 1.25f;
		switch (Step)
		{
		case EPRTripleResonanceStep::Axiom:
			Definition.QTEId = TEXT("TripleResonance_Axiom");
			Definition.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"));
			Definition.QTEType = FGameplayTag::RequestGameplayTag(TEXT("QTE.Type.RuleCounter"));
			Definition.AcceptedInputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Interact"));
			break;
		case EPRTripleResonanceStep::Kindle:
			Definition.QTEId = TEXT("TripleResonance_Kindle");
			Definition.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"));
			Definition.QTEType = FGameplayTag::RequestGameplayTag(TEXT("QTE.Type.Attack"));
			Definition.AcceptedInputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Attack"));
			break;
		case EPRTripleResonanceStep::Null:
			Definition.QTEId = TEXT("TripleResonance_Null");
			Definition.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"));
			Definition.QTEType = FGameplayTag::RequestGameplayTag(TEXT("QTE.Type.Control"));
			Definition.AcceptedInputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Execute"));
			break;
		default:
			Definition.WindowSeconds = 0.0f;
			break;
		}
		return Definition;
	}

	static FPRTripleResonanceQTEStepDefinition GetStepDefinition(const FName QTEId)
	{
		for (const EPRTripleResonanceStep Step : {EPRTripleResonanceStep::Axiom, EPRTripleResonanceStep::Kindle, EPRTripleResonanceStep::Null})
		{
			const FPRTripleResonanceQTEStepDefinition Definition = GetStepDefinition(Step);
			if (Definition.QTEId == QTEId) return Definition;
		}
		return FPRTripleResonanceQTEStepDefinition();
	}
};
