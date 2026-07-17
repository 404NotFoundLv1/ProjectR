// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyBrainComponent.h"

#include "Enemies/PREnemyCharacter.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatAttackProxyInterface.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

UPREnemyBrainComponent::UPREnemyBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPREnemyBrainComponent::InitializeBrain(APREnemyCharacter* InEnemy)
{
	Enemy = InEnemy;
	bRunning = IsValid(InEnemy);
	RuntimeState = FPREnemyRuntimeState();
	RuntimeState.BrainState = bRunning ? EPREnemyBrainState::AcquireTarget : EPREnemyBrainState::Dormant;
	if (const UPREnemyPrototypeDataAsset* Prototype = InEnemy ? InEnemy->GetEnemyPrototype() : nullptr)
	{
		RuntimeState.SpawnId = InEnemy->GetSpawnId();
		RuntimeState.CombatantId = InEnemy->GetCombatantId();
		RuntimeState.PrototypeTag = Prototype->PrototypeTag;
		RuntimeState.Mobility = Prototype->Mobility;
		RuntimeState.bAlive = true;
	}
	if (bRunning && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(ReevaluateTimer, this, &UPREnemyBrainComponent::ReevaluateTarget, 0.10f, true);
	}
}

void UPREnemyBrainComponent::StopBrain()
{
	bRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReevaluateTimer);
		World->GetTimerManager().ClearTimer(AttackTimer);
	}
	if (APREnemyCharacter* EnemyCharacter = Enemy.Get())
	{
		if (UPREnemyPlaneMovementComponent* Movement = EnemyCharacter->GetEnemyPlaneMovement())
		{
			Movement->StopEnemyMovement();
		}
	}
	RuntimeState.BrainState = EPREnemyBrainState::Dead;
	RuntimeState.AttackPhase = EPREnemyAttackPhase::None;
	RuntimeState.ActiveAttackTag = FGameplayTag();
	RuntimeState.Target.Reset();
	RuntimeState.bAlive = false;
	CurrentTarget.Reset();
	AttackToken.Invalidate();
	LockedAttackDirection = FVector::ZeroVector;
	AttackCooldownEndTime = 0.0;
}

void UPREnemyBrainComponent::SetBrainState(const EPREnemyBrainState InState)
{
	if (bRunning)
	{
		RuntimeState.BrainState = InState;
	}
}

void UPREnemyBrainComponent::SetAttackPhase(
	const EPREnemyAttackPhase InPhase,
	const FGameplayTag InAttackTag)
{
	if (bRunning)
	{
		RuntimeState.AttackPhase = InPhase;
		RuntimeState.ActiveAttackTag = InAttackTag;
	}
}

const FPREnemyRuntimeState& UPREnemyBrainComponent::GetRuntimeState() const
{
	return RuntimeState;
}

void UPREnemyBrainComponent::ReevaluateTarget()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!bRunning || !EnemyCharacter || EnemyCharacter->IsEnemyDead())
	{
		StopBrain();
		return;
	}
	if (RuntimeState.AttackPhase != EPREnemyAttackPhase::None
		&& !IsCurrentAttackTargetValid())
	{
		CancelAttack();
	}
	AActor* NewTarget = FindBestTarget();
	if (RuntimeState.AttackPhase != EPREnemyAttackPhase::None && NewTarget != CurrentTarget.Get())
	{
		CancelAttack();
	}
	CurrentTarget = NewTarget;
	RuntimeState.Target = NewTarget;
	if (!NewTarget)
	{
		SetBrainState(EPREnemyBrainState::AcquireTarget);
		if (UPREnemyPlaneMovementComponent* Movement = EnemyCharacter->GetEnemyPlaneMovement())
		{
			Movement->StopEnemyMovement();
		}
		return;
	}
	UpdateRangeMovement();
	if (APREnemyAIController* Controller = Cast<APREnemyAIController>(EnemyCharacter->GetController()))
	{
		Controller->SendEnemyReevaluateEvent();
	}
}

void UPREnemyBrainComponent::UpdateRangeMovement()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	AActor* Target = CurrentTarget.Get();
	const UPREnemyAttackDataAsset* Attack = FindAttackForTarget(Target);
	if (!bRunning || !EnemyCharacter || !Target || !Attack)
	{
		return;
	}
	const float DeltaX = Target->GetActorLocation().X - EnemyCharacter->GetActorLocation().X;
	if (FMath::Abs(DeltaX) <= Attack->MaxRange)
	{
		SetBrainState(EPREnemyBrainState::HoldRange);
		EnemyCharacter->GetEnemyPlaneMovement()->StopEnemyMovement();
		TryActivateCurrentAttack();
		return;
	}
	UPREnemyPlaneMovementComponent* Movement = EnemyCharacter->GetEnemyPlaneMovement();
	if (!Movement || !Movement->IsPlatformSafeInDirection(FMath::Sign(DeltaX), EnemyCharacter->GetEnemyPrototype()->Perception.EdgeProbeForward, EnemyCharacter->GetEnemyPrototype()->Perception.EdgeProbeDepth))
	{
		SetBrainState(EPREnemyBrainState::HoldRange);
		if (Movement) Movement->StopEnemyMovement();
		return;
	}
	SetBrainState(EPREnemyBrainState::Pursue);
	Movement->SetDesiredDirectionX(FMath::Sign(DeltaX));
}

bool UPREnemyBrainComponent::TryActivateCurrentAttack()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	const UPREnemyAttackDataAsset* Attack = FindAttackForTarget(CurrentTarget.Get());
	if (!bRunning || RuntimeState.AttackPhase != EPREnemyAttackPhase::None || !EnemyCharacter || !Attack)
	{
		return false;
	}
	FGameplayTagContainer FailureTags;
	return EnemyCharacter->GetProjectRAbilitySystemComponent()->TryActivateAbilityByAbilityTag(Attack->AttackTag, FailureTags);
}

bool UPREnemyBrainComponent::BeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target)
{
	if (!bRunning || !Attack || !Target || RuntimeState.AttackPhase != EPREnemyAttackPhase::None)
	{
		return false;
	}
	CurrentTarget = Target;
	RuntimeState.Target = Target;
	AttackToken = FGuid::NewGuid();
	LockedAttackDirection = Target->GetActorLocation() - Enemy.Get()->GetActorLocation();
	LockedAttackDirection.Y = 0.0f;
	LockedAttackDirection = LockedAttackDirection.GetSafeNormal();
	AttackCooldownEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + Attack->Cooldown : 0.0;
	if (AttackToken.IsValid() == false || LockedAttackDirection.IsNearlyZero())
	{
		CancelAttack();
		return false;
	}
	SetAttackPhase(EPREnemyAttackPhase::Windup, Attack->AttackTag);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishWindup, Attack->Windup, false);
		return true;
	}
	return false;
}

void UPREnemyBrainComponent::CancelAttack()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(AttackTimer);
	AttackToken.Invalidate();
	LockedAttackDirection = FVector::ZeroVector;
	AttackCooldownEndTime = 0.0;
	SetAttackPhase(EPREnemyAttackPhase::None);
}

void UPREnemyBrainComponent::FinishWindup()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	AActor* Target = CurrentTarget.Get();
	const UPREnemyAttackDataAsset* Attack = FindAttackForTarget(Target);
	if (!EnemyCharacter || !Target || !Attack || EnemyCharacter->IsEnemyDead()
		|| !IsCurrentAttackTargetValid() || !AttackToken.IsValid()) { CancelAttack(); return; }
	SetAttackPhase(EPREnemyAttackPhase::Active, Attack->AttackTag);
	FVector ToTarget = Target->GetActorLocation() - EnemyCharacter->GetActorLocation();
	ToTarget.Y = 0.0f;
	const float Distance = ToTarget.Length();
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(Attack->SweepHalfAngleDegrees));
	const bool bInsideSweep = Distance >= Attack->MinRange && Distance <= Attack->SweepRadius
		&& FVector::DotProduct(LockedAttackDirection, ToTarget.GetSafeNormal()) >= MinimumDot;
	if (!bInsideSweep)
	{
		GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishActive, Attack->ActiveWindow, false);
		return;
	}
	AActor* ClosestProxy = nullptr;
	float ClosestProxyDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* ProxyActor = *It;
		if (!ProxyActor || !ProxyActor->GetClass()->ImplementsInterface(UPRCombatAttackProxyInterface::StaticClass())) continue;
		IPRCombatAttackProxyInterface* Proxy = Cast<IPRCombatAttackProxyInterface>(ProxyActor);
		if (Proxy && Proxy->GetAttackProxyOwner() == Target)
		{
			const float DistanceSquared = FVector::DistSquared(EnemyCharacter->GetActorLocation(), ProxyActor->GetActorLocation());
			if (DistanceSquared < ClosestProxyDistanceSquared)
			{
				ClosestProxy = ProxyActor;
				ClosestProxyDistanceSquared = DistanceSquared;
			}
		}
	}
	if (ClosestProxy)
	{
		IPRCombatAttackProxyInterface* Proxy = Cast<IPRCombatAttackProxyInterface>(ClosestProxy);
		if (Proxy && Proxy->ConsumeAttackProxy(EnemyCharacter))
		{
			GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishActive, Attack->ActiveWindow, false);
			return;
		}
	}
	if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>())
	{
		FPRDamageRequest Request;
		Request.SourceId = Attack->AttackId;
		Request.DamageSource = const_cast<UPREnemyAttackDataAsset*>(Attack);
		Request.Instigator = EnemyCharacter;
		Request.Target = Target;
		Request.AbilityTag = Attack->AttackTag;
		Request.RawDamage = Attack->BaseDamage + EnemyCharacter->GetAttributeSet()->GetAttackPower() * Attack->AttackPowerScale;
		Request.DamageTags = Attack->DamageTags;
		Request.DamageTags.AddTag(EnemyCharacter->GetEnemyPrototype()->PrototypeTag);
		Request.DamageTags.AddTag(Attack->AttackTag);
		Request.ImpactOrigin = EnemyCharacter->GetActorLocation();
		Request.IncomingDirection = ToTarget.GetSafeNormal();
		Combat->ApplyDamage(Request);
	}
	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishActive, Attack->ActiveWindow, false);
}

void UPREnemyBrainComponent::FinishActive()
{
	const UPREnemyAttackDataAsset* Attack = FindAttackForTarget(CurrentTarget.Get());
	if (!Attack) { CancelAttack(); return; }
	SetAttackPhase(EPREnemyAttackPhase::Recovery, Attack->AttackTag);
	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishRecovery, Attack->Recovery, false);
}

void UPREnemyBrainComponent::FinishRecovery()
{
	const UPREnemyAttackDataAsset* Attack = FindAttackForTarget(CurrentTarget.Get());
	if (!Attack || !GetWorld())
	{
		CancelAttack();
		return;
	}
	const float RemainingCooldown = FMath::Max(0.0, AttackCooldownEndTime - GetWorld()->GetTimeSeconds());
	if (RemainingCooldown <= KINDA_SMALL_NUMBER)
	{
		FinishCooldown();
		return;
	}
	SetAttackPhase(EPREnemyAttackPhase::Cooldown, Attack->AttackTag);
	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishCooldown, RemainingCooldown, false);
}

void UPREnemyBrainComponent::FinishCooldown()
{
	SetAttackPhase(EPREnemyAttackPhase::None);
	AttackToken.Invalidate();
	LockedAttackDirection = FVector::ZeroVector;
	AttackCooldownEndTime = 0.0;
}

AActor* UPREnemyBrainComponent::FindBestTarget() const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter || !EnemyCharacter->GetEnemyPrototype() || !GetWorld()) return nullptr;
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AActor* Candidate = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Candidate || Candidate == EnemyCharacter
		|| !Candidate->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass())
		|| !Candidate->GetClass()->ImplementsInterface(UPRCombatantInterface::StaticClass())) return nullptr;
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Candidate);
	UPRAbilitySystemComponent* CandidateASC = AbilityInterface
		? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	if (!CandidateASC || CandidateASC->GetAvatarActor() != Candidate
		|| CandidateASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())) return nullptr;
	const float Distance = FMath::Abs(Candidate->GetActorLocation().X - EnemyCharacter->GetActorLocation().X);
	const float AllowedRange = CurrentTarget.Get() == Candidate
		? EnemyCharacter->GetEnemyPrototype()->Perception.LoseRange
		: EnemyCharacter->GetEnemyPrototype()->Perception.AcquireRange;
	return Distance <= AllowedRange ? Candidate : nullptr;
}

bool UPREnemyBrainComponent::IsCurrentAttackTargetValid() const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter || !GetWorld())
	{
		return false;
	}
	AActor* Target = CurrentTarget.Get();
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!Target || !PlayerController || PlayerController->GetPawn() != Target
		|| !Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass())
		|| !Target->GetClass()->ImplementsInterface(UPRCombatantInterface::StaticClass()))
	{
		return false;
	}
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Target);
	UPRAbilitySystemComponent* TargetASC = AbilityInterface
		? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	return TargetASC && TargetASC->GetAvatarActor() == Target
		&& !TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
		&& !TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag());
}

const UPREnemyAttackDataAsset* UPREnemyBrainComponent::FindAttackForTarget(AActor* Target) const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter || !Target) return nullptr;
	const float Distance = FMath::Abs(Target->GetActorLocation().X - EnemyCharacter->GetActorLocation().X);
	for (const TObjectPtr<UPREnemyAttackDataAsset>& Attack : EnemyCharacter->GetAttackDefinitions())
	{
		if (Attack && Distance >= Attack->MinRange && Distance <= Attack->MaxRange) return Attack;
	}
	return nullptr;
}
