// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "CoreGlobals.h"
#include "Misc/AutomationTest.h"

namespace PRPlayerSkillLifecycleAutomation
{
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
	void TickFor(const float Duration, const float Step = 1.0f / 120.0f) const
	{
		float Elapsed = 0.0f;
		while (Elapsed + UE_SMALL_NUMBER < Duration)
		{
			const float Delta = FMath::Min(Step, Duration - Elapsed);
			Tick(Delta);
			Elapsed += Delta;
		}
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

struct FGuardPlayer
{
	APRPlayerState* PlayerState = nullptr;
	APRPlayerController* Controller = nullptr;
	APRPlayerSkillCombatTestCharacter* Character = nullptr;
	UPRAbilitySystemComponent* ASC = nullptr;
};

bool SpawnGuardPlayer(UWorld* World, const FName TargetId, FGuardPlayer& OutPlayer)
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
	// Lifecycle fixtures are deliberately non-targetable. Hook-specific cases
	// opt their single intended target back in below.
	OutPlayer.Character->ConfigureTarget(TargetId, false);
	OutPlayer.Controller->SetPlayerState(OutPlayer.PlayerState);
	OutPlayer.Controller->Possess(OutPlayer.Character);
	OutPlayer.ASC = OutPlayer.PlayerState->GetProjectRAbilitySystemComponent();
	if (!OutPlayer.ASC)
	{
		return false;
	}
	OutPlayer.Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);
	return true;
}
} // namespace PRPlayerSkillLifecycleAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillLifecycleTest,
	"ProjectR.PlayerSkill.Runtime.LifecycleDeathAndAvatarReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPlayerSkillLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace PRPlayerSkillLifecycleAutomation;
	FScopedWorld World;
	APRPlayerSkillMitigationTestCharacter* Character =
		World.Get()->SpawnActor<APRPlayerSkillMitigationTestCharacter>();
	if (!TestNotNull(TEXT("Skill character spawned"), Character))
	{
		return false;
	}
	UPRPlayerSkillComponent* Component = Character->GetPlayerSkillComponent();
	if (!TestNotNull(TEXT("Native skill component exists"), Component))
	{
		return false;
	}
	TestFalse(TEXT("Skill component has no Tick"), Component->PrimaryComponentTick.bCanEverTick);
	const FGameplayTag ShadowThrustTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	TestEqual(TEXT("Component starts idle"), Component->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
	int32 ExpiredCount = 0;
	FGameplayTag ExpiredTag;
	EPRPlayerSkillPhase ExpiredPhase = EPRPlayerSkillPhase::Idle;
	const FDelegateHandle PhaseHandle = Component->OnPhaseExpired().AddLambda(
		[&ExpiredCount, &ExpiredTag, &ExpiredPhase](const FGameplayTag AbilityTag, const EPRPlayerSkillPhase Phase)
		{
			++ExpiredCount;
			ExpiredTag = AbilityTag;
			ExpiredPhase = Phase;
		});
	TestTrue(TEXT("Timed phase begins"), Component->BeginPhase(
		ShadowThrustTag, EPRPlayerSkillPhase::Startup, 0.01f));
	for (int32 TickIndex = 0; TickIndex < 3 && ExpiredCount == 0; ++TickIndex)
	{
		World.Tick(0.02f);
	}
	TestEqual(TEXT("Phase timer expires once"), ExpiredCount, 1);
	TestEqual(TEXT("Expired ability tag"), ExpiredTag, ShadowThrustTag);
	TestEqual(TEXT("Expired phase"), ExpiredPhase, EPRPlayerSkillPhase::Startup);
	Component->CancelExecution();
	TestEqual(TEXT("Cancel records Cancelled"), Component->GetCurrentPhase(), EPRPlayerSkillPhase::Cancelled);
	Component->ClearExecution();
	TestEqual(TEXT("Clear returns component to idle"), Component->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
	TestFalse(TEXT("Clear removes ability tag"), Component->GetCurrentAbilityTag().IsValid());
	TestFalse(TEXT("Clear removes displacement ID"), Component->GetActiveDisplacementRequestId().IsValid());
	Component->OnPhaseExpired().Remove(PhaseHandle);
	FGameplayTagContainer MitigationTags;
	FPRDamageRequest EmptyDamage;
	TestEqual(TEXT("Checkpoint A component does not implement guard logic"),
		Component->EvaluateIncomingDamage(EmptyDamage, MitigationTags),
		EPRCombatMitigationResult::NotHandled);
	TestTrue(TEXT("A mitigation adds no response tags"), MitigationTags.IsEmpty());

	APRPlayerSkillTestTarget* Owner = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	APRPlayerSkillTestTarget* FirstAvatar = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	APRPlayerSkillTestTarget* ReplacementAvatar = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	if (!TestNotNull(TEXT("ASC owner spawned"), Owner)
		|| !TestNotNull(TEXT("First Avatar spawned"), FirstAvatar)
		|| !TestNotNull(TEXT("Replacement Avatar spawned"), ReplacementAvatar))
	{
		return false;
	}
	Owner->ConfigureTarget(TEXT("Lifecycle.Owner"), false);
	FirstAvatar->ConfigureTarget(TEXT("Lifecycle.FirstAvatar"), false);
	ReplacementAvatar->ConfigureTarget(TEXT("Lifecycle.ReplacementAvatar"), false);
	UPRAbilitySystemComponent* ASC = NewObject<UPRAbilitySystemComponent>(Owner, TEXT("PlayerSkillLifecycleASC"));
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Owner, FirstAvatar);
	const FGameplayAbilitySpecHandle AbilityHandle = ASC->GiveAbility(
		FGameplayAbilitySpec(UPRPlayerSkillLifecycleTestAbility::StaticClass(), 1));
	auto IsAbilityActive = [ASC, AbilityHandle]()
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(AbilityHandle);
		return Spec && Spec->IsActive();
	};
	TestTrue(TEXT("Transient player skill activates"), ASC->TryActivateAbility(AbilityHandle));
	TestTrue(TEXT("Transient player skill remains active"), IsAbilityActive());
	TestTrue(TEXT("Active skill owns PlayerSkillActive tag"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetAbilityStatePlayerSkillActiveTag()));
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateStunnedTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestFalse(TEXT("Stunned cancels active player skill"), IsAbilityActive());
	TestFalse(TEXT("Stunned cancellation removes owned tag"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetAbilityStatePlayerSkillActiveTag()));
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateStunnedTag(), 0, EGameplayTagReplicationState::TagOnly);

	TestTrue(TEXT("Player skill reactivates before replacement"), ASC->TryActivateAbility(AbilityHandle));
	ASC->InitAbilityActorInfo(Owner, ReplacementAvatar);
	TestFalse(TEXT("Avatar replacement cancels active player skill"), IsAbilityActive());
	TestEqual(TEXT("Replacement becomes current Avatar"), ASC->GetAvatarActor(), static_cast<AActor*>(ReplacementAvatar));

	TestTrue(TEXT("Player skill reactivates before death"), ASC->TryActivateAbility(AbilityHandle));
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestFalse(TEXT("Dead cancels active player skill"), IsAbilityActive());
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);
	ASC->ClearActorInfo();
	TestFalse(TEXT("ClearActorInfo leaves no active player skill"), IsAbilityActive());
	ASC->DestroyComponent();

	const UPRPlayerSkillDataAsset* GuardData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_CounterProofWall.DA_Skill_CounterProofWall"));
	if (!TestNotNull(TEXT("CounterProofWall data is available for lifecycle cleanup"), GuardData))
	{
		return false;
	}
	FGuardPlayer GuardPlayer;
	if (!TestTrue(TEXT("CounterProofWall lifecycle player spawns"),
		SpawnGuardPlayer(World.Get(), TEXT("Automation.GuardLifecycle"), GuardPlayer)))
	{
		return false;
	}
	GuardPlayer.Character->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	FGameplayAbilitySpecHandle GuardHandle;
	for (const FGameplayAbilitySpec& Spec : GuardPlayer.ASC->GetActivatableAbilities())
	{
		if (Spec.SourceObject.Get() == GuardData)
		{
			GuardHandle = Spec.Handle;
			break;
		}
	}
	if (!TestTrue(TEXT("CounterProofWall lifecycle uses its one formally granted spec"),
		GuardHandle.IsValid()))
	{
		return false;
	}
	FGameplayTagContainer GuardingTags;
	GuardingTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	const FGameplayEffectQuery GuardingQuery =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(GuardingTags);
	auto ActivateGuard = [&GuardPlayer, GuardData]()
	{
		GuardPlayer.ASC->AbilityInputTagPressed(GuardData->InputTag);
	};
	auto HasNoGuardRuntimeState = [&GuardPlayer, &GuardingQuery]()
	{
		return !GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
			&& GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num() == 0
			&& GuardPlayer.Character->GetPlayerSkillComponent()->GetCurrentPhase() == EPRPlayerSkillPhase::Idle;
	};

	ActivateGuard();
	TestTrue(TEXT("CounterProofWall activates before death cleanup"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	GuardPlayer.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestTrue(TEXT("Death clears CounterProofWall effect, tag, and phase"), HasNoGuardRuntimeState());
	GuardPlayer.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);

	World.TickFor(6.01f);
	ActivateGuard();
	TestTrue(TEXT("CounterProofWall activates before Stunned cleanup"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	GuardPlayer.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateStunnedTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestTrue(TEXT("Stunned clears CounterProofWall effect, tag, and phase"), HasNoGuardRuntimeState());
	GuardPlayer.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateStunnedTag(), 0, EGameplayTagReplicationState::TagOnly);

	World.TickFor(6.01f);
	ActivateGuard();
	APRPlayerSkillCombatTestCharacter* Replacement =
		World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestNotNull(TEXT("CounterProofWall replacement avatar spawns"), Replacement))
	{
		return false;
	}
	Replacement->ConfigureTarget(TEXT("Lifecycle.GuardReplacement"), false);
	GuardPlayer.ASC->InitAbilityActorInfo(GuardPlayer.PlayerState, Replacement);
	TestTrue(TEXT("Avatar replacement clears CounterProofWall effect and tag"),
		!GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
			&& GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num() == 0);
	GuardPlayer.ASC->InitAbilityActorInfo(GuardPlayer.PlayerState, GuardPlayer.Character);

	APRPlayerSkillCombatTestCharacter* RemovalAvatar =
		World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!TestNotNull(TEXT("CounterProofWall removal avatar spawns"), RemovalAvatar))
	{
		return false;
	}
	RemovalAvatar->ConfigureTarget(TEXT("Lifecycle.GuardRemoval"), false);
	RemovalAvatar->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	RemovalAvatar->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	APRPlayerSkillTestPlayerState* RemovalState =
		World.Get()->SpawnActor<APRPlayerSkillTestPlayerState>();
	if (!TestNotNull(TEXT("CounterProofWall removal PlayerState spawns"), RemovalState))
	{
		return false;
	}
	UPRAbilitySystemComponent* RemovalASC = RemovalState->GetProjectRAbilitySystemComponent();
	UPRAttributeSet* RemovalAttributes = const_cast<UPRAttributeSet*>(RemovalState->GetAttributeSet());
	if (!TestNotNull(TEXT("CounterProofWall removal PlayerState owns an ASC"), RemovalASC)
		|| !TestNotNull(TEXT("CounterProofWall removal PlayerState owns attributes"), RemovalAttributes))
	{
		return false;
	}
	RemovalASC->InitAbilityActorInfo(RemovalState, RemovalAvatar);
	RemovalAttributes->InitMaxEnergy(100.0f);
	RemovalAttributes->InitEnergy(100.0f);
	UPRAbilitySetDataAsset* RemovalSet = NewObject<UPRAbilitySetDataAsset>(GetTransientPackage());
	FPRAbilitySetEntry& RemovalEntry = RemovalSet->AbilityEntries.AddDefaulted_GetRef();
	RemovalEntry.AbilityClass = GuardData->AbilityClass;
	RemovalEntry.AbilityLevel = 1;
	RemovalEntry.InputTag = GuardData->InputTag;
	RemovalEntry.bGrantOnInitialization = true;
	RemovalEntry.AbilityData = const_cast<UPRPlayerSkillDataAsset*>(GuardData);
	FPRAbilitySetGrantHandle RemovalGrant;
	if (!TestEqual(TEXT("Isolated CounterProofWall AbilitySet grants"),
		RemovalASC->GrantAbilitySet(RemovalSet, EPRAbilitySetGrantMode::AllEntries, RemovalGrant),
		EPRAbilitySetOperationStatus::Applied))
	{
		return false;
	}
	FGameplayTagContainer RemovalGuardingTags;
	RemovalGuardingTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	const FGameplayEffectQuery RemovalGuardingQuery =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(RemovalGuardingTags);
	RemovalASC->AbilityInputTagPressed(GuardData->InputTag);
	TestTrue(TEXT("Isolated CounterProofWall activates before AbilitySet removal"),
		RemovalASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("AbilitySet removal cancels its active CounterProofWall"),
		RemovalASC->RemoveAbilitySet(RemovalGrant), EPRAbilitySetOperationStatus::Removed);
	TestFalse(TEXT("AbilitySet removal clears CounterProofWall Guarding tag"),
		RemovalASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("AbilitySet removal clears CounterProofWall state effect"),
		RemovalASC->GetActiveEffects(RemovalGuardingQuery).Num(), 0);
	TestEqual(TEXT("AbilitySet removal clears CounterProofWall phase timer state"),
		RemovalAvatar->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);

	const UPRPlayerSkillDataAsset* HookData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_VectorHook.DA_Skill_VectorHook"));
	UPRCombatSubsystem* Combat = World.Get()->GetSubsystem<UPRCombatSubsystem>();
	if (!TestNotNull(TEXT("VectorHook data is available for cancellation cleanup"), HookData)
		|| !TestNotNull(TEXT("Combat subsystem is available for VectorHook outcomes"), Combat))
	{
		return false;
	}
	int32 HookCompletionOutcomes = 0;
	const FDelegateHandle HookOutcomeHandle = Combat->OnCombatEvent().AddLambda(
		[&HookCompletionOutcomes, HookData](const FPRCombatEvent& Event)
		{
			if (Event.AbilityTag == HookData->AbilityTag
				&& Event.EventTag == UPRTagLibrary::GetCombatEventAbilityOutcomeTag()
				&& Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()))
			{
				++HookCompletionOutcomes;
			}
		});
	auto FindFormalSpec = [](const UPRAbilitySystemComponent& InASC,
		const UPRPlayerSkillDataAsset& InData)
	{
		for (const FGameplayAbilitySpec& Spec : InASC.GetActivatableAbilities())
		{
			if (Spec.SourceObject.Get() == &InData)
			{
				return Spec.Handle;
			}
		}
		return FGameplayAbilitySpecHandle();
	};
	enum class EHookCancellationCase : uint8
	{
		TargetInvalid,
		TargetDead,
		TargetStunned,
		AvatarReplacement
	};
	auto VerifyHookCancellation = [this, &World, HookData, &FindFormalSpec,
		&HookCompletionOutcomes](const EHookCancellationCase Case, const TCHAR* Label)
	{
		FGuardPlayer HookSource;
		FGuardPlayer HookTarget;
		if (!TestTrue(FString::Printf(TEXT("%s VectorHook source spawns"), Label),
			SpawnGuardPlayer(World.Get(), FName(FString(Label) + TEXT(".Source")), HookSource))
			|| !TestTrue(FString::Printf(TEXT("%s VectorHook target spawns"), Label),
				SpawnGuardPlayer(World.Get(), FName(FString(Label) + TEXT(".Target")), HookTarget)))
		{
			return false;
		}
		// Keep this formal query away from the earlier lifecycle fixtures: their
		// capsules are valid World visibility blockers even when non-targetable.
		const FVector HookOrigin(0.0f, 0.0f, 5000.0f);
		HookSource.Character->SetActorLocation(HookOrigin);
		HookTarget.Character->SetActorLocation(HookOrigin + FVector(400.0f, 0.0f, 0.0f));
		HookSource.Character->ConfigureTarget(FName(FString(Label) + TEXT(".Source")), false);
		HookTarget.Character->ConfigureTarget(FName(FString(Label) + TEXT(".Target")), true);
		HookSource.Character->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		const FGameplayAbilitySpecHandle HookHandle = FindFormalSpec(*HookSource.ASC, *HookData);
		if (!TestTrue(FString::Printf(TEXT("%s has one formal VectorHook spec"), Label), HookHandle.IsValid()))
		{
			return false;
		}
		UPRPlayerSkillComponent* SourceComponent = HookSource.Character->GetPlayerSkillComponent();
		if (!TestNotNull(FString::Printf(TEXT("%s source owns a skill component"), Label), SourceComponent))
		{
			return false;
		}
		const int32 OutcomeStart = HookCompletionOutcomes;
		HookSource.ASC->AbilityInputTagPressed(HookData->InputTag);
		if (!TestEqual(FString::Printf(TEXT("%s VectorHook commits into Active"), Label),
			SourceComponent->GetCurrentPhase(), EPRPlayerSkillPhase::Active)
			|| !TestTrue(FString::Printf(TEXT("%s VectorHook owns one displacement"), Label),
				SourceComponent->GetActiveDisplacementRequestId().IsValid())
			|| !TestTrue(FString::Printf(TEXT("%s VectorHook Light target owns RootMotion"), Label),
				HookTarget.Character->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources()))
		{
			return false;
		}

		switch (Case)
		{
		case EHookCancellationCase::TargetInvalid:
			HookTarget.Character->Destroy();
			break;
		case EHookCancellationCase::TargetDead:
			HookTarget.ASC->SetLooseGameplayTagCount(
				UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
			break;
		case EHookCancellationCase::TargetStunned:
			HookTarget.ASC->SetLooseGameplayTagCount(
				UPRTagLibrary::GetStateStunnedTag(), 1, EGameplayTagReplicationState::TagOnly);
			break;
		case EHookCancellationCase::AvatarReplacement:
		{
			APRPlayerSkillCombatTestCharacter* Replacement =
				World.Get()->SpawnActor<APRPlayerSkillCombatTestCharacter>();
			if (!TestNotNull(FString::Printf(TEXT("%s replacement avatar spawns"), Label), Replacement))
			{
				return false;
			}
			Replacement->ConfigureTarget(FName(FString(Label) + TEXT(".Replacement")), false);
			HookSource.ASC->InitAbilityActorInfo(HookSource.PlayerState, Replacement);
			break;
		}
		}
		World.TickFor(0.05f);
		TestEqual(FString::Printf(TEXT("%s cancellation clears source phase"), Label),
			SourceComponent->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
		TestFalse(FString::Printf(TEXT("%s cancellation clears displacement request"), Label),
			SourceComponent->GetActiveDisplacementRequestId().IsValid());
		TestEqual(FString::Printf(TEXT("%s cancellation publishes no DisplacementApplied"), Label),
			HookCompletionOutcomes, OutcomeStart);
		World.TickFor(0.35f);
		TestEqual(FString::Printf(TEXT("%s cancellation leaves no delayed DisplacementApplied"), Label),
			HookCompletionOutcomes, OutcomeStart);
		if (IsValid(HookSource.Character))
		{
			HookSource.Character->ConfigureTarget(FName(FString(Label) + TEXT(".Source")), false);
		}
		if (IsValid(HookTarget.Character))
		{
			HookTarget.Character->ConfigureTarget(FName(FString(Label) + TEXT(".Target")), false);
		}
		return true;
	};
	const bool bHookInvalidClean = VerifyHookCancellation(
		EHookCancellationCase::TargetInvalid, TEXT("InvalidTarget"));
	const bool bHookDeadClean = VerifyHookCancellation(
		EHookCancellationCase::TargetDead, TEXT("DeadTarget"));
	const bool bHookStunnedClean = VerifyHookCancellation(
		EHookCancellationCase::TargetStunned, TEXT("StunnedTarget"));
	const bool bHookReplacementClean = VerifyHookCancellation(
		EHookCancellationCase::AvatarReplacement, TEXT("AvatarReplacement"));
	Combat->OnCombatEvent().Remove(HookOutcomeHandle);
	if (!(bHookInvalidClean && bHookDeadClean && bHookStunnedClean && bHookReplacementClean))
	{
		return false;
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
