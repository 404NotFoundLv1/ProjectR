// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/PRCompanionDialogueRegistryDataAsset.h"

FPrimaryAssetId UPRCompanionDialogueRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CompanionDialogueRegistry"), GetFName());
}

bool UPRCompanionDialogueRegistryDataAsset::ValidateRegistry(FString& OutError) const
{
	if (Axiom.IsNull() || Kindle.IsNull() || Null.IsNull() || WidgetClass.IsNull())
	{
		OutError = TEXT("Dialogue registry requires the three fixed companion assets and the dialogue widget class.");
		return false;
	}
	return true;
}
