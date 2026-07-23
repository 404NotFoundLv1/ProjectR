// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionRuntimeSubsystem.h"

#include "Companions/PRCompanionPawn.h"
#include "Companions/PRCompanionRuntimeDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace PRCompanionRuntimeSubsystem
{
const FSoftObjectPath AxiomRuntimePath(TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Axiom.DA_CompanionRuntime_Axiom"));
const FSoftObjectPath KindleRuntimePath(TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Kindle.DA_CompanionRuntime_Kindle"));
const FSoftObjectPath NullRuntimePath(TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Null.DA_CompanionRuntime_Null"));

FSoftObjectPath GetRuntimePath(const FGameplayTag CompanionId)
{
	if (CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())) return AxiomRuntimePath;
	if (CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag())) return KindleRuntimePath;
	if (CompanionId.MatchesTagExact(FPRCompanionContract::NullTag())) return NullRuntimePath;
	return FSoftObjectPath();
}
}

void UPRCompanionRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	CompanionSubsystem = Companions;
	if (Companions)
	{
		PrimarySyncChangedHandle = Companions->OnPrimarySyncChanged().AddUObject(this, &UPRCompanionRuntimeSubsystem::HandlePrimarySyncChanged);
		ReconcilePrimaryPawn();
	}
}

void UPRCompanionRuntimeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReconcileRetryTimer);
	}
	if (UPRCompanionSubsystem* Companions = CompanionSubsystem.Get(); Companions && PrimarySyncChangedHandle.IsValid())
	{
		Companions->OnPrimarySyncChanged().Remove(PrimarySyncChangedHandle);
	}
	PrimarySyncChangedHandle.Reset();
	CompanionSubsystem.Reset();
	DestroyActiveCompanionPawn();
	SupportEvent.Clear();
	Super::Deinitialize();
}

float UPRCompanionRuntimeSubsystem::CalculateEffectiveSupportInterval(
	const UPRCompanionRuntimeDataAsset& RuntimeData,
	const int32 Overload)
{
	return RuntimeData.BaseSupportInterval * (1.0f + static_cast<float>(FMath::Clamp(Overload, 0, 100)) / 100.0f);
}

FPRCompanionSupportEventNative& UPRCompanionRuntimeSubsystem::OnSupportEvent()
{
	return SupportEvent;
}

APRCompanionPawn* UPRCompanionRuntimeSubsystem::GetActiveCompanionPawn() const
{
	return ActiveCompanionPawn.Get();
}

void UPRCompanionRuntimeSubsystem::HandlePrimarySyncChanged(const FPRPrimaryCompanionSyncChangedEvent& Event)
{
	ReconcilePrimaryPawn();
}

void UPRCompanionRuntimeSubsystem::ReconcilePrimaryPawn()
{
	UPRCompanionSubsystem* Companions = CompanionSubsystem.Get();
	const FPRCompanionSyncState SyncState = Companions ? Companions->GetSyncState() : FPRCompanionSyncState();
	if (!SyncState.PrimaryCompanionId.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReconcileRetryTimer);
		DestroyActiveCompanionPawn();
		return;
	}
	if (APRCompanionPawn* Existing = ActiveCompanionPawn.Get())
	{
		if (Existing->GetCompanionId().MatchesTagExact(SyncState.PrimaryCompanionId))
		{
			return;
		}
		DestroyActiveCompanionPawn();
	}
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}
	const FSoftObjectPath RuntimePath = PRCompanionRuntimeSubsystem::GetRuntimePath(SyncState.PrimaryCompanionId);
	UPRCompanionRuntimeDataAsset* RuntimeData = RuntimePath.IsValid()
		? Cast<UPRCompanionRuntimeDataAsset>(RuntimePath.TryLoad()) : nullptr;
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	UClass* PawnClass = RuntimeData ? RuntimeData->PawnClass.LoadSynchronous() : nullptr;
	if (!RuntimeData || !PawnClass || !PlayerPawn)
	{
		// GameInstance identity can already be restored before the new world has
		// possessed its player pawn. Retry only until the one legal pawn exists.
		GetWorld()->GetTimerManager().SetTimer(ReconcileRetryTimer, this,
			&UPRCompanionRuntimeSubsystem::RetryReconcilePrimaryPawn, 0.10f, false);
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector SpawnLocation = PlayerPawn->GetActorLocation() + FVector(-180.0f, 0.0f, 80.0f);
	APRCompanionPawn* Spawned = GetWorld()->SpawnActor<APRCompanionPawn>(PawnClass, SpawnLocation, PlayerPawn->GetActorRotation(), SpawnParameters);
	if (Spawned)
	{
		GetWorld()->GetTimerManager().ClearTimer(ReconcileRetryTimer);
		ActiveCompanionPawn = Spawned;
		Spawned->ConfigureRuntime(RuntimeData, SyncState.PrimaryCompanionId);
	}
	else
	{
		// Spawn can legitimately fail while a travel world is still resolving
		// WorldStatic collision. Keep the fixed single-pawn contract by retrying
		// the same reconciler rather than retaining a stale or duplicate pawn.
		GetWorld()->GetTimerManager().SetTimer(ReconcileRetryTimer, this,
			&UPRCompanionRuntimeSubsystem::RetryReconcilePrimaryPawn, 0.10f, false);
	}
}

void UPRCompanionRuntimeSubsystem::RetryReconcilePrimaryPawn()
{
	ReconcilePrimaryPawn();
}

void UPRCompanionRuntimeSubsystem::DestroyActiveCompanionPawn()
{
	if (APRCompanionPawn* Existing = ActiveCompanionPawn.Get())
	{
		Existing->Destroy();
	}
	ActiveCompanionPawn.Reset();
}

void UPRCompanionRuntimeSubsystem::PublishSupportEvent(FPRCompanionSupportEvent&& Event)
{
	SupportEvent.Broadcast(Event);
}
