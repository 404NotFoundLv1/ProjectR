// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
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

private:
	TObjectPtr<UWorld> World;
};
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
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
