// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionComponent.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Companions/PRCompanionPawn.h"
#include "Companions/PRCompanionRuntimeDataAsset.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace PRCompanionRuntime
{
constexpr float FollowIntervalSeconds = 0.05f;
constexpr float DesiredFollowDistance = 180.0f;
constexpr float MinimumFollowDistance = 150.0f;
constexpr float MaximumFollowDistance = 220.0f;
constexpr float FollowHeightOffset = 80.0f;
constexpr float TeleportRecoveryDistance = 900.0f;
constexpr float HealthFloorEpsilon = KINDA_SMALL_NUMBER;

FGameplayTag GetTargetValidationTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Skill.BasicAttack"), false);
}

bool IsElite(const FGameplayTag PrototypeTag)
{
	return PrototypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false));
}
}

UPRCompanionComponent::UPRCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UPRCompanionComponent::CalculateHealthFloor(const FGameplayTag PrototypeTag, const float MaxHealth)
{
	if (!FMath::IsFinite(MaxHealth) || MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return MaxHealth * (PRCompanionRuntime::IsElite(PrototypeTag) ? 0.35f : 0.20f);
}

float UPRCompanionComponent::ClampNoKillDamage(
	const float RequestedDamage,
	const float CurrentShield,
	const float CurrentHealth,
	const float HealthFloor)
{
	if (!FMath::IsFinite(RequestedDamage) || !FMath::IsFinite(CurrentShield)
		|| !FMath::IsFinite(CurrentHealth) || !FMath::IsFinite(HealthFloor)
		|| RequestedDamage <= 0.0f)
	{
		return 0.0f;
	}
	const float SafeDamage = FMath::Max(0.0f, CurrentShield) + FMath::Max(0.0f, CurrentHealth - HealthFloor);
	return FMath::Min(RequestedDamage, SafeDamage);
}

void UPRCompanionComponent::InitializeCompanion(UPRCompanionRuntimeDataAsset* InRuntimeData, const FGameplayTag InCompanionId)
{
	ShutdownCompanion();
	if (!InRuntimeData || !InCompanionId.IsValid() || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	RuntimeData = InRuntimeData;
	CompanionId = InCompanionId;
	bRuntimeInitialized = true;
	if (UWorld* World = GetWorld())
	{
		ScheduleNextFollow();
	}
	ScheduleNextSupport();
}

void UPRCompanionComponent::ShutdownCompanion()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FollowTimer);
		World->GetTimerManager().ClearTimer(SupportTimer);
	}
	RuntimeData = nullptr;
	CompanionId = FGameplayTag();
	bRuntimeInitialized = false;
}

void UPRCompanionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPRCompanionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownCompanion();
	Super::EndPlay(EndPlayReason);
}

void UPRCompanionComponent::TickFollow()
{
	APRCompanionPawn* Pawn = Cast<APRCompanionPawn>(GetOwner());
	APawn* PlayerPawn = GetWorld() ? UGameplayStatics::GetPlayerPawn(GetWorld(), 0) : nullptr;
	if (!bRuntimeInitialized || !Pawn || !PlayerPawn || PlayerPawn == Pawn)
	{
		ScheduleNextFollow();
		return;
	}

	const FVector PlayerVelocity = PlayerPawn->GetVelocity();
	if (!FMath::IsNearlyZero(PlayerVelocity.X))
	{
		LastFollowDirection = FVector(PlayerVelocity.X > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
	}
	const FVector DesiredLocation = PlayerPawn->GetActorLocation()
		+ LastFollowDirection * PRCompanionRuntime::DesiredFollowDistance
		+ FVector(0.0f, 0.0f, PRCompanionRuntime::FollowHeightOffset);
	const FVector CurrentLocation = Pawn->GetActorLocation();
	const FVector PlanarDelta(DesiredLocation.X - CurrentLocation.X, 0.0f, DesiredLocation.Z - CurrentLocation.Z);
	const float Distance = PlanarDelta.Size();
	if (Distance > PRCompanionRuntime::TeleportRecoveryDistance)
	{
		// Keep the single recovery path subject to the same WorldStatic
		// encroachment check as normal CharacterMovement. A blocked recovery is
		// deliberately retried by the next fixed one-shot decision.
		Pawn->TeleportTo(DesiredLocation, Pawn->GetActorRotation(), false, false);
		ScheduleNextFollow();
		return;
	}
	// The companion remains inside the authored 150-220 cm dead-band. It only
	// resumes moving once the player has pulled farther away than that band.
	if (Distance <= PRCompanionRuntime::MaximumFollowDistance)
	{
		ScheduleNextFollow();
		return;
	}
	if (UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement())
	{
		Movement->AddInputVector(PlanarDelta.GetSafeNormal());
	}
	ScheduleNextFollow();
}

void UPRCompanionComponent::ScheduleNextFollow()
{
	if (bRuntimeInitialized && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(FollowTimer, this, &UPRCompanionComponent::TickFollow,
			PRCompanionRuntime::FollowIntervalSeconds, false);
	}
}

float UPRCompanionComponent::GetEffectiveInterval(float& OutOverload) const
{
	OutOverload = 0.0f;
	if (!RuntimeData || !GetWorld())
	{
		return 0.0f;
	}
	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	const UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	FPRCompanionRelationshipRecord Relationship;
	if (Companions && Companions->GetRelationshipSnapshot(CompanionId, Relationship))
	{
		OutOverload = static_cast<float>(Relationship.State.Overload);
	}
	return UPRCompanionRuntimeSubsystem::CalculateEffectiveSupportInterval(*RuntimeData, static_cast<int32>(OutOverload));
}

void UPRCompanionComponent::ScheduleNextSupport()
{
	if (!bRuntimeInitialized || !RuntimeData || !GetWorld())
	{
		return;
	}
	float UnusedOverload = 0.0f;
	const float Interval = GetEffectiveInterval(UnusedOverload);
	if (!FMath::IsFinite(Interval) || Interval <= 0.0f)
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(SupportTimer, this, &UPRCompanionComponent::ExecuteSupport, Interval, false);
}

APREnemyCharacter* UPRCompanionComponent::FindSupportTarget() const
{
	APRCompanionPawn* Pawn = Cast<APRCompanionPawn>(GetOwner());
	if (!Pawn || !GetWorld())
	{
		return nullptr;
	}
	TArray<APREnemyCharacter*> Candidates;
	for (TActorIterator<APREnemyCharacter> It(GetWorld()); It; ++It)
	{
		APREnemyCharacter* Enemy = *It;
		if (Enemy && IsSupportTargetEligible(*Enemy)) Candidates.Add(Enemy);
	}
	const FVector PawnLocation = Pawn->GetActorLocation();
	const bool bPreferUnstunned = RuntimeData->SupportType == EPRCompanionSupportType::ControlMark;
	Candidates.Sort([PawnLocation, bPreferUnstunned](const APREnemyCharacter& Left, const APREnemyCharacter& Right)
	{
		if (bPreferUnstunned)
		{
			const UPRAbilitySystemComponent* LeftASC = Left.GetProjectRAbilitySystemComponent();
			const UPRAbilitySystemComponent* RightASC = Right.GetProjectRAbilitySystemComponent();
			const bool bLeftStunned = LeftASC && LeftASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag());
			const bool bRightStunned = RightASC && RightASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag());
			if (bLeftStunned != bRightStunned) return !bLeftStunned;
		}
		const FVector LeftDelta = Left.GetActorLocation() - PawnLocation;
		const FVector RightDelta = Right.GetActorLocation() - PawnLocation;
		const float LeftDistance = FVector(LeftDelta.X, 0.0f, LeftDelta.Z).SizeSquared();
		const float RightDistance = FVector(RightDelta.X, 0.0f, RightDelta.Z).SizeSquared();
		return FMath::IsNearlyEqual(LeftDistance, RightDistance)
			? Left.GetAbilityTargetId().LexicalLess(Right.GetAbilityTargetId())
			: LeftDistance < RightDistance;
	});
	return Candidates.IsEmpty() ? nullptr : Candidates[0];
}

bool UPRCompanionComponent::IsSupportTargetEligible(const APREnemyCharacter& Target) const
{
	APRCompanionPawn* Pawn = Cast<APRCompanionPawn>(GetOwner());
	if (!Pawn || !RuntimeData || !GetWorld() || !Target.IsEnemyInitialized() || Target.IsEnemyDead()
		|| !Target.CanBeTargetedByAbility(PRCompanionRuntime::GetTargetValidationTag()))
	{
		return false;
	}
	const FVector Delta = Target.GetActorLocation() - Pawn->GetActorLocation();
	if (FMath::Abs(Delta.Y) > 1.0f || FVector(Delta.X, 0.0f, Delta.Z).SizeSquared() > FMath::Square(RuntimeData->SupportRange))
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CompanionSupportLOS), false, Pawn);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetActorLocation(), Target.GetActorLocation(), ECC_Visibility, Params)
		|| Hit.GetActor() == &Target;
}

bool UPRCompanionComponent::ResolveBossHealthFloor(APREnemyCharacter& Target, float& OutHealthFloor) const
{
	OutHealthFloor = 0.0f;
	if (!GetWorld())
	{
		return false;
	}
	UPRBossSubsystem* Bosses = GetWorld()->GetSubsystem<UPRBossSubsystem>();
	FPRBossRuntimeState BossState;
	if (!Bosses || !Bosses->GetActiveBossState(BossState) || BossState.SpawnId != Target.GetSpawnId())
	{
		return false;
	}
	const UPRAuditorBossDataAsset* Data = Cast<UPRAuditorBossDataAsset>(Target.GetEnemyPrototype());
	if (!Data || !FMath::IsFinite(BossState.MaxHealth) || BossState.MaxHealth <= 0.0f)
	{
		OutHealthFloor = TNumericLimits<float>::Max();
		return true;
	}
	if (BossState.Phase == EPRAuditorBossPhase::Sampling)
	{
		OutHealthFloor = BossState.MaxHealth * Data->RuleAuditHealthRatio;
	}
	else if (BossState.Phase == EPRAuditorBossPhase::RuleAudit)
	{
		OutHealthFloor = BossState.MaxHealth * Data->PredictionShieldHealthRatio;
	}
	else
	{
		OutHealthFloor = TNumericLimits<float>::Max();
	}
	return true;
}

void UPRCompanionComponent::ExecuteSupport()
{
	FPRCompanionSupportEvent Event;
	Event.CompanionId = CompanionId;
	Event.SourceId = FName(*CompanionId.ToString());
	Event.SupportType = RuntimeData ? RuntimeData->SupportType : EPRCompanionSupportType::Shield;
	Event.RequestedMagnitude = RuntimeData ? RuntimeData->SupportMagnitude : 0.0f;
	Event.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Event.EffectiveIntervalSeconds = GetEffectiveInterval(Event.Overload);
	Event.Result = EPRCompanionSupportResult::RejectedInvalidState;

	APRCompanionPawn* Pawn = Cast<APRCompanionPawn>(GetOwner());
	if (!bRuntimeInitialized || !RuntimeData || !Pawn || !Pawn->HasAuthority() || !GetWorld())
	{
		PublishSupportEvent(MoveTemp(Event));
		return;
	}

	if (RuntimeData->SupportType == EPRCompanionSupportType::Shield)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		IAbilitySystemInterface* AbilityInterface = PlayerPawn ? Cast<IAbilitySystemInterface>(PlayerPawn) : nullptr;
		UPRAbilitySystemComponent* ASC = AbilityInterface ? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
		if (ASC && RuntimeData->SupportEffectClass)
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
				RuntimeData->SupportEffectClass, 1.0f, ASC->MakeEffectContext());
			if (Spec.IsValid() && Spec.Data.IsValid())
			{
				// Instant effects intentionally do not create an ActiveEffect handle.
				// A valid formal spec is therefore the authoritative success criterion;
				// the player ASC remains the sole owner of the Shield mutation.
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				Event.Result = EPRCompanionSupportResult::Applied;
				Event.AppliedMagnitude = RuntimeData->SupportMagnitude;
			}
			else
			{
				Event.Result = EPRCompanionSupportResult::RejectedByCombat;
			}
		}
	}
	else
	{
		APREnemyCharacter* Target = FindSupportTarget();
		if (!Target)
		{
			Event.Result = EPRCompanionSupportResult::SkippedNoEligibleTarget;
		}
		else
		{
			if (!IsSupportTargetEligible(*Target))
			{
				Event.Result = EPRCompanionSupportResult::SkippedNoEligibleTarget;
				PublishSupportEvent(MoveTemp(Event));
				ScheduleNextSupport();
				return;
			}
			Event.TargetId = Target->GetAbilityTargetId();
			Event.TargetLocation = Target->GetActorLocation();
			float BossFloor = 0.0f;
			const bool bTargetIsBoss = ResolveBossHealthFloor(*Target, BossFloor);
			if (RuntimeData->SupportType == EPRCompanionSupportType::ControlMark && bTargetIsBoss)
			{
				Event.Result = EPRCompanionSupportResult::Applied;
			}
			else if (RuntimeData->SupportType == EPRCompanionSupportType::ControlMark)
			{
				UPRAbilitySystemComponent* ASC = Target->GetProjectRAbilitySystemComponent();
				if (ASC && RuntimeData->SupportEffectClass && !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()))
				{
					const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
						RuntimeData->SupportEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
					Event.Result = Handle.IsValid() ? EPRCompanionSupportResult::Applied : EPRCompanionSupportResult::RejectedByCombat;
					Event.AppliedMagnitude = Handle.IsValid() ? RuntimeData->SupportMagnitude : 0.0f;
				}
				else
				{
					Event.Result = EPRCompanionSupportResult::SkippedNoEligibleTarget;
				}
			}
			else
			{
				const UPRAttributeSet* Attributes = Target->GetAttributeSet();
				const UPREnemyPrototypeDataAsset* Prototype = Target->GetEnemyPrototype();
				const float Floor = bTargetIsBoss ? BossFloor : (Attributes && Prototype
					? CalculateHealthFloor(Prototype->PrototypeTag, Attributes->GetMaxHealth()) : 0.0f);
				if (!Attributes || Floor == TNumericLimits<float>::Max())
				{
					Event.Result = EPRCompanionSupportResult::SkippedBossPhaseFloor;
				}
				else
				{
					const float Damage = ClampNoKillDamage(RuntimeData->SupportMagnitude, Attributes->GetShield(), Attributes->GetHealth(), Floor);
					Event.HealthFloor = Floor;
					if (Damage <= PRCompanionRuntime::HealthFloorEpsilon)
					{
						Event.Result = EPRCompanionSupportResult::SkippedAtHealthFloor;
					}
					else if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>())
					{
						FPRDamageRequest Request;
						Request.SourceId = Event.SourceId;
						Request.DamageSource = Pawn;
						Request.Instigator = Pawn;
						Request.Target = Target;
						Request.RawDamage = Damage;
						Request.bCritical = false;
						Request.DamageTags.AddTag(CompanionId);
						Request.ImpactOrigin = Pawn->GetActorLocation();
						Request.IncomingDirection = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
						const EPRCombatRequestStatus Status = Combat->ApplyDamage(Request);
						const UPRAttributeSet* PostAttributes = Target->GetAttributeSet();
						const bool bFloorPreserved = PostAttributes && PostAttributes->GetHealth() + PRCompanionRuntime::HealthFloorEpsilon >= Floor;
						Event.Result = Status == EPRCombatRequestStatus::Applied && bFloorPreserved
							? EPRCompanionSupportResult::Applied : EPRCompanionSupportResult::RejectedByCombat;
						Event.AppliedMagnitude = Event.Result == EPRCompanionSupportResult::Applied ? Damage : 0.0f;
					}
				}
			}
		}
	}
	PublishSupportEvent(MoveTemp(Event));
	ScheduleNextSupport();
}

void UPRCompanionComponent::PublishSupportEvent(FPRCompanionSupportEvent&& Event) const
{
	if (UWorld* World = GetWorld())
	{
		if (UPRCompanionRuntimeSubsystem* Runtime = World->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			Runtime->PublishSupportEvent(MoveTemp(Event));
		}
	}
}
