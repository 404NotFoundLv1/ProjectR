// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealityHub/PRRealityHubTerminalRegistryDataAsset.h"

#include "Director/PRDirectorRuleDataAsset.h"
#include "Director/PRDirectorRuleRegistryDataAsset.h"
#include "Misc/DataValidation.h"
#include "RealityHub/PRRealityHubTerminalDataAsset.h"
#include "RealityHub/PRRealityHubTypes.h"

FPrimaryAssetId UPRRealityHubTerminalRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRealityHubRegistry")), GetFName());
}

const UPRRealityHubTerminalDataAsset* UPRRealityHubTerminalRegistryDataAsset::FindTerminal(const EPRRealityHubTerminal Terminal) const
{
	for (const TSoftObjectPtr<UPRRealityHubTerminalDataAsset>& Reference : Terminals)
	{
		const UPRRealityHubTerminalDataAsset* Asset = Reference.LoadSynchronous();
		if (Asset && Asset->Terminal == Terminal) return Asset;
	}
	return nullptr;
}

bool UPRRealityHubTerminalRegistryDataAsset::IsRegistryReady() const
{
	if (Terminals.Num() != 5 || DirectorRuleRegistry.IsNull()) return false;
	for (uint8 Value = static_cast<uint8>(EPRRealityHubTerminal::CassetteSlot); Value <= static_cast<uint8>(EPRRealityHubTerminal::DirectorForecaster); ++Value)
	{
		const UPRRealityHubTerminalDataAsset* Asset = FindTerminal(static_cast<EPRRealityHubTerminal>(Value));
		if (!Asset || !Asset->IsTerminalDefinitionValid()) return false;
	}
	const UPRDirectorRuleRegistryDataAsset* Rules = DirectorRuleRegistry.LoadSynchronous();
	return Rules && Rules->IsRegistryReady();
}

void UPRRealityHubTerminalRegistryDataAsset::GetForecastRuleIds(TArray<FGameplayTag>& OutRuleIds) const
{
	OutRuleIds.Reset();
	const UPRDirectorRuleRegistryDataAsset* Rules = DirectorRuleRegistry.LoadSynchronous();
	if (!Rules || !Rules->IsRegistryReady()) return;
	for (const TSoftObjectPtr<UPRDirectorRuleDataAsset>& Reference : Rules->Rules)
	{
		const UPRDirectorRuleDataAsset* Rule = Reference.LoadSynchronous();
		if (Rule && Rule->IsRuleDefinitionValid()) OutRuleIds.Add(Rule->RuleId);
	}
}

EDataValidationResult UPRRealityHubTerminalRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsRegistryReady())
	{
		Context.AddError(FText::FromString(TEXT("Reality Hub registry must contain exactly five unique fixed terminals and a ready Director registry.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
