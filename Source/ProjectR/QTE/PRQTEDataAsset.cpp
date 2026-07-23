// Copyright Epic Games, Inc. All Rights Reserved.

#include "QTE/PRQTEDataAsset.h"

#include "Companions/PRCompanionTypes.h"
#include "Core/PRTagLibrary.h"
#include "Misc/DataValidation.h"

namespace
{
FGameplayTag GetQTETypeTag(const TCHAR* TagName)
{
	return FGameplayTag::RequestGameplayTag(FName(TagName));
}
}

const TArray<FName>& FPRQTEContract::GetExpectedQTEIds()
{
	static const TArray<FName> Ids = {
		TEXT("Axiom_WeaknessAxiom"), TEXT("Axiom_ProbabilityShield"), TEXT("Axiom_PhaseCut"), TEXT("Axiom_CooperativeRefutation"),
		TEXT("Kindle_Breakout"), TEXT("Kindle_BurnoutTornado"), TEXT("Kindle_DetonationRelay"), TEXT("Kindle_ReverseBurnRescue"),
		TEXT("Null_VulnerabilityBackstab"), TEXT("Null_DecoyJudgment"), TEXT("Null_GarbageCollection"), TEXT("Null_VerdictTamper")};
	return Ids;
}

int32 FPRQTEContract::GetTypePriority(const FGameplayTag& QTEType)
{
	if (QTEType.MatchesTagExact(GetQTETypeTag(TEXT("QTE.Type.Rescue")))) return 5;
	if (QTEType.MatchesTagExact(GetQTETypeTag(TEXT("QTE.Type.Defense")))) return 4;
	if (QTEType.MatchesTagExact(GetQTETypeTag(TEXT("QTE.Type.RuleCounter")))) return 3;
	if (QTEType.MatchesTagExact(GetQTETypeTag(TEXT("QTE.Type.Control")))) return 2;
	if (QTEType.MatchesTagExact(GetQTETypeTag(TEXT("QTE.Type.Attack")))) return 1;
	return 0;
}

bool FPRQTEContract::IsExpectedQTEId(const FName QTEId)
{
	return GetExpectedQTEIds().Contains(QTEId);
}

float FPRQTEContract::CalculateEffectiveCooldown(const float BaseCooldownSeconds, const int32 Overload)
{
	return FMath::Max(0.01f, BaseCooldownSeconds) * (1.0f + FMath::Clamp(static_cast<float>(Overload), 0.0f, 100.0f) / 100.0f);
}

FPrimaryAssetId UPRQTEDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("QTE"), QTEId);
}

#if WITH_EDITOR
EDataValidationResult UPRQTEDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	const bool bKnownCompanion = FPRCompanionContract::GetCanonicalCompanionIds().ContainsByPredicate(
		[this](const FGameplayTag Candidate) { return Candidate.MatchesTagExact(CompanionId); });
	const bool bKnownType = FPRQTEContract::GetTypePriority(QTEType) > 0;
	bool bValid = FPRQTEContract::IsExpectedQTEId(QTEId)
		&& bKnownCompanion
		&& bKnownType
		&& !DisplayName.IsEmpty()
		&& !PromptText.IsEmpty()
		&& AcceptedInputTags.Num() > 0
		&& SuccessEffects.Num() == 1
		&& FMath::IsFinite(WindowSeconds) && WindowSeconds > 0.0f
		&& FMath::IsFinite(PerfectWindowFraction) && PerfectWindowFraction >= 0.0f && PerfectWindowFraction <= 1.0f
		&& FMath::IsFinite(BaseCooldownSeconds) && BaseCooldownSeconds >= 0.0f
		&& FMath::IsFinite(TriggerHealthFraction) && TriggerHealthFraction >= 0.0f && TriggerHealthFraction <= 1.0f
		&& RequiredDistinctTargetCount >= 0
		&& MinimumTrust >= 0 && MinimumTrust <= 100
		&& MaximumOverload >= 0 && MaximumOverload <= 100
		&& static_cast<uint8>(TriggerSource) <= static_cast<uint8>(EPRQTETriggerSource::CompanionSupportEvent)
		&& static_cast<uint8>(TriggerKind) <= static_cast<uint8>(EPRQTETriggerKind::NormalLowHealth)
		&& static_cast<uint8>(TriggerTargetScope) <= static_cast<uint8>(EPRQTETargetScope::NormalEnemy);
	for (const FGameplayTag& InputTag : AcceptedInputTags)
	{
		bValid &= InputTag.IsValid() && InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input")));
	}
	for (const FPRQTEEffectDefinition& Effect : SuccessEffects)
	{
		bValid &= static_cast<uint8>(Effect.EffectKind) <= static_cast<uint8>(EPRQTEEffectKind::ConfirmBasicAttackKill)
			&& static_cast<uint8>(Effect.TargetScope) <= static_cast<uint8>(EPRQTETargetScope::NormalEnemy)
			&& FMath::IsFinite(Effect.Magnitude) && Effect.Magnitude >= 0.0f
			&& FMath::IsFinite(Effect.Range) && Effect.Range >= 0.0f
			&& FMath::IsFinite(Effect.PulseIntervalSeconds) && Effect.PulseIntervalSeconds >= 0.0f
			&& Effect.MaxTargets >= 0 && Effect.PulseCount >= 0;
	}
	if (!bValid)
	{
		Context.AddError(FText::FromString(TEXT("QTE asset violates the fixed v0.3.2 schema.")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif
