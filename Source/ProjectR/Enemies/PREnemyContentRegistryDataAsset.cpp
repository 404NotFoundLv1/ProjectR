// Copyright ProjectR. All Rights Reserved.

#include "Enemies/PREnemyContentRegistryDataAsset.h"

#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"

FPrimaryAssetId UPREnemyContentRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), GetFName());
}

bool UPREnemyContentRegistryDataAsset::IsRegistryReady() const
{
	if (Entries.Num() != 4) return false;
	FString Previous;
	for (const FPREnemyContentRegistryEntry& Entry : Entries)
	{
		UPREnemyPrototypeDataAsset* PrototypeAsset = Entry.Prototype.LoadSynchronous();
		UClass* EnemyClassAsset = Entry.EnemyClass.LoadSynchronous();
		const FString Current = Entry.PrototypeId.ToString();
		if (!Entry.PrototypeId.IsValid() || Entry.PrototypeId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectREnemy"))
			|| !PrototypeAsset || !EnemyClassAsset || !EnemyClassAsset->IsChildOf(APREnemyCharacter::StaticClass())
			|| (!Previous.IsEmpty() && Previous >= Current)) return false;
		Previous = Current;
	}
	return true;
}

const FPREnemyContentRegistryEntry* UPREnemyContentRegistryDataAsset::FindEntry(const FPrimaryAssetId PrototypeId) const
{
	return Entries.FindByPredicate([PrototypeId](const FPREnemyContentRegistryEntry& Entry) { return Entry.PrototypeId == PrototypeId; });
}
