// Copyright Epic Games, Inc. All Rights Reserved.

#include "QTE/PRQTERegistryDataAsset.h"

#include "Misc/DataValidation.h"
#include "QTE/PRQTEDataAsset.h"

FPrimaryAssetId UPRQTERegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("QTERegistry"), TEXT("Default"));
}

#if WITH_EDITOR
EDataValidationResult UPRQTERegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	if (Entries.Num() != FPRQTEContract::GetExpectedQTEIds().Num() || PromptWidgetClass.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("QTE Registry must contain twelve entries and the prompt widget.")));
		return EDataValidationResult::Invalid;
	}
	TSet<FName> Seen;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		UPRQTEDataAsset* Asset = Entries[Index].LoadSynchronous();
		if (!Asset || !Asset->QTEId.IsEqual(FPRQTEContract::GetExpectedQTEIds()[Index]) || Seen.Contains(Asset->QTEId))
		{
			Context.AddError(FText::FromString(TEXT("QTE Registry entries must be canonical, unique, and ordered.")));
			return EDataValidationResult::Invalid;
		}
		Seen.Add(Asset->QTEId);
	}
	return EDataValidationResult::Valid;
}
#endif
