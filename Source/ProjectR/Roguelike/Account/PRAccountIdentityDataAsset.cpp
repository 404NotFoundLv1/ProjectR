// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Account/PRAccountIdentityDataAsset.h"

#include "Misc/DataValidation.h"

FPrimaryAssetId UPRAccountIdentityDataAsset::GetPrimaryAssetId() const
{
	return IdentityId.IsValid() ? IdentityId : FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRAccountIdentity")), GetFName());
}

bool UPRAccountIdentityDataAsset::IsIdentityDefinitionValid() const
{
	return IdentityId.PrimaryAssetType == FPrimaryAssetType(TEXT("ProjectRAccountIdentity"))
		&& IdentityId.PrimaryAssetName != NAME_None && !DisplayName.IsEmpty() && !Advantage.IsEmpty()
		&& !Defect.IsEmpty() && !RecommendedPlaystyle.IsEmpty();
}

EDataValidationResult UPRAccountIdentityDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsIdentityDefinitionValid())
	{
		Context.AddError(FText::FromString(TEXT("Account identity must use a fixed ProjectRAccountIdentity id and non-empty metadata.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
