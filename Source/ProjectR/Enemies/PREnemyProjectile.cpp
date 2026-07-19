// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyProjectile.h"

#include "AbilitySystemInterface.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Components/SphereComponent.h"
#include "Core/PRCombatAttackProxyInterface.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR.h"
#include "TimerManager.h"

namespace PREnemyProjectile
{
constexpr float CollisionRadius = 12.0f;
constexpr float ValidationInterval = 0.10f;

struct FProxyCandidate
{
	AActor* Actor = nullptr;
	IPRCombatAttackProxyInterface* Proxy = nullptr;
	float Alpha = 0.0f;
	FName Id;
};
}

void UPREnemyProjectileMovementComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	const FVector SegmentStart = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (APREnemyProjectile* Projectile = Cast<APREnemyProjectile>(GetOwner()))
	{
		if (UpdatedComponent && !Projectile->HandleMovementSegment(SegmentStart, UpdatedComponent->GetComponentLocation(), nullptr))
		{
			return;
		}
	}
}

void UPREnemyProjectileMovementComponent::HandleImpact(
	const FHitResult& Hit,
	const float TimeSlice,
	const FVector& MoveDelta)
{
	if (APREnemyProjectile* Projectile = Cast<APREnemyProjectile>(GetOwner()))
	{
		Projectile->HandleMovementSegment(Hit.TraceStart, Hit.ImpactPoint, &Hit);
		return;
	}
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}

APREnemyProjectile::APREnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(PREnemyProjectile::CollisionRadius);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetRootComponent(Collision);
	ProjectileMovement = CreateDefaultSubobject<UPREnemyProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bSweepCollision = true;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->MaxSimulationTimeStep = 1.0f / 60.0f;
	ProjectileMovement->MaxSimulationIterations = 8;
	ProjectileMovement->bConstrainToPlane = true;
	ProjectileMovement->SetPlaneConstraintNormal(FVector::YAxisVector);
}

bool APREnemyProjectile::InitializeProjectile(
	APREnemyCharacter* InSourceEnemy,
	AActor* InTargetAvatar,
	const UPREnemyAttackDataAsset* AttackData,
	const FGuid& InAttackToken,
	const FVector& InLockedDirection,
	const float InPlaneY)
{
	if (bInitialized || !HasAuthority() || !IsValid(InSourceEnemy) || !IsValid(InTargetAvatar)
		|| !AttackData || AttackData->Kind != EPREnemyAttackKind::Projectile || !InAttackToken.IsValid()
		|| !FMath::IsFinite(InPlaneY) || !FMath::IsFinite(AttackData->ProjectileSpeed)
		|| !FMath::IsFinite(AttackData->ProjectileLifetime) || AttackData->ProjectileSpeed <= 0.0f
		|| AttackData->ProjectileLifetime <= 0.0f)
	{
		return false;
	}

	FVector Direction = InLockedDirection;
	Direction.Y = 0.0f;
	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero() || !FMath::IsFinite(Direction.X) || !FMath::IsFinite(Direction.Y) || !FMath::IsFinite(Direction.Z))
	{
		return false;
	}

	SourceEnemy = InSourceEnemy;
	TargetAvatar = InTargetAvatar;
	AttackToken = InAttackToken;
	AttackId = AttackData->AttackId;
	AbilityTag = AttackData->AttackTag;
	DamageTags = AttackData->DamageTags;
	if (const UPREnemyPrototypeDataAsset* Prototype = InSourceEnemy->GetEnemyPrototype())
	{
		DamageTags.AddTag(Prototype->PrototypeTag);
	}
	DamageTags.AddTag(AttackData->AttackTag);
	LockedDirection = Direction;
	PlaneY = InPlaneY;
	RawDamage = AttackData->BaseDamage + InSourceEnemy->GetAttributeSet()->GetAttackPower() * AttackData->AttackPowerScale;
	if (!FMath::IsFinite(RawDamage) || RawDamage < 0.0f)
	{
		return false;
	}

	SetActorLocation(FVector(GetActorLocation().X, PlaneY, GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	Collision->IgnoreActorWhenMoving(InSourceEnemy, true);
	ProjectileMovement->SetPlaneConstraintOrigin(FVector(0.0f, PlaneY, 0.0f));
	ProjectileMovement->Velocity = LockedDirection * AttackData->ProjectileSpeed;
	ProjectileMovement->InitialSpeed = AttackData->ProjectileSpeed;
	ProjectileMovement->MaxSpeed = AttackData->ProjectileSpeed;
	InSourceEnemy->OnEndPlay.AddDynamic(this, &APREnemyProjectile::HandleWatchedActorEndPlay);
	InTargetAvatar->OnEndPlay.AddDynamic(this, &APREnemyProjectile::HandleWatchedActorEndPlay);
	GetWorldTimerManager().SetTimer(LifetimeTimer, this, &APREnemyProjectile::HandleLifetimeExpired, AttackData->ProjectileLifetime, false);
	GetWorldTimerManager().SetTimer(ValidationTimer, this, &APREnemyProjectile::ValidateRuntime, PREnemyProjectile::ValidationInterval, true);
	bInitialized = true;
	return true;
}

void APREnemyProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	GetWorldTimerManager().ClearTimer(ValidationTimer);
	if (AActor* Source = SourceEnemy.Get())
	{
		Source->OnEndPlay.RemoveDynamic(this, &APREnemyProjectile::HandleWatchedActorEndPlay);
	}
	if (AActor* Target = TargetAvatar.Get())
	{
		Target->OnEndPlay.RemoveDynamic(this, &APREnemyProjectile::HandleWatchedActorEndPlay);
	}
	if (!bResolved)
	{
		ReleaseAttackToken();
	}
	SourceEnemy.Reset();
	TargetAvatar.Reset();
	Super::EndPlay(EndPlayReason);
}

void APREnemyProjectile::HandleWatchedActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	FinishProjectile(false);
}

void APREnemyProjectile::HandleLifetimeExpired()
{
	FinishProjectile(false);
}

void APREnemyProjectile::ValidateRuntime()
{
	if (!IsRuntimeValid())
	{
		FinishProjectile(false);
	}
}

bool APREnemyProjectile::HandleMovementSegment(
	const FVector& SegmentStart,
	const FVector& SegmentEnd,
	const FHitResult* Impact)
{
	if (!bInitialized || bResolved)
	{
		return false;
	}
	if (!IsRuntimeValid())
	{
		FinishProjectile(false);
		return false;
	}
	if (TryConsumeNearestAttackProxy(SegmentStart, SegmentEnd))
	{
		return false;
	}
	FHitResult SweepImpact;
	const FHitResult* ResolvedImpact = Impact;
	if (!ResolvedImpact)
	{
		UWorld* World = GetWorld();
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyProjectileSegment), false, this);
		QueryParams.AddIgnoredActor(SourceEnemy.Get());
		const bool bStaticHit = World && World->SweepSingleByObjectType(SweepImpact, SegmentStart, SegmentEnd, FQuat::Identity,
			ObjectQuery, FCollisionShape::MakeSphere(PREnemyProjectile::CollisionRadius), QueryParams);
		AActor* Target = TargetAvatar.Get();
		const FVector Segment = SegmentEnd - SegmentStart;
		const float LengthSquared = Segment.SizeSquared();
		if (Target && LengthSquared > KINDA_SMALL_NUMBER)
		{
			FVector TargetPoint = Target->GetActorLocation();
			TargetPoint.Y = PlaneY;
			const float TargetAlpha = FMath::Clamp(FVector::DotProduct(TargetPoint - SegmentStart, Segment) / LengthSquared, 0.0f, 1.0f);
			const FVector NearestPoint = SegmentStart + Segment * TargetAlpha;
			const float TargetRadius = FMath::Max(16.0f, Target->GetSimpleCollisionRadius()) + PREnemyProjectile::CollisionRadius;
			if (FVector::DistSquared(TargetPoint, NearestPoint) <= FMath::Square(TargetRadius)
				&& (!bStaticHit || TargetAlpha < SweepImpact.Time))
			{
				FHitResult TargetImpact;
				TargetImpact.ImpactPoint = NearestPoint;
				TargetImpact.Location = NearestPoint;
				ResolveTargetHit(TargetImpact);
				return false;
			}
		}
		if (bStaticHit)
		{
			UE_LOG(LogProjectR, Warning, TEXT("Enemy projectile stopped by WorldStatic before reaching its target: %s."),
				*GetNameSafe(SweepImpact.GetActor()));
			ResolvedImpact = &SweepImpact;
		}
	}
	if (ResolvedImpact)
	{
		if (ResolvedImpact->GetActor() == TargetAvatar.Get())
		{
			ResolveTargetHit(*ResolvedImpact);
		}
		else
		{
			FinishProjectile(false);
		}
		return false;
	}
	return true;
}

bool APREnemyProjectile::TryConsumeNearestAttackProxy(const FVector& SegmentStart, const FVector& SegmentEnd)
{
	UWorld* World = GetWorld();
	AActor* Target = TargetAvatar.Get();
	APREnemyCharacter* Source = SourceEnemy.Get();
	if (!World || !Target || !Source)
	{
		return false;
	}
	const FVector Segment = SegmentEnd - SegmentStart;
	const float LengthSquared = Segment.SizeSquared();
	if (LengthSquared <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	TArray<PREnemyProjectile::FProxyCandidate> Candidates;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* CandidateActor = *It;
		if (!IsValid(CandidateActor))
		{
			continue;
		}
		IPRCombatAttackProxyInterface* Proxy = Cast<IPRCombatAttackProxyInterface>(CandidateActor);
		if (!Proxy || Proxy->GetAttackProxyOwner() != Target || Proxy->GetAttackProxyId().IsNone())
		{
			continue;
		}
		const float Alpha = FMath::Clamp(FVector::DotProduct(CandidateActor->GetActorLocation() - SegmentStart, Segment) / LengthSquared, 0.0f, 1.0f);
		Candidates.Add({CandidateActor, Proxy, Alpha, Proxy->GetAttackProxyId()});
	}
	Candidates.Sort([](const PREnemyProjectile::FProxyCandidate& Left, const PREnemyProjectile::FProxyCandidate& Right)
	{
		return !FMath::IsNearlyEqual(Left.Alpha, Right.Alpha)
			? Left.Alpha < Right.Alpha
			: Left.Id.LexicalLess(Right.Id);
	});
	for (const PREnemyProjectile::FProxyCandidate& Candidate : Candidates)
	{
		if (Candidate.Proxy && Candidate.Proxy->ConsumeAttackProxy(Source))
		{
			FinishProjectile(true);
			return true;
		}
	}
	return false;
}

bool APREnemyProjectile::ResolveTargetHit(const FHitResult& Impact)
{
	if (!TryResolveAttackToken())
	{
		UE_LOG(LogProjectR, Warning, TEXT("Enemy projectile could not resolve its one-shot attack token."));
		FinishProjectile(false);
		return false;
	}
	APREnemyCharacter* Source = SourceEnemy.Get();
	AActor* Target = TargetAvatar.Get();
	UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	if (!Source || !Target || !Combat)
	{
		FinishProjectile(false);
		return false;
	}
	FPRDamageRequest Request;
	Request.SourceId = AttackId;
	Request.DamageSource = this;
	Request.Instigator = Source;
	Request.Target = Target;
	Request.AbilityTag = AbilityTag;
	Request.RawDamage = RawDamage;
	Request.bCritical = false;
	Request.DamageTags = DamageTags;
	Request.ImpactOrigin = Impact.ImpactPoint;
	Request.IncomingDirection = LockedDirection;
	const EPRCombatRequestStatus DamageStatus = Combat->ApplyDamage(Request);
	if (DamageStatus != EPRCombatRequestStatus::Applied)
	{
		UE_LOG(LogProjectR, Warning, TEXT("Enemy projectile CombatSubsystem result was %d."), static_cast<int32>(DamageStatus));
	}
	FinishProjectile(false);
	return true;
}

bool APREnemyProjectile::IsRuntimeValid() const
{
	const APREnemyCharacter* Source = SourceEnemy.Get();
	AActor* Target = TargetAvatar.Get();
	const UWorld* World = GetWorld();
	if (!bInitialized || !HasAuthority() || !Source || !Target || !World || Source->IsEnemyDead())
	{
		return false;
	}
	const UPRAbilitySystemComponent* SourceASC = Source->GetProjectRAbilitySystemComponent();
	if (!SourceASC || SourceASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()))
	{
		return false;
	}
	const APlayerController* Controller = World->GetFirstPlayerController();
	if (!Controller || Controller->GetPawn() != Target || !Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		return false;
	}
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Target);
	const UPRAbilitySystemComponent* TargetASC = AbilityInterface
		? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent()) : nullptr;
	if (!TargetASC || TargetASC->GetAvatarActor() != Target
		|| TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
		|| TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()))
	{
		return false;
	}
	const UPREnemyPrototypeDataAsset* Prototype = Source->GetEnemyPrototype();
	return Prototype && FMath::Abs(Target->GetActorLocation().X - Source->GetActorLocation().X) <= Prototype->Perception.LoseRange;
}

bool APREnemyProjectile::TryResolveAttackToken()
{
	if (bResolved)
	{
		return false;
	}
	if (UPREnemyBrainComponent* Brain = SourceEnemy.IsValid() ? SourceEnemy->GetEnemyBrain() : nullptr)
	{
		bResolved = Brain->TryResolveProjectileToken(AttackToken);
	}
	return bResolved;
}

void APREnemyProjectile::ReleaseAttackToken()
{
	if (UPREnemyBrainComponent* Brain = SourceEnemy.IsValid() ? SourceEnemy->GetEnemyBrain() : nullptr)
	{
		Brain->ReleaseProjectileToken(AttackToken);
	}
}

void APREnemyProjectile::FinishProjectile(const bool bConsumeToken)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	if (bConsumeToken)
	{
		TryResolveAttackToken();
	}
	if (!bResolved)
	{
		ReleaseAttackToken();
	}
	Destroy();
}
