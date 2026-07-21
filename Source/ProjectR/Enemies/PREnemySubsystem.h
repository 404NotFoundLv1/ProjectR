// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/PREnemyTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PREnemySubsystem.generated.h"

class APREnemyCharacter;
class APREnemyProjectile;
class UPREnemyAttackDataAsset;
class UPREnemyPrototypeDataAsset;
class UPREnemyPrototypeRegistryDataAsset;
class UNiagaraSystem;
class USoundBase;

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
	bool IsRegistryReady() const;
	FPREnemyStateChangedNative& OnEnemyStateChanged();
	FSimpleMulticastDelegate& OnRegistryReady();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	friend class UPREnemyBrainComponent;

	struct FLoadedPrototype
	{
		TObjectPtr<UPREnemyPrototypeDataAsset> Prototype = nullptr;
		TSubclassOf<APREnemyCharacter> EnemyClass;
		TArray<TSubclassOf<APREnemyProjectile>> PreloadedProjectileClasses;
		TMap<FSoftObjectPath, TObjectPtr<UNiagaraSystem>> PreloadedAttackVFX;
		TMap<FSoftObjectPath, TObjectPtr<USoundBase>> PreloadedAttackSFX;
	};

	void LoadFixedRegistry();
	void FinishRegistryLoad();
	TSubclassOf<APREnemyProjectile> GetPreloadedProjectileClass(
		const APREnemyCharacter* Enemy,
		const UPREnemyAttackDataAsset* Attack) const;
	bool IsValidSpawnTransform(const FTransform& SpawnTransform) const;
	void BroadcastState(APREnemyCharacter* Enemy) const;

	TSoftObjectPtr<UPREnemyPrototypeRegistryDataAsset> RegistryAsset;
	TMap<FGameplayTag, FLoadedPrototype> LoadedPrototypes;
	TMap<FGuid, TWeakObjectPtr<APREnemyCharacter>> SpawnedEnemies;
	bool bRegistryReady = false;
	FPREnemyStateChangedNative EnemyStateChanged;
	FSimpleMulticastDelegate RegistryReady;
	TSet<FName> PresentationWarningKeys;
};
