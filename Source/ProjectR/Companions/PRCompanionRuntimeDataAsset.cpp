// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionRuntimeDataAsset.h"

#include "Companions/PRCompanionTypes.h"
#include "Misc/DataValidation.h"

namespace PRCompanionRuntimeDataValidation
{
struct FExpectedSchema
{
	FGameplayTag Id;
	EPRCompanionSupportType SupportType;
	float Interval;
	float Range;
	float Magnitude;
	const TCHAR* PawnClassPath;
	const TCHAR* EffectClassPath;
};

const TArray<FExpectedSchema>& GetExpectedSchemas()
{
	// GameplayTags are guaranteed ready by the first Data Validation request, but not during module static initialization.
	static const TArray<FExpectedSchema> ExpectedSchemas = {
		{ FPRCompanionContract::AxiomTag(), EPRCompanionSupportType::Shield, 8.0f, 0.0f, 20.0f,
			TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Axiom.BP_Companion_Axiom_C"),
			TEXT("/Game/ProjectR/Companions/Effects/GE_Companion_Axiom_Shield.GE_Companion_Axiom_Shield_C") },
		{ FPRCompanionContract::KindleTag(), EPRCompanionSupportType::LightDamage, 4.0f, 800.0f, 12.0f,
			TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Kindle.BP_Companion_Kindle_C"), nullptr },
		{ FPRCompanionContract::NullTag(), EPRCompanionSupportType::ControlMark, 6.0f, 700.0f, 0.0f,
			TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Null.BP_Companion_Null_C"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C") }
	};
	return ExpectedSchemas;
}

const FExpectedSchema* FindExpectedSchema(const FGameplayTag Id)
{
	for (const FExpectedSchema& Schema : GetExpectedSchemas())
	{
		if (Schema.Id.MatchesTagExact(Id)) return &Schema;
	}
	return nullptr;
}
}

FPrimaryAssetId UPRCompanionRuntimeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CompanionRuntime"), CompanionId.GetTagName());
}

#if WITH_EDITOR
EDataValidationResult UPRCompanionRuntimeDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const TArray<FGameplayTag>& CanonicalIds = FPRCompanionContract::GetCanonicalCompanionIds();
	if (!CanonicalIds.ContainsByPredicate([this](const FGameplayTag Candidate) { return Candidate.MatchesTagExact(CompanionId); }))
	{
		Context.AddError(FText::FromString(TEXT("Companion runtime configuration must use a canonical CompanionId.")));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(BaseSupportInterval) || BaseSupportInterval <= 0.0f || !FMath::IsFinite(SupportRange)
		|| SupportRange < 0.0f || !FMath::IsFinite(SupportMagnitude) || SupportMagnitude < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("Companion runtime support values must be finite and non-negative.")));
		Result = EDataValidationResult::Invalid;
	}
	const PRCompanionRuntimeDataValidation::FExpectedSchema* Expected = PRCompanionRuntimeDataValidation::FindExpectedSchema(CompanionId);
	const FString PawnClassPath = PawnClass.ToSoftObjectPath().ToString();
	const FString EffectClassPath = SupportEffectClass ? SupportEffectClass->GetPathName() : FString();
	if (!Expected || SupportType != Expected->SupportType
		|| !FMath::IsNearlyEqual(BaseSupportInterval, Expected->Interval)
		|| !FMath::IsNearlyEqual(SupportRange, Expected->Range)
		|| !FMath::IsNearlyEqual(SupportMagnitude, Expected->Magnitude)
		|| PawnClassPath != Expected->PawnClassPath
		|| (Expected->EffectClassPath ? EffectClassPath != Expected->EffectClassPath : !EffectClassPath.IsEmpty()))
	{
		Context.AddError(FText::FromString(TEXT("Companion runtime configuration must match its fixed v0.3.1 manifest schema.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
