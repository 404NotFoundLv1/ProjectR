// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealityHub/PRRealityHubTerminalDataAsset.h"

#include "Misc/DataValidation.h"

FPrimaryAssetId UPRRealityHubTerminalDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRealityHubTerminal")), GetFName());
}

bool UPRRealityHubTerminalDataAsset::IsTerminalDefinitionValid() const
{
	return Terminal != EPRRealityHubTerminal::None && !DisplayName.IsEmpty() && !Description.IsEmpty();
}

EDataValidationResult UPRRealityHubTerminalDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsTerminalDefinitionValid())
	{
		Context.AddError(FText::FromString(TEXT("Reality Hub terminal requires a fixed terminal id and non-empty display metadata.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
