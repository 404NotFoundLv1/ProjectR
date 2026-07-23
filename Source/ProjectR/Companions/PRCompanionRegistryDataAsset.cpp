// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionRegistryDataAsset.h"

#include "Companions/PRCompanionDataAsset.h"
#include "Core/PRRelationshipTypes.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UPRCompanionRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRCompanionRegistry")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPRCompanionRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const TArray<FGameplayTag>& CanonicalIds = FPRCompanionContract::GetCanonicalCompanionIds();
	if (Companions.Num() != CanonicalIds.Num())
	{
		Context.AddError(FText::FromString(TEXT("Companion Registry must contain exactly the canonical three companions.")));
		return EDataValidationResult::Invalid;
	}
	for (int32 Index = 0; Index < Companions.Num(); ++Index)
	{
		const UPRCompanionDataAsset* Asset = Companions[Index].LoadSynchronous();
		if (!Asset || !Asset->CompanionId.MatchesTagExact(CanonicalIds[Index]))
		{
			Context.AddError(FText::FromString(TEXT("Companion Registry order must be Axiom, Kindle, Null.")));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

const TArray<TSoftObjectPtr<UPRCompanionDataAsset>>& UPRCompanionRegistryDataAsset::GetCompanions() const
{
	return Companions;
}

