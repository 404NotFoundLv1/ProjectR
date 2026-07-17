// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/PREnemyTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PREnemySubsystem.generated.h"

class APREnemyCharacter;
class UPREnemyPrototypeDataAsset;
class UPREnemyPrototypeRegistryDataAsset;

UCLASS()
class PROJECTR_API UPREnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	EPREnemySpawnStatus SpawnEnemyPrototype(
		FGameplayTag PrototypeTag,
		const FTransform& SpawnTransform,
		FGuid& OutSpawnId,
		APREnemyCharacter*& OutEnemy);
	bool GetEnemyRuntimeState(FGuid SpawnId, FPREnemyRuntimeState& OutState) const;
	bool DespawnEnemy(FGuid SpawnId);
	FPREnemyStateChangedNative& OnEnemyStateChanged();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	struct FLoadedPrototype
	{
		TObjectPtr<UPREnemyPrototypeDataAsset> Prototype = nullptr;
		TSubclassOf<APREnemyCharacter> EnemyClass;
	};

	void LoadFixedRegistry();
	void FinishRegistryLoad();
	bool IsValidSpawnTransform(const FTransform& SpawnTransform) const;
	void BroadcastState(APREnemyCharacter* Enemy) const;

	TSoftObjectPtr<UPREnemyPrototypeRegistryDataAsset> RegistryAsset;
	TMap<FGameplayTag, FLoadedPrototype> LoadedPrototypes;
	TMap<FGuid, TWeakObjectPtr<APREnemyCharacter>> SpawnedEnemies;
	bool bRegistryReady = false;
	FPREnemyStateChangedNative EnemyStateChanged;
};
