// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/Bosses/PRBossPrototypeGameMode.h"

#include "Enemies/PREnemySubsystem.h"
#include "ProjectR.h"

void APRBossPrototypeGameMode::BeginPlay()
{
	Super::BeginPlay();
	UPREnemySubsystem* Enemies = GetWorld() ? GetWorld()->GetSubsystem<UPREnemySubsystem>() : nullptr;
	if (!Enemies)
	{
		UE_LOG(LogProjectR, Error, TEXT("Boss prototype start rejected: Enemy registry subsystem was unavailable."));
		return;
	}
	if (Enemies->IsRegistryReady())
	{
		SpawnAuditorWhenRegistryReady();
		return;
	}
	RegistryReadyHandle = Enemies->OnRegistryReady().AddUObject(this, &APRBossPrototypeGameMode::SpawnAuditorWhenRegistryReady);
}

void APRBossPrototypeGameMode::SpawnAuditorWhenRegistryReady()
{
	UPREnemySubsystem* Enemies = GetWorld() ? GetWorld()->GetSubsystem<UPREnemySubsystem>() : nullptr;
	if (!Enemies || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}
	Enemies->OnRegistryReady().Remove(RegistryReadyHandle);
	FGuid SpawnId;
	APREnemyCharacter* Boss = nullptr;
	const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 200.0f));
	if (Enemies->SpawnEnemyPrototype(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.AuditorBoss")), SpawnTransform, SpawnId, Boss)
		!= EPREnemySpawnStatus::Spawned)
	{
		UE_LOG(LogProjectR, Error, TEXT("Boss prototype start failed: fixed Auditor registry spawn was rejected."));
	}
}
