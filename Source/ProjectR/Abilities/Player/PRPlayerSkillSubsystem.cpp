// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillSubsystem.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "ProjectR.h"
#include "TimerManager.h"

namespace PRPlayerSkillTargeting
{
constexpr float PlaneTolerance = 1.0f;
constexpr float DestinationTolerance = 2.0f;
constexpr float ShadowPathRetreat = 1.0f;
constexpr uint16 RootMotionPriority = 500;
constexpr float CriticalMultiplier = 1.5f;

bool IsFiniteVector(const FVector& Value)
{
	return !Value.ContainsNaN();
}

FVector ToPlanar(const FVector& Value)
{
	return FVector(Value.X, 0.0f, Value.Z);
}

bool HasLineOfSight(const UWorld& World, const AActor& Source, const AActor& Target, const FVector End)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRPlayerSkillLineOfSight), false, &Source);
	FHitResult Hit;
	const bool bHit = World.LineTraceSingleByChannel(
		Hit,
		Source.GetActorLocation(),
		End,
		ECC_Visibility,
		Params);
	return !bHit || Hit.GetActor() == &Target;
}

struct FCandidate
{
	TWeakObjectPtr<AActor> Actor;
	FName TargetId;
	float DistanceSquared = 0.0f;
	float AngleDegrees = 0.0f;
};
} // namespace PRPlayerSkillTargeting

void UPRPlayerSkillSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
		this,
		&UPRPlayerSkillSubsystem::HandleWorldCleanup);
}

void UPRPlayerSkillSubsystem::Deinitialize()
{
	CleanupAllDisplacements();
	CleanupAllBurning(true);
	CleanupAllThunderDrops();
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}
	Super::Deinitialize();
}

bool UPRPlayerSkillSubsystem::QueryTargets(
	const FPRAbilityTargetQuery& Query,
	FPRAbilityTargetQueryResult& OutResult,
	FGameplayTag& OutFailureTag) const
{
	using namespace PRPlayerSkillTargeting;
	OutResult = FPRAbilityTargetQueryResult();
	OutFailureTag = FGameplayTag();
	UWorld* World = GetWorld();
	AActor* Source = Query.SourceActor.Get();
	if (!World || !IsValid(Source) || Source->GetWorld() != World || !Source->HasAuthority()
		|| !Query.AbilityTag.IsValid() || !Query.AbilityTag.ToString().StartsWith(TEXT("Skill."))
		|| !IsFiniteVector(Query.Origin) || !IsFiniteVector(Query.AimDirection)
		|| !IsFiniteVector(Query.AreaCenter) || !FMath::IsFinite(Query.Range)
		|| !FMath::IsFinite(Query.Radius) || !FMath::IsFinite(Query.HalfAngleDegrees)
		|| Query.Range < 0.0f || Query.Radius < 0.0f
		|| Query.HalfAngleDegrees < 0.0f || Query.HalfAngleDegrees > 180.0f
		|| FMath::Abs(Query.Origin.Y - Source->GetActorLocation().Y) > PlaneTolerance)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::Self)
	{
		OutResult.PrimaryTarget = Source;
		OutResult.Targets.Add(Source);
		OutResult.ResolvedPoint = Source->GetActorLocation();
		return true;
	}

	FVector PlanarAim = ToPlanar(Query.AimDirection);
	if (!PlanarAim.Normalize() || Query.Range <= 0.0f)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	FVector QueryCenter = Query.Origin;
	TArray<FOverlapResult> Overlaps;
	TArray<FHitResult> SweepHits;
	FCollisionObjectQueryParams TargetObjects;
	TargetObjects.AddObjectTypesToQuery(ECC_Pawn);
	TargetObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRPlayerSkillTargetQuery), false, Source);
	bool bQuerySucceeded = false;
	float PathBlockDistance = TNumericLimits<float>::Max();
	bool bPathBlocked = false;

	if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::ForwardSweep)
	{
		if (Query.Radius <= 0.0f)
		{
			OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
			return false;
		}
		const FVector End = Query.Origin + PlanarAim * Query.Range;
		bQuerySucceeded = World->SweepMultiByObjectType(
			SweepHits,
			Query.Origin,
			End,
			FQuat::Identity,
			TargetObjects,
			FCollisionShape::MakeSphere(Query.Radius),
			Params);

		FCollisionObjectQueryParams WorldStaticObjects;
		WorldStaticObjects.AddObjectTypesToQuery(ECC_WorldStatic);
		FHitResult BlockingHit;
		bPathBlocked = World->SweepSingleByObjectType(
			BlockingHit,
			Query.Origin,
			End,
			FQuat::Identity,
			WorldStaticObjects,
			FCollisionShape::MakeSphere(Query.Radius),
			Params);
		if (bPathBlocked)
		{
			PathBlockDistance = BlockingHit.Distance;
		}
		OutResult.ResolvedPoint = bPathBlocked ? BlockingHit.Location : End;
	}
	else
	{
		if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::GroundArea)
		{
			QueryCenter = Query.AreaCenter;
			if (FMath::Abs(QueryCenter.Y - Source->GetActorLocation().Y) > PlaneTolerance
				|| ToPlanar(QueryCenter - Query.Origin).SizeSquared() > FMath::Square(Query.Range))
			{
				OutFailureTag = UPRTagLibrary::GetAbilityActivateFailObstructedTag();
				return false;
			}
			QueryCenter.Y = Source->GetActorLocation().Y;
			FHitResult GroundPathHit;
			FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(PRPlayerSkillGroundPath), false, Source);
			if (World->LineTraceSingleByChannel(
				GroundPathHit,
				Query.Origin,
				QueryCenter,
				ECC_Visibility,
				GroundParams))
			{
				OutFailureTag = UPRTagLibrary::GetAbilityActivateFailObstructedTag();
				return false;
			}
			OutResult.ResolvedPoint = QueryCenter;
		}
		const float OverlapRadius = Query.TargetPolicy == EPRPlayerSkillTargetPolicy::GroundArea
			? Query.Radius
			: Query.Range;
		if (OverlapRadius <= 0.0f)
		{
			OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
			return false;
		}
		bQuerySucceeded = World->OverlapMultiByObjectType(
			Overlaps,
			QueryCenter,
			FQuat::Identity,
			TargetObjects,
			FCollisionShape::MakeSphere(OverlapRadius),
			Params);
	}

	TSet<TWeakObjectPtr<AActor>> SeenActors;
	TSet<FName> SeenIds;
	TArray<FCandidate> Candidates;
	bool bHadObstructedCandidate = bPathBlocked;
	auto ConsiderActor = [&](AActor* Actor)
	{
		if (!IsValid(Actor) || Actor == Source || SeenActors.Contains(Actor))
		{
			return;
		}
		SeenActors.Add(Actor);
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Actor);
		if (!TargetInterface || !TargetInterface->CanBeTargetedByAbility(Query.AbilityTag))
		{
			return;
		}
		const FName TargetId = TargetInterface->GetAbilityTargetId();
		const FVector TargetPoint = TargetInterface->GetAbilityTargetPoint();
		if (TargetId.IsNone() || !IsFiniteVector(TargetPoint)
			|| FMath::Abs(TargetPoint.Y - Source->GetActorLocation().Y) > PlaneTolerance)
		{
			return;
		}

		if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Actor))
		{
			const UAbilitySystemComponent* ASC = AbilityInterface->GetAbilitySystemComponent();
			if (ASC && ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
			{
				return;
			}
		}

		const FVector Delta = ToPlanar(TargetPoint - Query.Origin);
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared > FMath::Square(Query.Range))
		{
			return;
		}
		const float Distance = FMath::Sqrt(DistanceSquared);
		if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::ForwardSweep
			&& Distance > PathBlockDistance)
		{
			bHadObstructedCandidate = true;
			return;
		}
		const FVector Direction = Distance > UE_SMALL_NUMBER ? Delta / Distance : PlanarAim;
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(
			FMath::Clamp(FVector::DotProduct(PlanarAim, Direction), -1.0f, 1.0f)));
		if (Query.TargetPolicy != EPRPlayerSkillTargetPolicy::GroundArea
			&& Query.TargetPolicy != EPRPlayerSkillTargetPolicy::ForwardSweep
			&& Angle > Query.HalfAngleDegrees)
		{
			return;
		}
		if (!HasLineOfSight(*World, *Source, *Actor, TargetPoint))
		{
			bHadObstructedCandidate = true;
			return;
		}
		if (SeenIds.Contains(TargetId))
		{
			UE_LOG(LogProjectR, Error, TEXT("Player skill target query found duplicate TargetId %s."),
				*TargetId.ToString());
			Candidates.Reset();
			SeenIds.Add(NAME_None);
			return;
		}
		SeenIds.Add(TargetId);
		Candidates.Add({Actor, TargetId, DistanceSquared, Angle});
	};

	if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::ForwardSweep)
	{
		for (const FHitResult& Hit : SweepHits)
		{
			ConsiderActor(Hit.GetActor());
		}
	}
	else
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			ConsiderActor(Overlap.GetActor());
		}
	}

	if (SeenIds.Contains(NAME_None))
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailNoTargetTag();
		return false;
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (!FMath::IsNearlyEqual(A.DistanceSquared, B.DistanceSquared))
		{
			return A.DistanceSquared < B.DistanceSquared;
		}
		if (!FMath::IsNearlyEqual(A.AngleDegrees, B.AngleDegrees))
		{
			return A.AngleDegrees < B.AngleDegrees;
		}
		return A.TargetId.ToString() < B.TargetId.ToString();
	});

	if (Query.TargetPolicy == EPRPlayerSkillTargetPolicy::GroundArea && Candidates.IsEmpty())
	{
		return true;
	}
	if (!bQuerySucceeded || Candidates.IsEmpty())
	{
		OutFailureTag = bHadObstructedCandidate
			? UPRTagLibrary::GetAbilityActivateFailObstructedTag()
			: UPRTagLibrary::GetAbilityActivateFailNoTargetTag();
		return false;
	}

	const int32 ResultCount = Query.TargetPolicy == EPRPlayerSkillTargetPolicy::SingleTarget
		? 1
		: Candidates.Num();
	for (int32 Index = 0; Index < ResultCount; ++Index)
	{
		OutResult.Targets.Add(Candidates[Index].Actor);
	}
	OutResult.PrimaryTarget = OutResult.Targets[0];
	if (Query.TargetPolicy != EPRPlayerSkillTargetPolicy::GroundArea)
	{
		OutResult.ResolvedPoint = Cast<IPRAbilityTargetInterface>(OutResult.PrimaryTarget.Get())
			->GetAbilityTargetPoint();
	}
	return true;
}

bool UPRPlayerSkillSubsystem::RequestDisplacement(
	const FPRAbilityDisplacementRequest& Request,
	FGuid& OutRequestId,
	FGameplayTag& OutFailureTag)
{
	using namespace PRPlayerSkillTargeting;
	OutRequestId.Invalidate();
	OutFailureTag = FGameplayTag();
	AActor* Source = Request.SourceActor.Get();
	ACharacter* Target = Cast<ACharacter>(Request.TargetActor.Get());
	UWorld* World = GetWorld();
	if (!World || !IsValid(Source) || !IsValid(Target)
		|| Source->GetWorld() != World || Target->GetWorld() != World
		|| !Source->HasAuthority() || !Target->HasAuthority()
		|| !Request.AbilityTag.IsValid() || !Request.AbilityTag.ToString().StartsWith(TEXT("Skill."))
		|| !IsFiniteVector(Request.StartLocation) || !IsFiniteVector(Request.DesiredEndLocation)
		|| !FMath::IsFinite(Request.Duration) || Request.Duration <= 0.0f
		|| !FMath::IsFinite(Request.StopDistance) || Request.StopDistance < 0.0f
		|| !Request.bSweep || !Request.bStopOnBlockingHit
		|| !Target->GetActorLocation().Equals(Request.StartLocation, PlaneTolerance)
		|| FMath::Abs(Request.StartLocation.Y - Request.DesiredEndLocation.Y) > PlaneTolerance
		|| FMath::Abs(Request.StartLocation.Y - Source->GetActorLocation().Y) > PlaneTolerance)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	for (const TPair<FGuid, FActiveDisplacement>& Pair : ActiveDisplacements)
	{
		if (Pair.Value.TargetCharacter == Target)
		{
			OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
			return false;
		}
	}

	FVector Travel = ToPlanar(Request.DesiredEndLocation - Request.StartLocation);
	const float Distance = Travel.Size();
	if (Distance <= Request.StopDistance + UE_SMALL_NUMBER || !Travel.Normalize())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}
	FVector EffectiveEnd = Request.DesiredEndLocation - Travel * Request.StopDistance;
	EffectiveEnd.Y = Request.StartLocation.Y;
	if (HasWorldStaticObstruction(*Target, Request.StartLocation, EffectiveEnd, Source))
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailObstructedTag();
		return false;
	}

	UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
	if (!Movement || !Target->GetCapsuleComponent())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	const FGuid RequestId = FGuid::NewGuid();
	TSharedPtr<FRootMotionSource_MoveToForce> RootMotion = MakeShared<FRootMotionSource_MoveToForce>();
	RootMotion->InstanceName = FName(*FString::Printf(TEXT("PRSkill.%s"), *RequestId.ToString(EGuidFormats::Digits)));
	RootMotion->Priority = RootMotionPriority;
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Duration = Request.Duration;
	RootMotion->StartLocation = Request.StartLocation;
	RootMotion->TargetLocation = EffectiveEnd;
	RootMotion->bRestrictSpeedToExpected = true;
	const uint16 RootMotionId = Movement->ApplyRootMotionSource(RootMotion);
	if (RootMotionId == static_cast<uint16>(ERootMotionSourceID::Invalid))
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	FActiveDisplacement Job;
	Job.RequestId = RequestId;
	Job.AbilityTag = Request.AbilityTag;
	Job.SourceActor = Source;
	Job.TargetCharacter = Target;
	Job.PlaneOrigin = Request.StartLocation;
	Job.EffectiveEnd = EffectiveEnd;
	Job.RootMotionSourceId = RootMotionId;
	World->GetTimerManager().SetTimer(
		Job.MonitorTimerHandle,
		FTimerDelegate::CreateUObject(this, &UPRPlayerSkillSubsystem::HandleDisplacementMonitor, RequestId),
		1.0f / 120.0f,
		true);
	World->GetTimerManager().SetTimer(
		Job.CompletionTimerHandle,
		FTimerDelegate::CreateUObject(this, &UPRPlayerSkillSubsystem::HandleDisplacementDurationElapsed, RequestId),
		Request.Duration,
		false);
	ActiveDisplacements.Add(RequestId, Job);
	OutRequestId = RequestId;
	return true;
}

bool UPRPlayerSkillSubsystem::CancelDisplacement(const FGuid RequestId)
{
	if (!RequestId.IsValid() || !ActiveDisplacements.Contains(RequestId))
	{
		return false;
	}
	FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Cancelled, true);
	return true;
}

FPRAbilityDisplacementFinishedNative& UPRPlayerSkillSubsystem::OnDisplacementFinished()
{
	return DisplacementFinishedEvent;
}

bool UPRPlayerSkillSubsystem::PublishAbilityOutcome(const FPRCombatOutcomeRequest& Request) const
{
	const UWorld* World = GetWorld();
	UPRCombatSubsystem* Combat = World ? World->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	return Combat && Combat->PublishAbilityOutcome(Request);
}

bool UPRPlayerSkillSubsystem::ResolveShadowPath(
	const ACharacter& Character,
	FVector AimDirection,
	const float DesiredDistance,
	const float PathRadius,
	FVector& OutEndLocation,
	float& OutSafeDistance,
	FGameplayTag& OutFailureTag) const
{
	using namespace PRPlayerSkillTargeting;
	OutEndLocation = Character.GetActorLocation();
	OutSafeDistance = 0.0f;
	OutFailureTag = FGameplayTag();
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	AimDirection = ToPlanar(AimDirection);
	if (!World || !Capsule || Character.GetWorld() != World || !Character.HasAuthority()
		|| !AimDirection.Normalize() || !FMath::IsFinite(DesiredDistance)
		|| !FMath::IsFinite(PathRadius) || DesiredDistance <= 0.0f || PathRadius <= 0.0f)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	const FVector Start = Character.GetActorLocation();
	const FVector DesiredEnd = Start + AimDirection * DesiredDistance;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRShadowThrustPath), false, &Character);
	float BlockingDistance = DesiredDistance;
	auto AccumulateBlockingDistance = [&](const FCollisionShape& Shape)
	{
		TArray<FHitResult> Hits;
		if (!World->SweepMultiByObjectType(
			Hits,
			Start,
			DesiredEnd,
			FQuat::Identity,
			Objects,
			Shape,
			Params))
		{
			return;
		}
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.bBlockingHit && Hit.bStartPenetrating)
			{
				BlockingDistance = 0.0f;
				continue;
			}
			const FVector PlanarNormal = ToPlanar(Hit.ImpactNormal).GetSafeNormal();
			if (Hit.bBlockingHit
				&& FVector::DotProduct(PlanarNormal, AimDirection) < -UE_SMALL_NUMBER)
			{
				BlockingDistance = FMath::Min(BlockingDistance, Hit.Distance);
			}
		}
	};

	AccumulateBlockingDistance(FCollisionShape::MakeSphere(PathRadius));
	AccumulateBlockingDistance(FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight()));
	OutSafeDistance = BlockingDistance < DesiredDistance
		? BlockingDistance - ShadowPathRetreat
		: DesiredDistance;
	if (OutSafeDistance < ShadowPathRetreat)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailObstructedTag();
		return false;
	}
	OutEndLocation = Start + AimDirection * OutSafeDistance;
	OutEndLocation.Y = Start.Y;
	return true;
}

EPRCombatRequestStatus UPRPlayerSkillSubsystem::ApplySkillDamage(
	const FPRPlayerSkillExecutionSnapshot& Snapshot,
	AActor& Target,
	UObject& AbilitySource,
	const FVector ImpactOrigin,
	const float BaseDamage,
	const float AttackPowerScale,
	const bool bCritical,
	FVector DirectionFallback,
	const FName SourceIdOverride) const
{
	AActor* Source = Snapshot.SourceActor.Get();
	const UWorld* World = GetWorld();
	if (!World || !IsValid(Source) || !IsValid(&Target) || !IsValid(&AbilitySource)
		|| Source->GetWorld() != World || Target.GetWorld() != World
		|| !Source->HasAuthority() || !Snapshot.AbilityTag.IsValid()
		|| !FMath::IsFinite(Snapshot.AttackPower) || !FMath::IsFinite(BaseDamage)
		|| !FMath::IsFinite(AttackPowerScale) || BaseDamage < 0.0f || AttackPowerScale < 0.0f
		|| ImpactOrigin.ContainsNaN())
	{
		return EPRCombatRequestStatus::Invalid;
	}

	FName SourceId = SourceIdOverride;
	if (SourceId.IsNone())
	{
		const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Source);
		const UAbilitySystemComponent* ASC = AbilityInterface
			? AbilityInterface->GetAbilitySystemComponent()
			: nullptr;
		const AActor* Owner = ASC ? ASC->GetOwnerActor() : nullptr;
		const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(Owner);
		SourceId = Combatant ? Combatant->GetCombatantId() : NAME_None;
	}
	DirectionFallback = PRPlayerSkillTargeting::ToPlanar(DirectionFallback);
	FVector IncomingDirection = PRPlayerSkillTargeting::ToPlanar(
		Target.GetActorLocation() - Source->GetActorLocation());
	if (!IncomingDirection.Normalize())
	{
		IncomingDirection = DirectionFallback.GetSafeNormal();
	}
	const float CriticalScale = bCritical ? PRPlayerSkillTargeting::CriticalMultiplier : 1.0f;
	const float RawDamage = (BaseDamage + AttackPowerScale * Snapshot.AttackPower) * CriticalScale;
	if (SourceId.IsNone() || !IncomingDirection.IsNormalized()
		|| !FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
	{
		return EPRCombatRequestStatus::Invalid;
	}

	FPRDamageRequest Request;
	Request.SourceId = SourceId;
	Request.DamageSource = &AbilitySource;
	Request.Instigator = Source;
	Request.Target = &Target;
	Request.AbilityTag = Snapshot.AbilityTag;
	Request.RawDamage = RawDamage;
	Request.bCritical = bCritical;
	Request.ImpactOrigin = ImpactOrigin;
	Request.IncomingDirection = IncomingDirection;
	UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>();
	return Combat ? Combat->ApplyDamage(Request) : EPRCombatRequestStatus::Invalid;
}

bool UPRPlayerSkillSubsystem::ApplyOrRefreshBurning(
	const FPRPlayerSkillExecutionSnapshot& Snapshot,
	AActor& Target,
	UObject& AbilitySource,
	const TSubclassOf<UGameplayEffect> BurningEffectClass,
	const UPRFireSlashSkillFragment& Fragment)
{
	AActor* Source = Snapshot.SourceActor.Get();
	const IAbilitySystemInterface* SourceAbilityInterface = Cast<IAbilitySystemInterface>(Source);
	const IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(&Target);
	UPRAbilitySystemComponent* SourceASC = SourceAbilityInterface
		? Cast<UPRAbilitySystemComponent>(SourceAbilityInterface->GetAbilitySystemComponent())
		: nullptr;
	UPRAbilitySystemComponent* TargetASC = TargetAbilityInterface
		? Cast<UPRAbilitySystemComponent>(TargetAbilityInterface->GetAbilitySystemComponent())
		: nullptr;
	AActor* SourceOwner = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	AActor* TargetOwner = TargetASC ? TargetASC->GetOwnerActor() : nullptr;
	const IPRCombatantInterface* SourceCombatant = Cast<IPRCombatantInterface>(SourceOwner);
	const FName SourceId = SourceCombatant ? SourceCombatant->GetCombatantId() : NAME_None;
	if (!GetWorld() || !IsValid(Source) || !IsValid(&Target) || !IsValid(&AbilitySource)
		|| !SourceASC || !TargetASC || !SourceOwner || !TargetOwner || !SourceCombatant
		|| SourceId.IsNone()
		|| SourceASC->GetAvatarActor() != Source || TargetASC->GetAvatarActor() != &Target
		|| Source->GetWorld() != GetWorld() || Target.GetWorld() != GetWorld()
		|| !Source->HasAuthority() || !BurningEffectClass
		|| TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
		|| !FMath::IsFinite(Fragment.BurningBaseDamage)
		|| !FMath::IsFinite(Fragment.BurningAttackPowerScale)
		|| !FMath::IsFinite(Fragment.BurningTickInterval)
		|| Fragment.BurningBaseDamage < 0.0f || Fragment.BurningAttackPowerScale < 0.0f
		|| Fragment.BurningTickInterval <= 0.0f || Fragment.BurningTickCount <= 0)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(&AbilitySource);
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
		BurningEffectClass,
		1.0f,
		Context);
	if (!Spec.IsValid())
	{
		return false;
	}
	const FActiveGameplayEffectHandle NewHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*Spec.Data.Get(),
		TargetASC);
	if (!NewHandle.WasSuccessfullyApplied())
	{
		return false;
	}

	const TWeakObjectPtr<AActor> TargetKey(&Target);
	FActiveBurning* Existing = ActiveBurning.Find(TargetKey);
	if (Existing && Existing->EffectHandle.IsValid()
		&& Existing->EffectHandle != NewHandle)
	{
		if (FOnActiveGameplayEffectRemoved_Info* Removed =
			Existing->TargetASC.IsValid()
				? Existing->TargetASC->OnGameplayEffectRemoved_InfoDelegate(Existing->EffectHandle)
				: nullptr)
		{
			Removed->Remove(Existing->EffectRemovedHandle);
		}
		if (Existing->TargetASC.IsValid())
		{
			Existing->TargetASC->RemoveActiveGameplayEffect(Existing->EffectHandle);
		}
	}

	FActiveBurning& Burning = ActiveBurning.FindOrAdd(TargetKey);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Burning.TimerHandle);
	}
	if (Burning.EffectHandle != NewHandle || !Burning.EffectRemovedHandle.IsValid())
	{
		Burning.EffectRemovedHandle.Reset();
		if (FOnActiveGameplayEffectRemoved_Info* Removed =
			TargetASC->OnGameplayEffectRemoved_InfoDelegate(NewHandle))
		{
			Burning.EffectRemovedHandle = Removed->AddUObject(
				this,
				&UPRPlayerSkillSubsystem::HandleBurningEffectRemoved,
				TargetKey);
		}
	}
	Burning.TargetASC = TargetASC;
	Burning.TargetOwner = TargetOwner;
	Burning.TargetAvatar = &Target;
	Burning.SourceASC = SourceASC;
	Burning.SourceOwner = SourceOwner;
	Burning.SourceAvatar = Source;
	Burning.AbilitySource = &AbilitySource;
	Burning.SourceId = SourceId;
	Burning.AbilityTag = Snapshot.AbilityTag;
	Burning.AttackPower = Snapshot.AttackPower;
	Burning.BaseDamage = Fragment.BurningBaseDamage;
	Burning.AttackPowerScale = Fragment.BurningAttackPowerScale;
	Burning.TickInterval = Fragment.BurningTickInterval;
	Burning.AimFallback = Snapshot.AimDirection;
	Burning.EffectHandle = NewHandle;
	Burning.RemainingTicks = Fragment.BurningTickCount;
	GetWorld()->GetTimerManager().SetTimer(
		Burning.TimerHandle,
		FTimerDelegate::CreateUObject(
			this,
			&UPRPlayerSkillSubsystem::HandleBurningTick,
			TargetKey),
		Burning.TickInterval,
		false);
	return true;
}

void UPRPlayerSkillSubsystem::HandleBurningTick(const TWeakObjectPtr<AActor> TargetAvatar)
{
	ExecuteBurningTick(TargetAvatar, false);
}

void UPRPlayerSkillSubsystem::HandleBurningEffectRemoved(
	const FGameplayEffectRemovalInfo& RemovalInfo,
	const TWeakObjectPtr<AActor> TargetAvatar)
{
	FActiveBurning* Burning = ActiveBurning.Find(TargetAvatar);
	if (!Burning)
	{
		return;
	}
	Burning->EffectHandle.Invalidate();
	Burning->EffectRemovedHandle.Reset();
	if (!RemovalInfo.bPrematureRemoval && Burning->RemainingTicks == 1)
	{
		ExecuteBurningTick(TargetAvatar, true);
		return;
	}
	CleanupBurning(TargetAvatar, false);
}

void UPRPlayerSkillSubsystem::ExecuteBurningTick(
	const TWeakObjectPtr<AActor> TargetAvatar,
	const bool bAllowExpiredEffect)
{
	FActiveBurning* Burning = ActiveBurning.Find(TargetAvatar);
	if (!Burning || Burning->RemainingTicks <= 0)
	{
		CleanupBurning(TargetAvatar, false);
		return;
	}
	if (!IsBurningBindingValid(*Burning))
	{
		CleanupBurning(TargetAvatar, true);
		return;
	}
	if (!bAllowExpiredEffect && !Burning->EffectHandle.IsValid())
	{
		CleanupBurning(TargetAvatar, false);
		return;
	}

	FPRPlayerSkillExecutionSnapshot Snapshot;
	Snapshot.AbilityTag = Burning->AbilityTag;
	Snapshot.SourceActor = Burning->SourceAvatar;
	Snapshot.AimDirection = Burning->AimFallback;
	Snapshot.AttackPower = Burning->AttackPower;
	Snapshot.bCritical = false;
	AActor* Target = Burning->TargetAvatar.Get();
	AActor* Source = Burning->SourceAvatar.Get();
	UObject* AbilitySource = Burning->AbilitySource.Get();
	const EPRCombatRequestStatus Status = ApplySkillDamage(
		Snapshot,
		*Target,
		*AbilitySource,
		Source->GetActorLocation(),
		Burning->BaseDamage,
		Burning->AttackPowerScale,
		false,
		Burning->AimFallback,
		Burning->SourceId);
	if (Status == EPRCombatRequestStatus::Invalid)
	{
		CleanupBurning(TargetAvatar, true);
		return;
	}

	--Burning->RemainingTicks;
	if (Burning->RemainingTicks <= 0
		|| Burning->TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		CleanupBurning(TargetAvatar, true);
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		Burning->TimerHandle,
		FTimerDelegate::CreateUObject(
			this,
			&UPRPlayerSkillSubsystem::HandleBurningTick,
			TargetAvatar),
		Burning->TickInterval,
		false);
}

void UPRPlayerSkillSubsystem::CleanupBurning(
	const TWeakObjectPtr<AActor> TargetAvatar,
	const bool bRemoveEffect)
{
	FActiveBurning* BurningPtr = ActiveBurning.Find(TargetAvatar);
	if (!BurningPtr)
	{
		return;
	}
	FActiveBurning Burning = *BurningPtr;
	ActiveBurning.Remove(TargetAvatar);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Burning.TimerHandle);
	}
	if (Burning.TargetASC.IsValid() && Burning.EffectHandle.IsValid())
	{
		if (FOnActiveGameplayEffectRemoved_Info* Removed =
			Burning.TargetASC->OnGameplayEffectRemoved_InfoDelegate(Burning.EffectHandle))
		{
			Removed->Remove(Burning.EffectRemovedHandle);
		}
		if (bRemoveEffect)
		{
			Burning.TargetASC->RemoveActiveGameplayEffect(Burning.EffectHandle);
		}
	}
}

void UPRPlayerSkillSubsystem::CleanupAllBurning(const bool bRemoveEffects)
{
	TArray<TWeakObjectPtr<AActor>> Targets;
	ActiveBurning.GetKeys(Targets);
	for (const TWeakObjectPtr<AActor>& Target : Targets)
	{
		CleanupBurning(Target, bRemoveEffects);
	}
}

bool UPRPlayerSkillSubsystem::IsBurningBindingValid(const FActiveBurning& Burning) const
{
	return IsValid(Burning.TargetASC.Get()) && IsValid(Burning.SourceASC.Get())
		&& IsValid(Burning.TargetOwner.Get()) && IsValid(Burning.SourceOwner.Get())
		&& IsValid(Burning.TargetAvatar.Get()) && IsValid(Burning.SourceAvatar.Get())
		&& IsValid(Burning.AbilitySource.Get())
		&& Burning.TargetASC->GetOwnerActor() == Burning.TargetOwner.Get()
		&& Burning.TargetASC->GetAvatarActor() == Burning.TargetAvatar.Get()
		&& Burning.SourceASC->GetOwnerActor() == Burning.SourceOwner.Get()
		&& Burning.SourceASC->GetAvatarActor() == Burning.SourceAvatar.Get()
		&& Burning.TargetAvatar->GetWorld() == GetWorld()
		&& Burning.SourceAvatar->GetWorld() == GetWorld()
		&& !Burning.TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
}

void UPRPlayerSkillSubsystem::HandleDisplacementMonitor(const FGuid RequestId)
{
	const FActiveDisplacement* Job = ActiveDisplacements.Find(RequestId);
	if (!Job)
	{
		return;
	}
	ACharacter* Target = Job->TargetCharacter.Get();
	AActor* Source = Job->SourceActor.Get();
	if (!IsValid(Target) || !IsValid(Source)
		|| FMath::Abs(Target->GetActorLocation().Y - Job->PlaneOrigin.Y) > PRPlayerSkillTargeting::PlaneTolerance)
	{
		FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Cancelled, true);
		return;
	}

	if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Target))
	{
		const UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(
			AbilityInterface->GetAbilitySystemComponent());
		if (ASC && (ASC->GetAvatarActor() != Target
			|| ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
			|| ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())))
		{
			FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Cancelled, true);
			return;
		}
	}

	if (PRPlayerSkillTargeting::ToPlanar(Job->EffectiveEnd - Target->GetActorLocation()).Size()
		<= PRPlayerSkillTargeting::DestinationTolerance)
	{
		FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Completed, true);
		return;
	}
	if (HasWorldStaticObstruction(*Target, Target->GetActorLocation(), Job->EffectiveEnd, Source))
	{
		FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Blocked, true);
	}
}

void UPRPlayerSkillSubsystem::HandleDisplacementDurationElapsed(const FGuid RequestId)
{
	const FActiveDisplacement* Job = ActiveDisplacements.Find(RequestId);
	const ACharacter* Target = Job ? Job->TargetCharacter.Get() : nullptr;
	const bool bReached = Target
		&& PRPlayerSkillTargeting::ToPlanar(Job->EffectiveEnd - Target->GetActorLocation()).Size()
			<= PRPlayerSkillTargeting::DestinationTolerance;
	FinishDisplacement(
		RequestId,
		bReached ? EPRAbilityDisplacementEndReason::Completed : EPRAbilityDisplacementEndReason::Blocked,
		true);
}

void UPRPlayerSkillSubsystem::HandleWorldCleanup(
	UWorld* World,
	const bool bSessionEnded,
	const bool bCleanupResources)
{
	if (World == GetWorld())
	{
		CleanupAllDisplacements();
		CleanupAllBurning(true);
		CleanupAllThunderDrops();
	}
}

bool UPRPlayerSkillSubsystem::ScheduleThunderDrop(
	const FPRPlayerSkillExecutionSnapshot& Snapshot,
	const FVector ImpactPoint,
	UPRPlayerSkillDataAsset& SkillData,
	const TSubclassOf<UGameplayEffect> StunnedEffectClass)
{
	AActor* Source = Snapshot.SourceActor.Get();
	UWorld* World = GetWorld();
	if (!World || !IsValid(Source) || Source->GetWorld() != World || !Source->HasAuthority()
		|| !Snapshot.AbilityTag.IsValid() || Snapshot.AbilityTag != SkillData.AbilityTag
		|| ImpactPoint.ContainsNaN() || !StunnedEffectClass
		|| !FMath::IsFinite(SkillData.ActiveDuration) || SkillData.ActiveDuration <= 0.0f)
	{
		return false;
	}

	const TWeakObjectPtr<UPRPlayerSkillDataAsset> Key(&SkillData);
	if (ActiveThunderDrops.Contains(Key))
	{
		return false;
	}
	FActiveThunderDrop Active;
	Active.Snapshot = Snapshot;
	Active.ImpactPoint = ImpactPoint;
	Active.SkillData = &SkillData;
	Active.StunnedEffectClass = StunnedEffectClass;
	World->GetTimerManager().SetTimer(
		Active.TimerHandle,
		FTimerDelegate::CreateUObject(this, &UPRPlayerSkillSubsystem::ResolveThunderDrop, Key),
		SkillData.ActiveDuration,
		false);
	ActiveThunderDrops.Add(Key, Active);
	return true;
}

void UPRPlayerSkillSubsystem::ResolveThunderDrop(const TWeakObjectPtr<UPRPlayerSkillDataAsset> SkillDataKey)
{
	FActiveThunderDrop* ActivePtr = ActiveThunderDrops.Find(SkillDataKey);
	if (!ActivePtr)
	{
		return;
	}
	const FActiveThunderDrop Active = *ActivePtr;
	ActiveThunderDrops.Remove(SkillDataKey);
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle = Active.TimerHandle;
		World->GetTimerManager().ClearTimer(TimerHandle);
	}

	UPRPlayerSkillDataAsset* SkillData = Active.SkillData.Get();
	AActor* Source = Active.Snapshot.SourceActor.Get();
	if (!SkillData || !IsValid(Source) || !Source->HasAuthority() || Source->GetWorld() != GetWorld()
		|| !Active.StunnedEffectClass || Active.ImpactPoint.ContainsNaN())
	{
		return;
	}

	FPRAbilityTargetQuery Query;
	Query.SourceActor = Source;
	Query.AbilityTag = Active.Snapshot.AbilityTag;
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::GroundArea;
	Query.Origin = Active.Snapshot.Origin;
	Query.AimDirection = Active.Snapshot.AimDirection;
	Query.AreaCenter = Active.ImpactPoint;
	Query.Range = SkillData->Range;
	Query.Radius = SkillData->Radius;
	FPRAbilityTargetQueryResult Targets;
	FGameplayTag FailureTag;
	if (!QueryTargets(Query, Targets, FailureTag))
	{
		return;
	}

	TSet<FName> DamagedTargetIds;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Targets.Targets)
	{
		AActor* Target = TargetPtr.Get();
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target);
		if (!IsValid(Target) || !TargetInterface || TargetInterface->GetAbilityTargetId().IsNone()
			|| DamagedTargetIds.Contains(TargetInterface->GetAbilityTargetId()))
		{
			continue;
		}
		DamagedTargetIds.Add(TargetInterface->GetAbilityTargetId());
		const EPRCombatRequestStatus Status = ApplySkillDamage(
			Active.Snapshot,
			*Target,
			*SkillData,
			Active.ImpactPoint,
			Active.Snapshot.BaseDamage,
			Active.Snapshot.AttackPowerScale,
			Active.Snapshot.bCritical,
			Active.Snapshot.AimDirection);
		if (Status == EPRCombatRequestStatus::Applied)
		{
			ApplyStunnedEffect(*Source, *Target, *SkillData, Active.StunnedEffectClass);
		}
	}
}

void UPRPlayerSkillSubsystem::CleanupAllThunderDrops()
{
	for (const TPair<TWeakObjectPtr<UPRPlayerSkillDataAsset>, FActiveThunderDrop>& Pair : ActiveThunderDrops)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle = Pair.Value.TimerHandle;
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
	ActiveThunderDrops.Reset();
}

bool UPRPlayerSkillSubsystem::ApplyStunnedEffect(
	AActor& Source,
	AActor& Target,
	UPRPlayerSkillDataAsset& SkillData,
	const TSubclassOf<UGameplayEffect> StunnedEffectClass) const
{
	const IAbilitySystemInterface* SourceInterface = Cast<IAbilitySystemInterface>(&Source);
	const IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(&Target);
	UPRAbilitySystemComponent* SourceASC = SourceInterface
		? Cast<UPRAbilitySystemComponent>(SourceInterface->GetAbilitySystemComponent()) : nullptr;
	UPRAbilitySystemComponent* TargetASC = TargetInterface
		? Cast<UPRAbilitySystemComponent>(TargetInterface->GetAbilitySystemComponent()) : nullptr;
	if (!SourceASC || !TargetASC || !StunnedEffectClass
		|| TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		return false;
	}
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(&SkillData);
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(StunnedEffectClass, 1.0f, Context);
	return Spec.IsValid() && SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC).WasSuccessfullyApplied();
}

void UPRPlayerSkillSubsystem::FinishDisplacement(
	const FGuid RequestId,
	const EPRAbilityDisplacementEndReason EndReason,
	const bool bBroadcast)
{
	FActiveDisplacement* JobPtr = ActiveDisplacements.Find(RequestId);
	if (!JobPtr)
	{
		return;
	}
	FActiveDisplacement Job = *JobPtr;
	ActiveDisplacements.Remove(RequestId);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Job.CompletionTimerHandle);
		World->GetTimerManager().ClearTimer(Job.MonitorTimerHandle);
	}
	ACharacter* Target = Job.TargetCharacter.Get();
	if (Target)
	{
		if (UCharacterMovementComponent* Movement = Target->GetCharacterMovement())
		{
			if (Job.RootMotionSourceId != static_cast<uint16>(ERootMotionSourceID::Invalid))
			{
				Movement->RemoveRootMotionSourceByID(Job.RootMotionSourceId);
			}
			// MoveToForce leaves its final velocity on CharacterMovement after the
			// source is removed. Clear that forced velocity so Recovery cannot drift
			// beyond the validated endpoint (or continue after block/cancel cleanup).
			Movement->StopMovementImmediately();
		}
	}
	if (bBroadcast)
	{
		FPRAbilityDisplacementResult Result;
		Result.RequestId = RequestId;
		Result.AbilityTag = Job.AbilityTag;
		Result.SourceActor = Job.SourceActor;
		Result.TargetActor = Job.TargetCharacter;
		Result.FinalLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
		Result.EndReason = EndReason;
		DisplacementFinishedEvent.Broadcast(Result);
	}
}

bool UPRPlayerSkillSubsystem::HasWorldStaticObstruction(
	const ACharacter& Character,
	const FVector Start,
	const FVector End,
	const AActor* SourceActor) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return true;
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRPlayerSkillDisplacementPath), false, &Character);
	if (SourceActor && SourceActor != &Character)
	{
		Params.AddIgnoredActor(SourceActor);
	}
	FHitResult Hit;
	return World->SweepSingleByObjectType(
		Hit,
		Start,
		End,
		FQuat::Identity,
		Objects,
		FCollisionShape::MakeCapsule(
			Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight()),
		Params);
}

void UPRPlayerSkillSubsystem::CleanupAllDisplacements()
{
	TArray<FGuid> RequestIds;
	ActiveDisplacements.GetKeys(RequestIds);
	for (const FGuid& RequestId : RequestIds)
	{
		FinishDisplacement(RequestId, EPRAbilityDisplacementEndReason::Cancelled, false);
	}
}
