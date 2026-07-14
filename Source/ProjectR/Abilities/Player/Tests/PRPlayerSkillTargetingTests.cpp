// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Core/PRTagLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

namespace PRPlayerSkillTargetAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

class FScopedWorld
{
public:
	FScopedWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FScopedWorld()
	{
		if (World)
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}

	UWorld* Get() const { return World; }
	void Tick(const float DeltaSeconds) const
	{
		// Automation tests execute within one engine frame. TimerManager deliberately
		// ticks at most once per GFrameCounter, so advance the synthetic frame here.
		++GFrameCounter;
		World->Tick(ELevelTick::LEVELTICK_All, DeltaSeconds);
	}

private:
	TObjectPtr<UWorld> World;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillTargetingTest,
	"ProjectR.PlayerSkill.Runtime.TargetSelectionAndDisplacement",
	PRPlayerSkillTargetAutomation::TestFlags)

bool FPRPlayerSkillTargetingTest::RunTest(const FString& Parameters)
{
	PRPlayerSkillTargetAutomation::FScopedWorld World;
	UPRPlayerSkillSubsystem* Subsystem = World.Get()->GetSubsystem<UPRPlayerSkillSubsystem>();
	if (!TestNotNull(TEXT("Skill subsystem exists"), Subsystem))
	{
		return false;
	}

	APRPlayerSkillTestTarget* Source = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	APRPlayerSkillTestTarget* Near = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	APRPlayerSkillTestTarget* Far = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	APRPlayerSkillTestTarget* Friendly = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	if (!TestNotNull(TEXT("Source spawned"), Source)
		|| !TestNotNull(TEXT("Near target spawned"), Near)
		|| !TestNotNull(TEXT("Far target spawned"), Far)
		|| !TestNotNull(TEXT("Friendly target spawned"), Friendly))
	{
		return false;
	}

	Source->SetActorLocation(FVector::ZeroVector);
	Near->SetActorLocation(FVector(100.0f, 0.0f, 50.0f));
	Near->ConfigureTarget(TEXT("Target.A"), true);
	Far->SetActorLocation(FVector(200.0f, 0.0f, -50.0f));
	Far->ConfigureTarget(TEXT("Target.B"), true);
	Friendly->SetActorLocation(FVector(150.0f, 0.0f, 140.0f));
	Friendly->ConfigureTarget(TEXT("Target.Friendly"), false);

	FPRAbilityTargetQuery Query;
	Query.SourceActor = Source;
	Query.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardArea;
	Query.Origin = Source->GetActorLocation();
	Query.AimDirection = FVector::ForwardVector;
	Query.Range = 300.0f;
	Query.HalfAngleDegrees = 45.0f;

	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	TestTrue(TEXT("Forward area query succeeds"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestFalse(TEXT("Successful query has no failure tag"), FailureTag.IsValid());
	TestEqual(TEXT("Friendly target filtered"), Result.Targets.Num(), 2);
	if (Result.Targets.Num() == 2)
	{
		TestEqual(TEXT("Near target sorts first"), Result.Targets[0].Get(), static_cast<AActor*>(Near));
		TestEqual(TEXT("Far target sorts second"), Result.Targets[1].Get(), static_cast<AActor*>(Far));
	}

	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::SingleTarget;
	TestTrue(TEXT("Single target query succeeds"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Single target selects deterministic first"), Result.Targets.Num(), 1);
	if (Result.Targets.Num() == 1)
	{
		TestEqual(TEXT("Single target is Near"), Result.Targets[0].Get(), static_cast<AActor*>(Near));
	}

	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::Self;
	TestTrue(TEXT("Self query succeeds"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Self query returns source"), Result.PrimaryTarget.Get(), static_cast<AActor*>(Source));

	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::GroundArea;
	Query.AreaCenter = FVector(100.0f, 0.0f, 100.0f);
	Query.Range = 300.0f;
	Query.Radius = 60.0f;
	TestTrue(TEXT("Ground area resolves a legal point"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Ground area projects to source Y plane"), Result.ResolvedPoint.Y, Source->GetActorLocation().Y);
	TestTrue(TEXT("Ground area includes Near"), Result.Targets.Contains(Near));
	Query.AreaCenter = FVector(300.0f, 0.0f, 0.0f);
	Query.Radius = 10.0f;
	TestTrue(TEXT("Legal empty ground area succeeds"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Legal empty ground area has no targets"), Result.Targets.Num(), 0);
	Query.AreaCenter.Y = 5.0f;
	TestFalse(TEXT("Cross-plane ground area is rejected"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Cross-plane ground area is obstructed"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailObstructedTag());

	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardArea;
	Query.AreaCenter = FVector::ZeroVector;
	Query.Range = 300.0f;
	Query.Radius = 0.0f;
	Far->ConfigureTarget(TEXT("Target.A"), true);
	AddExpectedError(TEXT("duplicate TargetId Target.A"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Duplicate target IDs fail the entire query"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Duplicate ID failure is NoTarget"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailNoTargetTag());
	Far->ConfigureTarget(TEXT("Target.B"), true);

	Friendly->SetTestCollisionObjectType(ECC_WorldStatic);
	Friendly->SetActorLocation(FVector(50.0f, 0.0f, 25.0f));
	Far->ConfigureTarget(TEXT("Target.B"), false);
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::SingleTarget;
	TestFalse(TEXT("Visibility blocker rejects the only target"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Visibility blocker reports Obstructed"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailObstructedTag());
	Friendly->SetTestCollisionObjectType(ECC_WorldDynamic);
	Friendly->SetActorLocation(FVector(150.0f, 0.0f, 140.0f));
	Far->ConfigureTarget(TEXT("Target.B"), true);

	Near->SetActorLocation(FVector(100.0f, 5.0f, 50.0f));
	Far->ConfigureTarget(TEXT("Target.B"), false);
	TestFalse(TEXT("Target outside source Y plane is filtered"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Plane-filtered target reports NoTarget"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailNoTargetTag());
	Near->SetActorLocation(FVector(100.0f, 0.0f, 50.0f));
	Far->ConfigureTarget(TEXT("Target.B"), true);

	Query.AimDirection = FVector::ZeroVector;
	TestFalse(TEXT("Zero planar aim is rejected"), Subsystem->QueryTargets(Query, Result, FailureTag));
	TestEqual(TEXT("Invalid aim failure tag"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag());

	APRPlayerSkillMitigationTestCharacter* MoveTarget =
		World.Get()->SpawnActor<APRPlayerSkillMitigationTestCharacter>();
	APRPlayerSkillTestTarget* MovementObstacle = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	if (!TestNotNull(TEXT("Displacement character spawned"), MoveTarget)
		|| !TestNotNull(TEXT("Movement obstacle spawned"), MovementObstacle))
	{
		return false;
	}
	MoveTarget->SetActorLocation(FVector(0.0f, 0.0f, 300.0f));
	MoveTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	MovementObstacle->ConfigureTarget(TEXT("Obstacle"), false);
	MovementObstacle->SetTestCollisionObjectType(ECC_WorldStatic);
	MovementObstacle->SetActorLocation(FVector(1000.0f, 0.0f, 1000.0f));

	TArray<FPRAbilityDisplacementResult> DisplacementResults;
	const FDelegateHandle DisplacementHandle = Subsystem->OnDisplacementFinished().AddLambda(
		[&DisplacementResults](const FPRAbilityDisplacementResult& Finished)
		{
			DisplacementResults.Add(Finished);
		});
	auto MakeDisplacement = [Source, MoveTarget](const FVector Start)
	{
		FPRAbilityDisplacementRequest Request;
		Request.SourceActor = Source;
		Request.TargetActor = MoveTarget;
		Request.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
		Request.StartLocation = Start;
		Request.DesiredEndLocation = Start + FVector(120.0f, 0.0f, 0.0f);
		Request.Duration = 0.12f;
		Request.StopDistance = 20.0f;
		return Request;
	};

	FGuid RequestId;
	Query.AimDirection = FVector::ForwardVector;
	FPRAbilityDisplacementRequest Displacement = MakeDisplacement(MoveTarget->GetActorLocation());
	TestTrue(TEXT("Authority displacement request succeeds"),
		Subsystem->RequestDisplacement(Displacement, RequestId, FailureTag));
	TestTrue(TEXT("Displacement request ID is valid"), RequestId.IsValid());
	TestTrue(TEXT("Displacement is driven by an active RootMotionSource"),
		MoveTarget->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources());
	TestTrue(TEXT("Active displacement cancels"), Subsystem->CancelDisplacement(RequestId));
	TestEqual(TEXT("Cancel broadcasts once"), DisplacementResults.Num(), 1);
	if (DisplacementResults.Num() == 1)
	{
		TestEqual(TEXT("Cancel end reason"), DisplacementResults[0].EndReason,
			EPRAbilityDisplacementEndReason::Cancelled);
	}

	Displacement = MakeDisplacement(MoveTarget->GetActorLocation());
	TestTrue(TEXT("Second displacement starts"),
		Subsystem->RequestDisplacement(Displacement, RequestId, FailureTag));
	MovementObstacle->SetActorLocation(Displacement.StartLocation + FVector(50.0f, 0.0f, 0.0f));
	for (int32 TickIndex = 0; TickIndex < 4 && DisplacementResults.Num() < 2; ++TickIndex)
	{
		World.Tick(1.0f / 60.0f);
	}
	TestEqual(TEXT("Active sweep obstruction broadcasts"), DisplacementResults.Num(), 2);
	if (DisplacementResults.Num() == 2)
	{
		TestEqual(TEXT("Active obstruction end reason"), DisplacementResults[1].EndReason,
			EPRAbilityDisplacementEndReason::Blocked);
	}

	MovementObstacle->SetActorLocation(FVector(1000.0f, 0.0f, 1000.0f));
	Displacement = MakeDisplacement(MoveTarget->GetActorLocation());
	TestTrue(TEXT("Completion displacement starts"),
		Subsystem->RequestDisplacement(Displacement, RequestId, FailureTag));
	TestTrue(TEXT("Completion branch also uses RootMotionSource"),
		MoveTarget->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources());
	// The minimal automation world has no game-mode movement loop. Simulate the
	// RootMotionSource reaching its computed endpoint (120 - 20 stop distance)
	// so this branch verifies the subsystem's Completed classification.
	MoveTarget->SetActorLocation(Displacement.StartLocation + FVector(100.0f, 0.0f, 0.0f));
	for (int32 TickIndex = 0; TickIndex < 12 && DisplacementResults.Num() < 3; ++TickIndex)
	{
		World.Tick(1.0f / 60.0f);
	}
	TestEqual(TEXT("Completed displacement broadcasts"), DisplacementResults.Num(), 3);
	if (DisplacementResults.Num() == 3)
	{
		TestEqual(TEXT("Completed end reason"), DisplacementResults[2].EndReason,
			EPRAbilityDisplacementEndReason::Completed);
	}

	Displacement = MakeDisplacement(MoveTarget->GetActorLocation());
	TestTrue(TEXT("World-cleanup displacement starts"),
		Subsystem->RequestDisplacement(Displacement, RequestId, FailureTag));
	const int32 ResultCountBeforeCleanup = DisplacementResults.Num();
	FWorldDelegates::OnWorldCleanup.Broadcast(World.Get(), true, true);
	TestFalse(TEXT("World cleanup removes request idempotently"), Subsystem->CancelDisplacement(RequestId));
	TestEqual(TEXT("World cleanup does not broadcast into teardown"),
		DisplacementResults.Num(), ResultCountBeforeCleanup);
	Subsystem->OnDisplacementFinished().Remove(DisplacementHandle);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
