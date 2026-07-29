// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PREncounterDataAsset.h"

FPrimaryAssetId UPREncounterDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectREncounter")), GetFName());
}

bool UPREncounterDataAsset::IsEncounterDefinitionValid() const
{
	if (!EncounterId.IsValid() || EncounterId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectREncounter")) || EncounterId != GetPrimaryAssetId() || Kind == EPRRoomEncounterKind::None)
	{
		return false;
	}
	if (Kind == EPRRoomEncounterKind::Boss)
	{
		return ExpectedBossId.IsValid() && ExpectedBossId.ToString().StartsWith(TEXT("Enemy.Type."));
	}
	if (SpawnDefinitions.IsEmpty()) return false;
	for (const FPREncounterSpawnDefinition& Spawn : SpawnDefinitions)
	{
		const bool bTagSelector = Spawn.PrototypeTag.IsValid() && Spawn.PrototypeTag.ToString().StartsWith(TEXT("Enemy."));
		const bool bIdSelector = Spawn.PrototypeId.IsValid() && Spawn.PrototypeId.PrimaryAssetType == FPrimaryAssetType(TEXT("ProjectREnemy"));
		if (bTagSelector == bIdSelector) return false;
	}
	return true;
}
