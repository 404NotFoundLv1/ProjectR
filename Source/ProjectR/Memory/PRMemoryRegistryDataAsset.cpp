// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRMemoryRegistryDataAsset.h"
#include "Memory/PRMemoryPersonaDataAsset.h"

FPrimaryAssetId UPRMemoryRegistryDataAsset::GetPrimaryAssetId() const { return FPrimaryAssetId(TEXT("ProjectRMemoryRegistry"), GetFName()); }
bool UPRMemoryRegistryDataAsset::IsRegistryReady() const
{
	static const FName Expected[] = { TEXT("Axiom"), TEXT("Kindle"), TEXT("Null") };
	if (Personas.Num() != UE_ARRAY_COUNT(Expected)) return false;
	for (int32 Index = 0; Index < Personas.Num(); ++Index)
	{
		const UPRMemoryPersonaDataAsset* Persona = Personas[Index].LoadSynchronous();
		if (!Persona || !Persona->IsDefinitionValid() || Persona->ProviderCompanionId != Expected[Index]) return false;
	}
	return true;
}
const UPRMemoryPersonaDataAsset* UPRMemoryRegistryDataAsset::FindPersona(const FName ProviderCompanionId) const
{
	for (const TSoftObjectPtr<UPRMemoryPersonaDataAsset>& Ref : Personas)
	{
		const UPRMemoryPersonaDataAsset* Persona = Ref.LoadSynchronous();
		if (Persona && Persona->ProviderCompanionId == ProviderCompanionId) return Persona;
	}
	return nullptr;
}
