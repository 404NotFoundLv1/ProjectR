// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionDataAsset.h"

#include "Misc/DataValidation.h"

FPrimaryAssetId UPRCompanionDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRCompanion")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPRCompanionDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!CompanionId.IsValid() || !CompanionId.ToString().StartsWith(TEXT("Companion."))
		|| DisplayName.IsEmpty() || RoleLabel.IsEmpty() || PersonaSummary.IsEmpty())
	{
		Context.AddError(FText::FromString(TEXT("Companion requires a Companion.* id and non-empty presentation metadata.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

