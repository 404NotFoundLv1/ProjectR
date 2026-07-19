// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyBrainComponent.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Core/PRCombatAttackProxyInterface.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemyProjectile.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Enemies/PREnemySubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR.h"
#include "TimerManager.h"

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
	ActiveAttack.Reset();
	AttackToken.Invalidate();
	ActiveProjectileTokens.Empty();
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

void UPREnemyBrainComponent::SetAttackPhase(const EPREnemyAttackPhase InPhase, const FGameplayTag InAttackTag)
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
	if (RuntimeState.AttackPhase != EPREnemyAttackPhase::None && !IsCurrentAttackTargetValid())
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
	const UPREnemyPrototypeDataAsset* Prototype = EnemyCharacter ? EnemyCharacter->GetEnemyPrototype() : nullptr;
	const UPREnemyAttackDataAsset* Attack = FindConfiguredAttack();
	UPREnemyPlaneMovementComponent* Movement = EnemyCharacter ? EnemyCharacter->GetEnemyPlaneMovement() : nullptr;
	if (!bRunning || !EnemyCharacter || !Target || !Prototype || !Attack || !Movement)
	{
		return;
	}
	if (RuntimeState.AttackPhase != EPREnemyAttackPhase::None)
	{
		SetBrainState(EPREnemyBrainState::Attack);
		Movement->StopEnemyMovement();
		return;
	}

	const float DeltaX = Target->GetActorLocation().X - EnemyCharacter->GetActorLocation().X;
	const float Distance = FMath::Abs(DeltaX);
	const float Direction = FMath::Sign(DeltaX);
	if (Distance > Prototype->Perception.PreferredMaxRange)
	{
		if (!Movement->IsPlatformSafeInDirection(Direction, Prototype->Perception.EdgeProbeForward, Prototype->Perception.EdgeProbeDepth))
		{
			SetBrainState(EPREnemyBrainState::HoldRange);
			Movement->StopEnemyMovement();
			return;
		}
		SetBrainState(EPREnemyBrainState::Pursue);
		Movement->SetDesiredDirectionX(Direction);
		return;
	}
	if (Distance < Prototype->Perception.PreferredMinRange)
	{
		SetBrainState(EPREnemyBrainState::Retreat);
		if (!Movement->IsPlatformSafeInDirection(-Direction, Prototype->Perception.EdgeProbeForward, Prototype->Perception.EdgeProbeDepth))
		{
			Movement->StopEnemyMovement();
			return;
		}
		Movement->SetDesiredDirectionX(-Direction);
		return;
	}

	SetBrainState(EPREnemyBrainState::HoldRange);
	Movement->StopEnemyMovement();
	if (CanBeginAttack(Attack, Target))
	{
		TryActivateCurrentAttack();
	}
}

bool UPREnemyBrainComponent::TryActivateCurrentAttack()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	const UPREnemyAttackDataAsset* Attack = FindConfiguredAttack();
	AActor* Target = CurrentTarget.Get();
	if (!EnemyCharacter || !Attack || !CanBeginAttack(Attack, Target))
	{
		return false;
	}
	FGameplayTagContainer FailureTags;
	return EnemyCharacter->GetProjectRAbilitySystemComponent()->TryActivateAbilityByAbilityTag(Attack->AttackTag, FailureTags);
}

bool UPREnemyBrainComponent::CanBeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target) const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!bRunning || !Attack || !Target || !EnemyCharacter || RuntimeState.AttackPhase != EPREnemyAttackPhase::None
		|| Target != CurrentTarget.Get() || !IsCurrentAttackTargetValid())
	{
		return false;
	}
	if (!EnemyCharacter->GetAttackDefinitions().Contains(Attack))
	{
		return false;
	}
	const float Distance = FMath::Abs(Target->GetActorLocation().X - EnemyCharacter->GetActorLocation().X);
	if (Distance < Attack->MinRange || Distance > Attack->MaxRange)
	{
		return false;
	}
	return Attack->Kind != EPREnemyAttackKind::Projectile || HasLineOfSightTo(Target);
}

bool UPREnemyBrainComponent::BeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target)
{
	if (!CanBeginAttack(Attack, Target))
	{
		return false;
	}
	CurrentTarget = Target;
	RuntimeState.Target = Target;
	ActiveAttack = Attack;
	AttackToken = FGuid::NewGuid();
	LockedAttackDirection = Target->GetActorLocation() - Enemy.Get()->GetActorLocation();
	LockedAttackDirection.Y = 0.0f;
	LockedAttackDirection = LockedAttackDirection.GetSafeNormal();
	AttackCooldownEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + Attack->Cooldown : 0.0;
	if (!AttackToken.IsValid() || LockedAttackDirection.IsNearlyZero())
	{
		CancelAttack();
		return false;
	}
	SetBrainState(EPREnemyBrainState::Attack);
	SetAttackPhase(EPREnemyAttackPhase::Windup, Attack->AttackTag);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishWindup, Attack->Windup, false);
		return true;
	}
	CancelAttack();
	return false;
}

void UPREnemyBrainComponent::CancelAttack()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimer);
	}
	ActiveAttack.Reset();
	AttackToken.Invalidate();
	LockedAttackDirection = FVector::ZeroVector;
	AttackCooldownEndTime = 0.0;
	SetAttackPhase(EPREnemyAttackPhase::None);
}

void UPREnemyBrainComponent::FinishWindup()
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	AActor* Target = CurrentTarget.Get();
	const UPREnemyAttackDataAsset* Attack = ActiveAttack.Get();
	if (!EnemyCharacter || !Target || !Attack || EnemyCharacter->IsEnemyDead() || !IsCurrentAttackTargetValid() || !AttackToken.IsValid())
	{
		CancelAttack();
		return;
	}
	SetAttackPhase(EPREnemyAttackPhase::Active, Attack->AttackTag);
	if (Attack->Kind == EPREnemyAttackKind::Projectile)
	{
		SpawnProjectile(Attack, Target);
		GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishActive, Attack->ActiveWindow, false);
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - EnemyCharacter->GetActorLocation();
	ToTarget.Y = 0.0f;
	const float Distance = ToTarget.Length();
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(Attack->SweepHalfAngleDegrees));
	const bool bInsideSweep = Distance >= Attack->MinRange && Distance <= Attack->SweepRadius
		&& FVector::DotProduct(LockedAttackDirection, ToTarget.GetSafeNormal()) >= MinimumDot;
	if (bInsideSweep)
	{
		AActor* ClosestProxy = nullptr;
		float ClosestProxyDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* ProxyActor = *It;
			if (!ProxyActor || !ProxyActor->GetClass()->ImplementsInterface(UPRCombatAttackProxyInterface::StaticClass()))
			{
				continue;
			}
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
			if (IPRCombatAttackProxyInterface* Proxy = Cast<IPRCombatAttackProxyInterface>(ClosestProxy);
				Proxy && Proxy->ConsumeAttackProxy(EnemyCharacter))
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
	}
	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishActive, Attack->ActiveWindow, false);
}

void UPREnemyBrainComponent::FinishActive()
{
	const UPREnemyAttackDataAsset* Attack = ActiveAttack.Get();
	if (!Attack || !GetWorld())
	{
		CancelAttack();
		return;
	}
	SetAttackPhase(EPREnemyAttackPhase::Recovery, Attack->AttackTag);
	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UPREnemyBrainComponent::FinishRecovery, Attack->Recovery, false);
}

void UPREnemyBrainComponent::FinishRecovery()
{
	const UPREnemyAttackDataAsset* Attack = ActiveAttack.Get();
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
	ActiveAttack.Reset();
	AttackToken.Invalidate();
	LockedAttackDirection = FVector::ZeroVector;
	AttackCooldownEndTime = 0.0;
}

AActor* UPREnemyBrainComponent::FindBestTarget() const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter || !EnemyCharacter->GetEnemyPrototype() || !GetWorld())
	{
		return nullptr;
	}
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AActor* Candidate = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Candidate || Candidate == EnemyCharacter
		|| !Candidate->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass())
		|| !Candidate->GetClass()->ImplementsInterface(UPRCombatantInterface::StaticClass()))
	{
		return nullptr;
	}
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Candidate);
	UPRAbilitySystemComponent* CandidateASC = AbilityInterface
		? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	if (!CandidateASC || CandidateASC->GetAvatarActor() != Candidate
		|| CandidateASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		return nullptr;
	}
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

bool UPREnemyBrainComponent::HasLineOfSightTo(AActor* Target) const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	UWorld* World = GetWorld();
	if (!EnemyCharacter || !Target || !World)
	{
		return false;
	}
	FVector Start = EnemyCharacter->GetActorLocation();
	FVector End = Target->GetActorLocation();
	Start.Y = EnemyCharacter->GetActorLocation().Y;
	End.Y = Start.Y;
	if (!FMath::IsFinite(Start.X) || !FMath::IsFinite(Start.Y) || !FMath::IsFinite(Start.Z)
		|| !FMath::IsFinite(End.X) || !FMath::IsFinite(End.Y) || !FMath::IsFinite(End.Z))
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PREnemyRangedLOS), false, EnemyCharacter);
	return !World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) || Hit.GetActor() == Target;
}

const UPREnemyAttackDataAsset* UPREnemyBrainComponent::FindConfiguredAttack() const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter)
	{
		return nullptr;
	}
	for (const TObjectPtr<UPREnemyAttackDataAsset>& Attack : EnemyCharacter->GetAttackDefinitions())
	{
		if (Attack)
		{
			return Attack;
		}
	}
	return nullptr;
}

const UPREnemyAttackDataAsset* UPREnemyBrainComponent::FindAttackForTarget(AActor* Target) const
{
	const APREnemyCharacter* EnemyCharacter = Enemy.Get();
	if (!EnemyCharacter || !Target)
	{
		return nullptr;
	}
	const float Distance = FMath::Abs(Target->GetActorLocation().X - EnemyCharacter->GetActorLocation().X);
	for (const TObjectPtr<UPREnemyAttackDataAsset>& Attack : EnemyCharacter->GetAttackDefinitions())
	{
		if (Attack && Distance >= Attack->MinRange && Distance <= Attack->MaxRange)
		{
			return Attack;
		}
	}
	return nullptr;
}

bool UPREnemyBrainComponent::SpawnProjectile(const UPREnemyAttackDataAsset* Attack, AActor* Target)
{
	APREnemyCharacter* EnemyCharacter = Enemy.Get();
	UWorld* World = GetWorld();
	UPREnemySubsystem* EnemySubsystem = World ? World->GetSubsystem<UPREnemySubsystem>() : nullptr;
	UClass* ProjectileClass = EnemySubsystem ? EnemySubsystem->GetPreloadedProjectileClass(EnemyCharacter, Attack).Get() : nullptr;
	if (!EnemyCharacter || !World || !Attack || !Target || !ProjectileClass
		|| !ProjectileClass->IsChildOf(APREnemyProjectile::StaticClass()) || !AttackToken.IsValid())
	{
		UE_LOG(LogProjectR, Error, TEXT("Enemy projectile spawn rejected: fixed preloaded projectile class, attack, target, or token was invalid."));
		return false;
	}
	if (ActiveProjectileTokens.Contains(AttackToken))
	{
		return false;
	}
	ActiveProjectileTokens.Add(AttackToken);
	const UCapsuleComponent* Capsule = EnemyCharacter->GetCapsuleComponent();
	const float SpawnOffset = (Capsule ? Capsule->GetScaledCapsuleRadius() : 0.0f) + 14.0f;
	FVector SpawnLocation = EnemyCharacter->GetActorLocation() + LockedAttackDirection * SpawnOffset;
	SpawnLocation.Y = EnemyCharacter->GetActorLocation().Y;
	const FTransform SpawnTransform(LockedAttackDirection.Rotation(), SpawnLocation);
	APREnemyProjectile* Projectile = World->SpawnActorDeferred<APREnemyProjectile>(
		ProjectileClass, SpawnTransform, EnemyCharacter, EnemyCharacter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		ActiveProjectileTokens.Remove(AttackToken);
		return false;
	}
	Projectile->FinishSpawning(SpawnTransform);
	if (!Projectile->InitializeProjectile(EnemyCharacter, Target, Attack, AttackToken, LockedAttackDirection, SpawnLocation.Y))
	{
		UE_LOG(LogProjectR, Error, TEXT("Enemy projectile spawn rejected: completed projectile initialization failed."));
		Projectile->Destroy();
		ActiveProjectileTokens.Remove(AttackToken);
		return false;
	}
	return true;
}

bool UPREnemyBrainComponent::TryResolveProjectileToken(const FGuid& Token)
{
	return Token.IsValid() && ActiveProjectileTokens.Remove(Token) > 0;
}

void UPREnemyBrainComponent::ReleaseProjectileToken(const FGuid& Token)
{
	if (Token.IsValid())
	{
		ActiveProjectileTokens.Remove(Token);
	}
}
