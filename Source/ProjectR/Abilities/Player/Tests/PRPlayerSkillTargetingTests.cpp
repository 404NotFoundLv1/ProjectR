// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRGA_VectorHook.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Core/PRTagLibrary.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Combat/PRCombatSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

template <typename T>
T* SpawnBlueprintActor(UWorld* World, const TCHAR* ClassPath)
{
	UClass* ActorClass = LoadClass<T>(nullptr, ClassPath);
	return ActorClass ? World->SpawnActor<T>(ActorClass) : nullptr;
}

struct FHookPlayer
{
	APRPlayerState* PlayerState = nullptr;
	APRPlayerController* Controller = nullptr;
	APRPlayerSkillCombatTestCharacter* Character = nullptr;
	UPRAbilitySystemComponent* ASC = nullptr;
	const UPRAttributeSet* Attributes = nullptr;
};

bool SpawnHookPlayer(UWorld* World, const FVector Location, const FName TargetId, FHookPlayer& OutPlayer)
{
	OutPlayer.PlayerState = SpawnBlueprintActor<APRPlayerState>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	OutPlayer.Controller = SpawnBlueprintActor<APRPlayerController>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
	OutPlayer.Character = World->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!OutPlayer.PlayerState || !OutPlayer.Controller || !OutPlayer.Character)
	{
		return false;
	}
	OutPlayer.Character->SetActorLocation(Location);
	OutPlayer.Character->ConfigureTarget(TargetId, true);
	OutPlayer.Controller->SetPlayerState(OutPlayer.PlayerState);
	OutPlayer.Controller->Possess(OutPlayer.Character);
	OutPlayer.ASC = OutPlayer.PlayerState->GetProjectRAbilitySystemComponent();
	OutPlayer.Attributes = OutPlayer.PlayerState->GetAttributeSet();
	if (!OutPlayer.ASC || !OutPlayer.Attributes)
	{
		return false;
	}
	OutPlayer.Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 0.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 0.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);
	return true;
}

FGameplayAbilitySpecHandle GrantFormalHook(UPRAbilitySystemComponent& ASC, const UPRPlayerSkillDataAsset& Data)
{
	for (const FGameplayAbilitySpec& ExistingSpec : ASC.GetActivatableAbilities())
	{
		if (ExistingSpec.SourceObject.Get() == &Data)
		{
			return ExistingSpec.Handle;
		}
	}
	FGameplayAbilitySpec Spec(Data.AbilityClass, 1, INDEX_NONE, const_cast<UPRPlayerSkillDataAsset*>(&Data));
	Spec.GetDynamicSpecSourceTags().AddTag(Data.AbilityTag);
	Spec.GetDynamicSpecSourceTags().AddTag(Data.InputTag);
	return ASC.GiveAbility(Spec);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillTargetingTest,
	"ProjectR.PlayerSkill.Runtime.TargetSelectionAndDisplacement",
	PRPlayerSkillTargetAutomation::TestFlags)

bool FPRPlayerSkillTargetingTest::RunTest(const FString& Parameters)
{
	using namespace PRPlayerSkillTargetAutomation;
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
	Near->SetActorLocation(FVector(120.0f, 0.0f, 0.0f));
	Query.AreaCenter = FVector(240.0f, 0.0f, 0.0f);
	Query.Radius = 150.0f;
	FailureTag = FGameplayTag();
	TestTrue(TEXT("Ground area ignores an eligible Visibility-blocking pawn on its path"),
		Subsystem->QueryTargets(Query, Result, FailureTag));
	TestFalse(TEXT("Eligible ground-area pawn leaves no failure tag"), FailureTag.IsValid());
	TestTrue(TEXT("Ground area still returns the pawn on its path"), Result.Targets.Contains(Near));
	Near->SetActorLocation(FVector(100.0f, 0.0f, 50.0f));
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

	FVector ResolvedShadowEnd;
	float ResolvedShadowDistance = 0.0f;
	MovementObstacle->SetActorLocation(MoveTarget->GetActorLocation());
	TestFalse(TEXT("Shadow path rejects a WorldStatic start penetration"),
		Subsystem->ResolveShadowPath(
			*MoveTarget,
			FVector::ForwardVector,
			450.0f,
			75.0f,
			ResolvedShadowEnd,
			ResolvedShadowDistance,
			FailureTag));
	TestEqual(TEXT("Start penetration reports Obstructed"), FailureTag,
		UPRTagLibrary::GetAbilityActivateFailObstructedTag());
	MovementObstacle->SetActorLocation(MoveTarget->GetActorLocation() + FVector(200.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Shadow path resolves a safe endpoint before WorldStatic"),
		Subsystem->ResolveShadowPath(
			*MoveTarget,
			FVector::ForwardVector,
			450.0f,
			75.0f,
			ResolvedShadowEnd,
			ResolvedShadowDistance,
			FailureTag));
	TestTrue(TEXT("Shadow safe endpoint remains before the obstacle"),
		ResolvedShadowDistance > 0.0f && ResolvedShadowDistance < 200.0f
			&& ResolvedShadowEnd.X < MovementObstacle->GetActorLocation().X);
	TestEqual(TEXT("Shadow safe endpoint preserves the X/Z movement plane"),
		ResolvedShadowEnd.Y, MoveTarget->GetActorLocation().Y);
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
	// The minimal automation world has no game-mode movement loop. Simulate a
	// coarse runtime frame where MoveToForce advances beyond its computed endpoint
	// (120 - 20 stop distance). The subsystem must clamp the legal overshoot back
	// to that endpoint and still classify the displacement as Completed.
	const FVector ExpectedCompletionLocation =
		Displacement.StartLocation + FVector(100.0f, 0.0f, 0.0f);
	MoveTarget->SetActorLocation(Displacement.StartLocation + FVector(180.0f, 0.0f, 0.0f));
	MoveTarget->GetCharacterMovement()->Velocity = FVector(2500.0f, 0.0f, 0.0f);
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
	TestTrue(TEXT("Coarse-frame displacement clamps to its validated endpoint"),
		MoveTarget->GetActorLocation().Equals(ExpectedCompletionLocation, 0.1f));
	TestTrue(TEXT("Completed displacement clears residual forced movement velocity"),
		MoveTarget->GetCharacterMovement()->Velocity.IsNearlyZero());

	Displacement = MakeDisplacement(MoveTarget->GetActorLocation());
	TestTrue(TEXT("World-cleanup displacement starts"),
		Subsystem->RequestDisplacement(Displacement, RequestId, FailureTag));
	const int32 ResultCountBeforeCleanup = DisplacementResults.Num();
	FWorldDelegates::OnWorldCleanup.Broadcast(World.Get(), true, true);
	TestFalse(TEXT("World cleanup removes request idempotently"), Subsystem->CancelDisplacement(RequestId));
	TestEqual(TEXT("World cleanup does not broadcast into teardown"),
		DisplacementResults.Num(), ResultCountBeforeCleanup);
	Subsystem->OnDisplacementFinished().Remove(DisplacementHandle);

	// RED for v0.2.0-D: Hook must discover NoTarget before Commit. The native
	// parent is still a configuration shell at this point, so GAS incorrectly
	// accepts the formal OnInputTriggered activation.
	APRPlayerSkillCombatTestCharacter* HookSource =
		World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestNotNull(TEXT("VectorHook source character spawned"), HookSource))
	{
		return false;
	}
	HookSource->SetActorLocation(FVector(0.0f, 0.0f, -300.0f));
	HookSource->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	UPRAbilitySystemComponent* HookASC = NewObject<UPRAbilitySystemComponent>(HookSource);
	HookASC->RegisterComponent();
	HookASC->InitAbilityActorInfo(HookSource, HookSource);
	UPRPlayerSkillDataAsset* HookData = NewObject<UPRPlayerSkillDataAsset>(HookASC);
	HookData->AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"));
	HookData->InputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Skill.VectorHook"));
	HookData->AbilityClass = UPRPlayerSkillVectorHookTestAbility::StaticClass();
	HookData->ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
	HookData->TargetPolicy = EPRPlayerSkillTargetPolicy::SingleTarget;
	HookData->Range = 800.0f;
	HookData->HalfAngleDegrees = 45.0f;
	HookData->ActiveDuration = 0.30f;
	UPRVectorHookSkillFragment* HookFragment = NewObject<UPRVectorHookSkillFragment>(HookData);
	HookFragment->StopDistance = 120.0f;
	HookData->SkillFragment = HookFragment;
	UPRPlayerSkillComponent* HookComponent = HookSource->GetPlayerSkillComponent();
	if (!TestNotNull(TEXT("VectorHook source owns a skill component"), HookComponent))
	{
		HookASC->DestroyComponent();
		return false;
	}
	FGameplayAbilitySpec HookSpec(UPRPlayerSkillVectorHookTestAbility::StaticClass(), 1, INDEX_NONE, HookData);
	HookSpec.GetDynamicSpecSourceTags().AddTag(HookData->AbilityTag);
	HookSpec.GetDynamicSpecSourceTags().AddTag(HookData->InputTag);
	const FGameplayAbilitySpecHandle HookHandle = HookASC->GiveAbility(HookSpec);
	TestTrue(TEXT("VectorHook test spec is granted"), HookHandle.IsValid());
	TestFalse(TEXT("VectorHook NoTarget fails before Commit"), HookASC->TryActivateAbility(HookHandle));
	TestEqual(TEXT("VectorHook NoTarget leaves the component idle"),
		HookComponent->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
	TestFalse(TEXT("VectorHook NoTarget leaves no owned displacement"),
		HookComponent->GetActiveDisplacementRequestId().IsValid());
	HookASC->DestroyComponent();

	// v0.2.0-D: exercise the real VectorHook data/Cost/Cooldown and its native
	// parent. Each fixture uses a distinct Z lane so deterministic target queries
	// cannot accidentally select a prior scenario's actor.
	const UPRPlayerSkillDataAsset* FormalHookData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_VectorHook.DA_Skill_VectorHook"));
	UPRCombatSubsystem* Combat = World.Get()->GetSubsystem<UPRCombatSubsystem>();
	if (!TestNotNull(TEXT("Formal VectorHook data exists"), FormalHookData)
		|| !TestNotNull(TEXT("Combat subsystem exists for VectorHook outcomes"), Combat))
	{
		return false;
	}
	TestEqual(TEXT("VectorHook damage remains zero by data contract"), FormalHookData->BaseDamage, 0.0f);
	TestEqual(TEXT("VectorHook AttackPower scale remains zero by data contract"), FormalHookData->AttackPowerScale, 0.0f);
	TArray<FPRCombatEvent> HookEvents;
	const FDelegateHandle HookEventHandle = Combat->OnCombatEvent().AddLambda(
		[&HookEvents](const FPRCombatEvent& Event)
		{
			if (Event.AbilityTag == FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook")))
			{
				HookEvents.Add(Event);
			}
		});
	auto ConfigureHookFacing = [](APRPlayerSkillCombatTestCharacter& Character)
	{
		Character.GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	};
	auto HookCooldownActive = [](const UPRAbilitySystemComponent& ASC)
	{
		return ASC.HasMatchingGameplayTag(
			FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.VectorHook")));
	};

	FHookPlayer NoTargetPlayer;
	if (!TestTrue(TEXT("VectorHook NoTarget formal player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -1000.0f), TEXT("Hook.NoTarget.Source"), NoTargetPlayer)))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*NoTargetPlayer.Character);
	const FGameplayAbilitySpecHandle NoTargetHandle = GrantFormalHook(*NoTargetPlayer.ASC, *FormalHookData);
	const float NoTargetEnergy = NoTargetPlayer.Attributes->GetEnergy();
	TestFalse(TEXT("VectorHook formal NoTarget fails before Commit"),
		NoTargetPlayer.ASC->TryActivateAbility(NoTargetHandle));
	TestEqual(TEXT("VectorHook NoTarget does not consume Energy"),
		NoTargetPlayer.Attributes->GetEnergy(), NoTargetEnergy);
	TestFalse(TEXT("VectorHook NoTarget does not create Cooldown"), HookCooldownActive(*NoTargetPlayer.ASC));

	FHookPlayer ObstructedPlayer;
	APRPlayerSkillCombatTestCharacter* ObstructedTarget =
		World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	APRPlayerSkillTestTarget* PreCommitBlocker = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	if (!TestTrue(TEXT("VectorHook obstructed formal player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -2000.0f), TEXT("Hook.Obstructed.Source"), ObstructedPlayer))
		|| !TestNotNull(TEXT("VectorHook obstructed target spawns"), ObstructedTarget)
		|| !TestNotNull(TEXT("VectorHook pre-Commit blocker spawns"), PreCommitBlocker))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*ObstructedPlayer.Character);
	ObstructedTarget->SetActorLocation(FVector(400.0f, 0.0f, -2000.0f));
	ObstructedTarget->ConfigureTarget(TEXT("Hook.Obstructed.Target"), true, EPRAbilityTargetMobility::Heavy);
	ObstructedTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	PreCommitBlocker->ConfigureTarget(TEXT("Hook.Obstructed.Blocker"), false);
	PreCommitBlocker->SetTestCollisionObjectType(ECC_WorldStatic);
	PreCommitBlocker->SetActorLocation(FVector(120.0f, 0.0f, -2000.0f));
	const FGameplayAbilitySpecHandle ObstructedHandle = GrantFormalHook(*ObstructedPlayer.ASC, *FormalHookData);
	const float ObstructedEnergy = ObstructedPlayer.Attributes->GetEnergy();
	TestFalse(TEXT("VectorHook LOS/path obstruction fails before Commit"),
		ObstructedPlayer.ASC->TryActivateAbility(ObstructedHandle));
	TestEqual(TEXT("VectorHook obstruction does not consume Energy"),
		ObstructedPlayer.Attributes->GetEnergy(), ObstructedEnergy);
	TestFalse(TEXT("VectorHook obstruction does not create Cooldown"), HookCooldownActive(*ObstructedPlayer.ASC));
	PreCommitBlocker->SetActorLocation(FVector(10000.0f, 0.0f, 10000.0f));

	auto CompleteHook = [&World](ACharacter& MovedCharacter, const FVector& EffectiveEnd)
	{
		// The isolated automation world has no full movement simulation. This is
		// the same coarse-frame harness used by the frozen displacement tests: the
		// subsystem owns completion/classification after the swept request starts.
		MovedCharacter.SetActorLocation(EffectiveEnd);
		MovedCharacter.GetCharacterMovement()->Velocity = FVector::ZeroVector;
		for (int32 TickIndex = 0; TickIndex < 8; ++TickIndex)
		{
			World.Tick(1.0f / 60.0f);
		}
	};

	FHookPlayer HeavyPlayer;
	APRPlayerSkillCombatTestCharacter* HeavyTarget = World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestTrue(TEXT("VectorHook Heavy player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -3000.0f), TEXT("Hook.Heavy.Source"), HeavyPlayer))
		|| !TestNotNull(TEXT("VectorHook Heavy target spawns"), HeavyTarget))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*HeavyPlayer.Character);
	HeavyTarget->SetActorLocation(FVector(400.0f, 0.0f, -3000.0f));
	HeavyTarget->ConfigureTarget(TEXT("Hook.Heavy.Target"), true, EPRAbilityTargetMobility::Heavy);
	HeavyTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const FVector HeavyStart = HeavyPlayer.Character->GetActorLocation();
	const FVector HeavyTargetStart = HeavyTarget->GetActorLocation();
	const int32 HeavyEventStart = HookEvents.Num();
	TestTrue(TEXT("VectorHook Heavy commits after target/path validation"),
		HeavyPlayer.ASC->TryActivateAbility(GrantFormalHook(*HeavyPlayer.ASC, *FormalHookData)));
	CompleteHook(*HeavyPlayer.Character, FVector(280.0f, 0.0f, -3000.0f));
	TestTrue(TEXT("VectorHook Heavy moves the player to its 120cm stop distance"),
		HeavyPlayer.Character->GetActorLocation().Equals(FVector(280.0f, 0.0f, -3000.0f), 0.1f));
	TestEqual(TEXT("VectorHook Heavy leaves the target stationary"), HeavyTarget->GetActorLocation(), HeavyTargetStart);
	TestEqual(TEXT("VectorHook Heavy preserves the source X/Z plane"),
		HeavyPlayer.Character->GetActorLocation().Y, HeavyStart.Y);
	TestEqual(TEXT("VectorHook Heavy publishes exactly one completed outcome"), HookEvents.Num(), HeavyEventStart + 1);
	if (HookEvents.Num() == HeavyEventStart + 1)
	{
		TestEqual(TEXT("VectorHook completion is a zero-damage AbilityOutcome"),
			HookEvents.Last().EventTag, UPRTagLibrary::GetCombatEventAbilityOutcomeTag());
		TestEqual(TEXT("VectorHook completion RawDamage is zero"), HookEvents.Last().RawDamage, 0.0f);
		TestTrue(TEXT("VectorHook completion has DisplacementApplied"), HookEvents.Last().ResponseTags.HasTagExact(
			UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()));
	}

	FHookPlayer LightPlayer;
	APRPlayerSkillCombatTestCharacter* LightTarget = World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestTrue(TEXT("VectorHook Light player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -4000.0f), TEXT("Hook.Light.Source"), LightPlayer))
		|| !TestNotNull(TEXT("VectorHook Light target spawns"), LightTarget))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*LightPlayer.Character);
	LightTarget->SetActorLocation(FVector(400.0f, 0.0f, -4000.0f));
	LightTarget->ConfigureTarget(TEXT("Hook.Light.Target"), true, EPRAbilityTargetMobility::Light);
	LightTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const FVector LightSourceStart = LightPlayer.Character->GetActorLocation();
	const FVector LightTargetStart = LightTarget->GetActorLocation();
	TestTrue(TEXT("VectorHook Light commits against a character-backed Light target"),
		LightPlayer.ASC->TryActivateAbility(GrantFormalHook(*LightPlayer.ASC, *FormalHookData)));
	CompleteHook(*LightTarget, FVector(120.0f, 0.0f, -4000.0f));
	TestEqual(TEXT("VectorHook Light leaves the player stationary"),
		LightPlayer.Character->GetActorLocation(), LightSourceStart);
	TestTrue(TEXT("VectorHook Light moves target to its 120cm stop distance"),
		LightTarget->GetActorLocation().Equals(FVector(120.0f, 0.0f, -4000.0f), 0.1f));
	TestEqual(TEXT("VectorHook Light preserves the target X/Z plane"),
		LightTarget->GetActorLocation().Y, LightTargetStart.Y);

	FHookPlayer AnchoredPlayer;
	APRPlayerSkillCombatTestCharacter* AnchoredTarget = World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestTrue(TEXT("VectorHook Anchored player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -5000.0f), TEXT("Hook.Anchored.Source"), AnchoredPlayer))
		|| !TestNotNull(TEXT("VectorHook Anchored target spawns"), AnchoredTarget))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*AnchoredPlayer.Character);
	AnchoredTarget->SetActorLocation(FVector(400.0f, 0.0f, -5000.0f));
	AnchoredTarget->ConfigureTarget(TEXT("Hook.Anchored.Target"), true, EPRAbilityTargetMobility::Anchored);
	AnchoredTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const FVector AnchoredTargetStart = AnchoredTarget->GetActorLocation();
	TestTrue(TEXT("VectorHook Anchored commits after validation"),
		AnchoredPlayer.ASC->TryActivateAbility(GrantFormalHook(*AnchoredPlayer.ASC, *FormalHookData)));
	CompleteHook(*AnchoredPlayer.Character, FVector(280.0f, 0.0f, -5000.0f));
	TestTrue(TEXT("VectorHook Anchored pulls the player to 120cm"),
		AnchoredPlayer.Character->GetActorLocation().Equals(FVector(280.0f, 0.0f, -5000.0f), 0.1f));
	TestEqual(TEXT("VectorHook Anchored never moves the target"), AnchoredTarget->GetActorLocation(), AnchoredTargetStart);

	FHookPlayer RuntimeBlockedPlayer;
	APRPlayerSkillCombatTestCharacter* RuntimeBlockedTarget = World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	APRPlayerSkillTestTarget* RuntimeBlocker = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	if (!TestTrue(TEXT("VectorHook runtime-blocked player spawns"), SpawnHookPlayer(
		World.Get(), FVector(0.0f, 0.0f, -6000.0f), TEXT("Hook.RuntimeBlocked.Source"), RuntimeBlockedPlayer))
		|| !TestNotNull(TEXT("VectorHook runtime-blocked target spawns"), RuntimeBlockedTarget)
		|| !TestNotNull(TEXT("VectorHook runtime blocker spawns"), RuntimeBlocker))
	{
		Combat->OnCombatEvent().Remove(HookEventHandle);
		return false;
	}
	ConfigureHookFacing(*RuntimeBlockedPlayer.Character);
	RuntimeBlockedTarget->SetActorLocation(FVector(400.0f, 0.0f, -6000.0f));
	RuntimeBlockedTarget->ConfigureTarget(TEXT("Hook.RuntimeBlocked.Target"), true, EPRAbilityTargetMobility::Heavy);
	RuntimeBlockedTarget->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const int32 RuntimeBlockedEventStart = HookEvents.Num();
	TestTrue(TEXT("VectorHook starts before a runtime WorldStatic obstruction"),
		RuntimeBlockedPlayer.ASC->TryActivateAbility(GrantFormalHook(*RuntimeBlockedPlayer.ASC, *FormalHookData)));
	RuntimeBlocker->ConfigureTarget(TEXT("Hook.Runtime.Blocker"), false);
	RuntimeBlocker->SetTestCollisionObjectType(ECC_WorldStatic);
	RuntimeBlocker->SetActorLocation(FVector(120.0f, 0.0f, -6000.0f));
	for (int32 TickIndex = 0; TickIndex < 8; ++TickIndex)
	{
		World.Tick(1.0f / 60.0f);
	}
	TestEqual(TEXT("VectorHook runtime WorldStatic block publishes no completion outcome"),
		HookEvents.Num(), RuntimeBlockedEventStart);
	TestEqual(TEXT("VectorHook runtime WorldStatic block clears active phase"),
		RuntimeBlockedPlayer.Character->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);

	Combat->OnCombatEvent().Remove(HookEventHandle);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
