// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Account/PRAccountIdentityRegistryDataAsset.h"

#include "Misc/DataValidation.h"
#include "Roguelike/Account/PRAccountIdentityDataAsset.h"

namespace PRAccountIdentityRegistryPrivate
{
const TCHAR* const ExpectedNames[] = { TEXT("Blank"), TEXT("Exile"), TEXT("Observer"), TEXT("Security"), TEXT("Technician") };
}

FPrimaryAssetId UPRAccountIdentityRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRAccountIdentityRegistry")), GetFName());
}

const UPRAccountIdentityDataAsset* UPRAccountIdentityRegistryDataAsset::FindIdentity(const FPrimaryAssetId IdentityId) const
{
	for (const TSoftObjectPtr<UPRAccountIdentityDataAsset>& Reference : Identities)
	{
		const UPRAccountIdentityDataAsset* Asset = Reference.LoadSynchronous();
		if (Asset && Asset->GetPrimaryAssetId() == IdentityId) return Asset;
	}
	return nullptr;
}

bool UPRAccountIdentityRegistryDataAsset::IsRegistryReady() const
{
	if (Identities.Num() != UE_ARRAY_COUNT(PRAccountIdentityRegistryPrivate::ExpectedNames)) return false;
	FString Previous;
	for (int32 Index = 0; Index < Identities.Num(); ++Index)
	{
		const UPRAccountIdentityDataAsset* Asset = Identities[Index].LoadSynchronous();
		if (!Asset || !Asset->IsIdentityDefinitionValid()) return false;
		const FPrimaryAssetId Id = Asset->GetPrimaryAssetId();
		if (Id.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRAccountIdentity"))
			|| Id.PrimaryAssetName.ToString() != PRAccountIdentityRegistryPrivate::ExpectedNames[Index]) return false;
		const FString Current = Id.ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current;
	}
	return true;
}

EDataValidationResult UPRAccountIdentityRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsRegistryReady())
	{
		Context.AddError(FText::FromString(TEXT("Account identity registry must contain exactly the five fixed, sorted identity assets.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
