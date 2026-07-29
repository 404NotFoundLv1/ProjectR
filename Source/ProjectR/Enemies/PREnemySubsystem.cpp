// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemySubsystem.h"

#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyContentRegistryDataAsset.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyProjectile.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "ProjectR.h"
#include "Sound/SoundBase.h"

namespace PREnemyRegistry
{
const FSoftObjectPath RegistryPath(TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
}

void UPREnemySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Authoring worlds must never load or instantiate enemy runtime assets.
	// The fixed registry is only meaningful in a Game/PIE world, where spawning
	// is authoritative and its async completion has a valid gameplay lifecycle.
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	RegistryAsset = TSoftObjectPtr<UPREnemyPrototypeRegistryDataAsset>(PREnemyRegistry::RegistryPath);
	LoadFixedRegistry();
}

void UPREnemySubsystem::Deinitialize()
{
	for (TPair<FGuid, TWeakObjectPtr<APREnemyCharacter>>& Pair : SpawnedEnemies)
	{
		if (APREnemyCharacter* Enemy = Pair.Value.Get())
		{
			if (UPREnemyBrainComponent* Brain = Enemy->GetEnemyBrain()) Brain->StopBrain();
		}
	}
	SpawnedEnemies.Empty();
	LoadedPrototypes.Empty();
	LoadedContentPrototypes.Empty();
	ContentRegistry = nullptr;
	ConfiguredContentRegistryId = FPrimaryAssetId();
	bRegistryReady = false;
	Super::Deinitialize();
}

void UPREnemySubsystem::LoadFixedRegistry()
{
	if (RegistryAsset.IsNull()) return;
	UAssetManager::GetStreamableManager().RequestAsyncLoad(RegistryAsset.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UPREnemySubsystem::FinishRegistryLoad));
}

void UPREnemySubsystem::FinishRegistryLoad()
{
	UPREnemyPrototypeRegistryDataAsset* Registry = RegistryAsset.Get();
	if (!Registry) return;
	for (const FPREnemyPrototypeRegistryEntry& Entry : Registry->GetEntries())
	{
		// The data asset is the only whitelist. No class, path, or UObject enters SpawnEnemyPrototype from callers.
		if (!Entry.PrototypeTag.IsValid() || Entry.Prototype.IsNull() || Entry.EnemyClass.IsNull()) continue;
		UPREnemyPrototypeDataAsset* Prototype = Entry.Prototype.LoadSynchronous();
		UClass* LoadedClass = Entry.EnemyClass.LoadSynchronous();
		if (Prototype && LoadedClass && LoadedClass->IsChildOf(APREnemyCharacter::StaticClass()))
		{
			FLoadedPrototype& Loaded = LoadedPrototypes.FindOrAdd(Entry.PrototypeTag);
			Loaded.Prototype = Prototype;
			Loaded.EnemyClass = LoadedClass;
			bool bProjectileConfigurationValid = true;
			for (const TObjectPtr<UPREnemyAttackDataAsset>& Attack : Prototype->AttackDefinitions)
			{
				if (!Attack || Attack->Kind != EPREnemyAttackKind::Projectile)
				{
					continue;
				}
				UClass* ProjectileClass = Attack->ProjectileClass.LoadSynchronous();
				if (!ProjectileClass || !ProjectileClass->IsChildOf(APREnemyProjectile::StaticClass()))
				{
					bProjectileConfigurationValid = false;
					break;
				}
				Loaded.PreloadedProjectileClasses.Add(ProjectileClass);
			}
			if (!bProjectileConfigurationValid)
			{
				LoadedPrototypes.Remove(Entry.PrototypeTag);
			}
			else
			{
				for (const TObjectPtr<UPREnemyAttackDataAsset>& Attack : Prototype->AttackDefinitions)
				{
					if (!Attack) continue;
					if (UNiagaraSystem* VFX = Attack->VFX.LoadSynchronous())
					{
						Loaded.PreloadedAttackVFX.Add(Attack->VFX.ToSoftObjectPath(), VFX);
					}
					else if (const FName WarningKey(*FString::Printf(TEXT("VFX.%s"), *Attack->AttackId.ToString())); !PresentationWarningKeys.Contains(WarningKey))
					{
						PresentationWarningKeys.Add(WarningKey);
						UE_LOG(LogProjectR, Warning, TEXT("Enemy presentation VFX preload failed for %s; gameplay remains available."), *Attack->AttackId.ToString());
					}
					if (USoundBase* SFX = Attack->SFX.LoadSynchronous())
					{
						Loaded.PreloadedAttackSFX.Add(Attack->SFX.ToSoftObjectPath(), SFX);
					}
					else if (const FName WarningKey(*FString::Printf(TEXT("SFX.%s"), *Attack->AttackId.ToString())); !PresentationWarningKeys.Contains(WarningKey))
					{
						PresentationWarningKeys.Add(WarningKey);
						UE_LOG(LogProjectR, Warning, TEXT("Enemy presentation SFX preload failed for %s; gameplay remains available."), *Attack->AttackId.ToString());
					}
				}
			}
		}
	}
	// The fixed Registry asset is the complete whitelist. Runtime code must not
	// duplicate a list of allowed Enemy tags: every declared entry must load
	// successfully before callers can request one of those exact entries.
	bRegistryReady = Registry->GetEntries().Num() > 0
		&& LoadedPrototypes.Num() == Registry->GetEntries().Num();
	if (bRegistryReady)
	{
		RegistryReady.Broadcast();
	}
}

TSubclassOf<APREnemyProjectile> UPREnemySubsystem::GetPreloadedProjectileClass(
	const APREnemyCharacter* Enemy,
	const UPREnemyAttackDataAsset* Attack) const
{
	const UPREnemyPrototypeDataAsset* Prototype = Enemy ? Enemy->GetEnemyPrototype() : nullptr;
	const FLoadedPrototype* Loaded = Prototype ? LoadedPrototypes.Find(Prototype->PrototypeTag) : nullptr;
	if (!Loaded || !Attack || Attack->Kind != EPREnemyAttackKind::Projectile)
	{
		return nullptr;
	}
	const FSoftObjectPath RequestedPath = Attack->ProjectileClass.ToSoftObjectPath();
	for (const TSubclassOf<APREnemyProjectile> PreloadedClass : Loaded->PreloadedProjectileClasses)
	{
		if (PreloadedClass && FSoftObjectPath(PreloadedClass.Get()) == RequestedPath)
		{
			return PreloadedClass;
		}
	}
	return nullptr;
}

bool UPREnemySubsystem::IsValidSpawnTransform(const FTransform& SpawnTransform) const
{
	return SpawnTransform.IsValid() && !SpawnTransform.GetLocation().ContainsNaN()
		&& FMath::IsNearlyEqual(SpawnTransform.GetScale3D().X, 1.0f)
		&& FMath::IsNearlyEqual(SpawnTransform.GetScale3D().Y, 1.0f)
		&& FMath::IsNearlyEqual(SpawnTransform.GetScale3D().Z, 1.0f);
}

EPREnemySpawnStatus UPREnemySubsystem::SpawnEnemyPrototype(
	const FGameplayTag PrototypeTag,
	const FTransform& SpawnTransform,
	FGuid& OutSpawnId,
	APREnemyCharacter*& OutEnemy)
{
	OutSpawnId.Invalidate();
	OutEnemy = nullptr;
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return EPREnemySpawnStatus::NotAuthoritative;
	}
	if (!bRegistryReady) return EPREnemySpawnStatus::NotReady;
	if (!IsValidSpawnTransform(SpawnTransform)) return EPREnemySpawnStatus::InvalidTransform;
	const FLoadedPrototype* Entry = LoadedPrototypes.Find(PrototypeTag);
	if (!Entry) return EPREnemySpawnStatus::UnknownPrototype;
	OutSpawnId = FGuid::NewGuid();
	APREnemyCharacter* Enemy = GetWorld()->SpawnActorDeferred<APREnemyCharacter>(Entry->EnemyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy)
	{
		OutSpawnId.Invalidate();
		return EPREnemySpawnStatus::SpawnFailed;
	}
	Enemy->ConfigureSpawn(Entry->Prototype, OutSpawnId, SpawnTransform.GetLocation().Y);
	Enemy->FinishSpawning(SpawnTransform);
	// Deferred spawning does not guarantee that AutoPossessAI has completed before
	// FinishSpawning returns. Request the fixed controller only when auto-possession
	// has not already supplied one; a second possession would restart StateTree.
	if (!Enemy->GetController())
	{
		Enemy->SpawnDefaultController();
	}
	if (!Enemy->IsEnemyInitialized())
	{
		Enemy->Destroy();
		OutSpawnId.Invalidate();
		return EPREnemySpawnStatus::SpawnFailed;
	}
	SpawnedEnemies.Add(OutSpawnId, Enemy);
	OutEnemy = Enemy;
	BroadcastState(Enemy);
	return EPREnemySpawnStatus::Spawned;
}

EPREnemyContentResult UPREnemySubsystem::ConfigureContentRegistry(const FPrimaryAssetId RegistryId)
{
	if (!SpawnedEnemies.IsEmpty()) return EPREnemyContentResult::Busy;
	if (!RegistryId.IsValid() || RegistryId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectREnemyContentRegistry"))) return EPREnemyContentResult::InvalidRegistry;
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(RegistryId);
	UPREnemyContentRegistryDataAsset* Candidate = Path.IsValid() ? Cast<UPREnemyContentRegistryDataAsset>(Path.TryLoad()) : nullptr;
	if (!Candidate) return EPREnemyContentResult::NotFound;
	if (!Candidate->IsRegistryReady()) return EPREnemyContentResult::InvalidRegistry;
	TMap<FPrimaryAssetId, FLoadedPrototype> CandidateEntries;
	for (const FPREnemyContentRegistryEntry& Entry : Candidate->Entries)
	{
		UPREnemyPrototypeDataAsset* Prototype = Entry.Prototype.LoadSynchronous();
		UClass* EnemyClass = Entry.EnemyClass.LoadSynchronous();
		if (!Prototype || !EnemyClass || !EnemyClass->IsChildOf(APREnemyCharacter::StaticClass())) return EPREnemyContentResult::InvalidRegistry;
		FLoadedPrototype& Loaded = CandidateEntries.Add(Entry.PrototypeId);
		Loaded.Prototype = Prototype;
		Loaded.EnemyClass = EnemyClass;
	}
	ContentRegistry = Candidate;
	ConfiguredContentRegistryId = RegistryId;
	LoadedContentPrototypes = MoveTemp(CandidateEntries);
	return EPREnemyContentResult::Succeeded;
}

FPrimaryAssetId UPREnemySubsystem::GetConfiguredContentRegistryId() const
{
	return ConfiguredContentRegistryId;
}

EPREnemySpawnStatus UPREnemySubsystem::SpawnEnemyPrototype(
	const FPrimaryAssetId PrototypeId,
	const FTransform& SpawnTransform,
	FGuid& OutSpawnId,
	APREnemyCharacter*& OutEnemy)
{
	OutSpawnId.Invalidate();
	OutEnemy = nullptr;
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return EPREnemySpawnStatus::NotAuthoritative;
	if (!ContentRegistry || !ConfiguredContentRegistryId.IsValid()) return EPREnemySpawnStatus::NotReady;
	if (!IsValidSpawnTransform(SpawnTransform)) return EPREnemySpawnStatus::InvalidTransform;
	const FLoadedPrototype* Entry = LoadedContentPrototypes.Find(PrototypeId);
	if (!Entry) return EPREnemySpawnStatus::UnknownPrototype;
	OutSpawnId = FGuid::NewGuid();
	APREnemyCharacter* Enemy = GetWorld()->SpawnActorDeferred<APREnemyCharacter>(Entry->EnemyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy) { OutSpawnId.Invalidate(); return EPREnemySpawnStatus::SpawnFailed; }
	Enemy->ConfigureSpawn(Entry->Prototype, OutSpawnId, SpawnTransform.GetLocation().Y);
	Enemy->FinishSpawning(SpawnTransform);
	if (!Enemy->GetController()) Enemy->SpawnDefaultController();
	if (!Enemy->IsEnemyInitialized()) { Enemy->Destroy(); OutSpawnId.Invalidate(); return EPREnemySpawnStatus::SpawnFailed; }
	SpawnedEnemies.Add(OutSpawnId, Enemy);
	OutEnemy = Enemy;
	BroadcastState(Enemy);
	return EPREnemySpawnStatus::Spawned;
}

bool UPREnemySubsystem::GetEnemyRuntimeState(const FGuid SpawnId, FPREnemyRuntimeState& OutState) const
{
	OutState = FPREnemyRuntimeState();
	const TWeakObjectPtr<APREnemyCharacter>* Found = SpawnedEnemies.Find(SpawnId);
	if (!Found || !Found->IsValid()) return false;
	OutState = (*Found)->GetEnemyBrain()->GetRuntimeState();
	for (const TPair<FPrimaryAssetId, FLoadedPrototype>& Pair : LoadedContentPrototypes)
	{
		if (Pair.Value.Prototype == (*Found)->GetEnemyPrototype()) { OutState.PrototypeId = Pair.Key; break; }
	}
	return true;
}

bool UPREnemySubsystem::ResolveSpawnedEnemy(const FGuid SpawnId, APREnemyCharacter*& OutEnemy) const
{
	OutEnemy = nullptr;
	const TWeakObjectPtr<APREnemyCharacter>* Found = SpawnedEnemies.Find(SpawnId);
	if (!Found || !Found->IsValid()) return false;
	OutEnemy = Found->Get();
	return OutEnemy != nullptr;
}

bool UPREnemySubsystem::DespawnEnemy(const FGuid SpawnId)
{
	TWeakObjectPtr<APREnemyCharacter>* Found = SpawnedEnemies.Find(SpawnId);
	if (!Found) return false;
	if (APREnemyCharacter* Enemy = Found->Get()) Enemy->Destroy();
	SpawnedEnemies.Remove(SpawnId);
	return true;
}

bool UPREnemySubsystem::IsRegistryReady() const
{
	return bRegistryReady;
}

void UPREnemySubsystem::BroadcastState(APREnemyCharacter* Enemy) const
{
	if (Enemy && Enemy->GetEnemyBrain())
	{
		EnemyStateChanged.Broadcast(Enemy->GetEnemyBrain()->GetRuntimeState());
	}
}

FPREnemyStateChangedNative& UPREnemySubsystem::OnEnemyStateChanged()
{
	return EnemyStateChanged;
}

FSimpleMulticastDelegate& UPREnemySubsystem::OnRegistryReady()
{
	return RegistryReady;
}
