// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRPlayerSkillAutomationToolset.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/PRGameplayAbility.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Abilities/Player/PRSkillDecoyActor.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputDeviceLibrary.h"
#include "GameplayEffect.h"
#include "InputCoreTypes.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Sound/SoundBase.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRPlayerSkillAutomationToolset
{
constexpr TCHAR ShadowDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ShadowThrust.DA_Skill_ShadowThrust");
constexpr TCHAR FireDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_FireSlash.DA_Skill_FireSlash");
constexpr TCHAR ThunderDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ThunderDrop.DA_Skill_ThunderDrop");
constexpr TCHAR AfterimageDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_AfterimageDodge.DA_Skill_AfterimageDodge");
constexpr TCHAR VectorHookDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_VectorHook.DA_Skill_VectorHook");
constexpr TCHAR CounterProofWallDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_CounterProofWall.DA_Skill_CounterProofWall");

enum class ESmokePhase : uint8
{
	ShadowKeyboardStart,
	ShadowKeyboardWait,
	FireKeyboardStart,
	FireKeyboardWait,
	ShadowCooldownWait,
	ShadowGamepadWait,
	FireCooldownWait,
	FireGamepadCommitWait,
	FireGamepadBurningWait,
	Complete
};

struct FTransientCombatant
{
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerSkillCombatTestCharacter> Character;
	TWeakObjectPtr<APRPlayerSkillCombatTestCharacter> PreviousAvatar;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	const UPRAttributeSet* Attributes = nullptr;
};

template <typename TObjectType>
TObjectType* SpawnBlueprintActor(UWorld& World, const TCHAR* ClassPath)
{
	UClass* ActorClass = LoadClass<TObjectType>(nullptr, ClassPath);
	return ActorClass ? World.SpawnActor<TObjectType>(ActorClass) : nullptr;
}

void ConfigureCharacterCollision(APRPlayerSkillCombatTestCharacter& Character)
{
	if (UCapsuleComponent* Capsule = Character.GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionObjectType(ECC_Pawn);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

bool SpawnCombatant(
	UWorld& World,
	const FVector Location,
	const FName TargetId,
	FTransientCombatant& OutCombatant)
{
	APRPlayerState* PlayerState = SpawnBlueprintActor<APRPlayerState>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	APRPlayerController* Controller = SpawnBlueprintActor<APRPlayerController>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APRPlayerSkillCombatTestCharacter* Character = World.SpawnActor<APRPlayerSkillCombatTestCharacter>(
		APRPlayerSkillCombatTestCharacter::StaticClass(),
		Location,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!PlayerState || !Controller || !Character)
	{
		return false;
	}

	Character->ConfigureTarget(TargetId, true);
	ConfigureCharacterCollision(*Character);
	Controller->SetPlayerState(PlayerState);
	Controller->Possess(Character);
	UPRAbilitySystemComponent* ASC = PlayerState->GetProjectRAbilitySystemComponent();
	const UPRAttributeSet* Attributes = PlayerState->GetAttributeSet();
	if (!ASC || !Attributes || ASC->GetAvatarActor() != Character)
	{
		return false;
	}

	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Character->GetCharacterMovement()->StopMovementImmediately();
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 0.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 0.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetAttackPowerAttribute(), 10.0f);

	OutCombatant.PlayerState = PlayerState;
	OutCombatant.Controller = Controller;
	OutCombatant.Character = Character;
	OutCombatant.ASC = ASC;
	OutCombatant.Attributes = Attributes;
	return true;
}

class FPIEPlayerSkillCheckpointBRunner
	: public TSharedFromThis<FPIEPlayerSkillCheckpointBRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIEPlayerSkillCheckpointBRunner> Runner =
			MakeShared<FPIEPlayerSkillCheckpointBRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(
			NewObject<UToolCallAsyncResultString>());
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		Runner->PhaseStartTime = Runner->GetPlayWorldTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIEPlayerSkillCheckpointBRunner()
	{
		Cleanup();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Fail(TEXT("RunPIEPlayerSkillCheckpointBSmoke requires an active authoritative in-process PIE world."));
			return false;
		}

		World = PlayWorld;
		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		ASC = PlayerState.IsValid() ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		Attributes = PlayerState.IsValid() ? PlayerState->GetAttributeSet() : nullptr;
		SkillComponent = Character.IsValid() ? Character->GetPlayerSkillComponent() : nullptr;
		CombatSubsystem = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		Viewport = PlayWorld->GetGameViewport();
		InputViewport = Viewport.IsValid() ? Viewport->GetGameViewport() : nullptr;
		InputDevice = UInputDeviceLibrary::GetDefaultInputDevice();
		ShadowData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, ShadowDataPath);
		FireData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, FireDataPath);
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid()
			|| !ASC.IsValid() || !Attributes || !SkillComponent.IsValid()
			|| !CombatSubsystem.IsValid() || !Viewport.IsValid() || !InputViewport
			|| !InputDevice.IsValid() || !ShadowData.IsValid() || !FireData.IsValid())
		{
			Fail(TEXT("PIE is missing the formal player, two skill DataAssets, viewport, ASC, or combat objects."));
			return false;
		}
		if (!CheckFormalSpecContract() || !CheckFrozenDataContract())
		{
			Fail(TEXT("The formal v0.2.0-B AbilitySet or skill data contract is not exact."));
			return false;
		}

		InitialTransform = Character->GetActorTransform();
		InitialMovementMode = Character->GetCharacterMovement()->MovementMode;
		InitialHealth = Attributes->GetHealth();
		InitialShield = Attributes->GetShield();
		InitialEnergy = Attributes->GetEnergy();
		InitialAttackPower = Attributes->GetAttackPower();

		AimDirection = Character->GetActorForwardVector();
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			AimDirection = Mesh->GetRightVector();
		}
		AimDirection.Y = 0.0f;
		AimDirection = AimDirection.GetSafeNormal();
		if (!AimDirection.IsNormalized() || FMath::Abs(AimDirection.X) < 0.99f
			|| FMath::Abs(AimDirection.Z) > 0.01f)
		{
			Fail(TEXT("The formal player mesh does not expose a finite horizontal X/Z skill aim."));
			return false;
		}

		TestOrigin = InitialTransform.GetLocation() + FVector(0.0f, 0.0f, 1000.0f);
		InitialTimeDilation = PlayWorld->GetWorldSettings()->TimeDilation;
		SetAutomationTimeDilation(0.09f);
		Character->SetActorLocation(TestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*ASC, 100.0f, 0.0f, 100.0f, 10.0f);

		if (!SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 225.0f,
			TEXT("PIE.Skill.TargetA"), TargetA)
			|| !SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 2000.0f,
				TEXT("PIE.Skill.TargetB"), TargetB))
		{
			Fail(TEXT("The fixed PIE smoke could not spawn its transient combat targets."));
			return false;
		}

		CombatEventHandle = CombatSubsystem->OnCombatEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEPlayerSkillCheckpointBRunner>(AsShared())](
				const FPRCombatEvent& Event)
			{
				if (TSharedPtr<FPIEPlayerSkillCheckpointBRunner> This = WeakThis.Pin())
				{
					This->CombatEvents.Add(Event);
				}
			});
		LifecycleHandle = ASC->OnAbilityLifecycleEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEPlayerSkillCheckpointBRunner>(AsShared())](
				const FPRAbilityLifecycleEvent& Event)
			{
				if (TSharedPtr<FPIEPlayerSkillCheckpointBRunner> This = WeakThis.Pin())
				{
					This->LifecycleEvents.Add(Event);
				}
			});
		Phase = ESmokePhase::ShadowKeyboardStart;
		return true;
	}

	bool Tick(float DeltaSeconds)
	{
		if (!World.IsValid() || !Character.IsValid() || !ASC.IsValid()
			|| !CombatSubsystem.IsValid() || !GEditor || GEditor->PlayWorld != World.Get())
		{
			Fail(TEXT("PIE ended or ProjectR player-skill objects became invalid during smoke."));
			return false;
		}

		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ESmokePhase::ShadowKeyboardStart:
			PrepareTarget(TargetA, Character->GetActorLocation() + AimDirection * 225.0f);
			MoveFar(TargetB);
			ShadowKeyboardEventStart = CombatEvents.Num();
			ShadowKeyboardStartLocation = Character->GetActorLocation();
			SendPressAndRelease(EKeys::U);
			Advance(ESmokePhase::ShadowKeyboardWait);
			break;

		case ESmokePhase::ShadowKeyboardWait:
			if (IsAbilityActive(ShadowData->AbilityTag)
				|| SkillComponent->GetActiveDisplacementRequestId().IsValid())
			{
				if (Elapsed < 3.0)
				{
					break;
				}
			}
			{
				const float Energy = Attributes->GetEnergy();
				const float Health = TargetA.Attributes->GetHealth();
				const int32 DamageCount = CountCombatEvents(
					ShadowKeyboardEventStart, ShadowData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetA.Character.Get());
				const int32 OutcomeCount = CountCombatEvents(
					ShadowKeyboardEventStart, ShadowData->AbilityTag,
					UPRTagLibrary::GetCombatEventAbilityOutcomeTag(), PlayerState.Get());
				const FVector Delta = Character->GetActorLocation() - ShadowKeyboardStartLocation;
				const float AlongAim = FVector::DotProduct(Delta, AimDirection);
				const float YDrift = FMath::Abs(Delta.Y);
				const bool bActive = IsAbilityActive(ShadowData->AbilityTag);
				const int32 CooldownCount = ASC->GetGameplayTagCount(ShadowCooldownTag());
				const EPRPlayerSkillPhase CurrentPhase = SkillComponent->GetCurrentPhase();
				const bool bRequestActive = SkillComponent->GetActiveDisplacementRequestId().IsValid();
				if (!FMath::IsNearlyEqual(Energy, 80.0f, 0.1f)
					|| !FMath::IsNearlyEqual(Health, 60.0f, 0.1f)
					|| DamageCount != 1 || OutcomeCount != 1
					|| AlongAim < 400.0f || AlongAim > 500.0f || YDrift > 1.0f
					|| bActive || CooldownCount != 1)
				{
					Fail(FString::Printf(
						TEXT("Keyboard ShadowThrust mismatch: Energy=%.2f Health=%.2f Damage=%d Outcome=%d AlongAim=%.2f YDrift=%.2f Active=%s Cooldown=%d Phase=%d Request=%s."),
						Energy, Health, DamageCount, OutcomeCount, AlongAim, YDrift,
						bActive ? TEXT("true") : TEXT("false"), CooldownCount,
						static_cast<int32>(CurrentPhase), bRequestActive ? TEXT("true") : TEXT("false")));
					return false;
				}
			}
			bKeyboardShadowPassed = true;
			Advance(ESmokePhase::FireKeyboardStart);
			break;

		case ESmokePhase::FireKeyboardStart:
			SetAutomationTimeDilation(1.0f);
			PrepareTarget(TargetA, Character->GetActorLocation() + AimDirection * 200.0f);
			MoveFar(TargetB);
			FireKeyboardEventStart = CombatEvents.Num();
			SendPressAndRelease(EKeys::I);
			Advance(ESmokePhase::FireKeyboardWait);
			break;

		case ESmokePhase::FireKeyboardWait:
			if (IsAbilityActive(FireData->AbilityTag) || HasBurning(TargetA))
			{
				if (Elapsed < 4.0)
				{
					break;
				}
			}
			if (!CheckEnergy(55.0f)
				|| !FMath::IsNearlyEqual(TargetA.Attributes->GetHealth(), 60.0f, 0.1f)
				|| CountCombatEvents(FireKeyboardEventStart, FireData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetA.Character.Get()) != 4
				|| HasBurning(TargetA) || IsAbilityActive(FireData->AbilityTag))
			{
				Fail(TEXT("Keyboard FireSlash did not complete one initial hit, three Burning ticks, cost, or cleanup."));
				return false;
			}
			bKeyboardFirePassed = true;
			Advance(ESmokePhase::ShadowCooldownWait);
			break;

		case ESmokePhase::ShadowCooldownWait:
			if (ASC->GetGameplayTagCount(ShadowCooldownTag()) != 0)
			{
				if (Elapsed > 5.0)
				{
					Fail(TEXT("ShadowThrust cooldown did not expire before the gamepad blocking scenario."));
					return false;
				}
				break;
			}
			if (!StartBlockedShadowScenario())
			{
				return false;
			}
			Advance(ESmokePhase::ShadowGamepadWait);
			break;

		case ESmokePhase::ShadowGamepadWait:
			if (IsAbilityActive(ShadowData->AbilityTag)
				|| SkillComponent->GetActiveDisplacementRequestId().IsValid())
			{
				if (Elapsed < 3.0)
				{
					break;
				}
			}
			{
				const float Energy = Attributes->GetEnergy();
				const float NearHealth = TargetA.Attributes->GetHealth();
				const float FarHealth = TargetB.Attributes->GetHealth();
				const int32 NearDamageCount = CountCombatEvents(
					ShadowGamepadEventStart, ShadowData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetA.Character.Get());
				const int32 FarDamageCount = CountCombatEvents(
					ShadowGamepadEventStart, ShadowData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetB.Character.Get());
				const int32 OutcomeCount = CountCombatEvents(
					ShadowGamepadEventStart, ShadowData->AbilityTag,
					UPRTagLibrary::GetCombatEventAbilityOutcomeTag(), PlayerState.Get());
				const FVector Delta = Character->GetActorLocation() - ShadowGamepadStartLocation;
				const float AlongAim = FVector::DotProduct(Delta, AimDirection);
				const float YDrift = FMath::Abs(Delta.Y);
				const bool bActive = IsAbilityActive(ShadowData->AbilityTag);
				if (!FMath::IsNearlyEqual(Energy, 35.0f, 0.1f)
					|| !FMath::IsNearlyEqual(NearHealth, 60.0f, 0.1f)
					|| !FMath::IsNearlyEqual(FarHealth, 100.0f, 0.1f)
					|| NearDamageCount != 1 || FarDamageCount != 0 || OutcomeCount != 0
					|| AlongAim < 1.0f || AlongAim > 249.0f || YDrift > 1.0f || bActive)
				{
					Fail(FString::Printf(
						TEXT("Gamepad ShadowThrust mismatch: Energy=%.2f NearHealth=%.2f FarHealth=%.2f NearDamage=%d FarDamage=%d Outcome=%d AlongAim=%.2f YDrift=%.2f Active=%s."),
						Energy, NearHealth, FarHealth, NearDamageCount, FarDamageCount,
						OutcomeCount, AlongAim, YDrift, bActive ? TEXT("true") : TEXT("false")));
					return false;
				}
			}
			bGamepadShadowPassed = true;
			DestroyWall();
			SetAutomationTimeDilation(1.0f);
			Advance(ESmokePhase::FireCooldownWait);
			break;

		case ESmokePhase::FireCooldownWait:
			if (ASC->GetGameplayTagCount(FireCooldownTag()) != 0)
			{
				if (Elapsed > 6.0)
				{
					Fail(TEXT("FireSlash cooldown did not expire before the gamepad cancellation scenario."));
					return false;
				}
				break;
			}
			PrepareTarget(TargetA, Character->GetActorLocation() + AimDirection * 200.0f);
			MoveFar(TargetB);
			FireGamepadEventStart = CombatEvents.Num();
			SetAutomationTimeDilation(0.09f);
			SendPressAndRelease(EKeys::Gamepad_RightTrigger);
			Advance(ESmokePhase::FireGamepadCommitWait);
			break;

		case ESmokePhase::FireGamepadCommitWait:
			if (CheckEnergy(10.0f)
				&& CountCombatEvents(FireGamepadEventStart, FireData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetA.Character.Get()) >= 1)
			{
				FPRAbilityRuntimeState State;
				if (!ASC->GetAbilityRuntimeStateByAbilityTag(FireData->AbilityTag, State)
					|| !State.bActive)
				{
					Fail(TEXT("FireSlash was no longer active when its post-Commit cancellation was requested."));
					return false;
				}
				ASC->CancelAbilityHandle(State.SpecHandle);
				bFireCancelledAfterCommit = !IsAbilityActive(FireData->AbilityTag)
					&& CheckEnergy(10.0f)
					&& ASC->GetGameplayTagCount(FireCooldownTag()) == 1;
				Advance(ESmokePhase::FireGamepadBurningWait);
			}
			else if (Elapsed > 1.0)
			{
				Fail(TEXT("Gamepad FireSlash did not Commit and apply its initial hit before timeout."));
				return false;
			}
			break;

		case ESmokePhase::FireGamepadBurningWait:
			if (HasBurning(TargetA))
			{
				if (Elapsed < 4.0)
				{
					break;
				}
			}
			if (!bFireCancelledAfterCommit || !CheckEnergy(10.0f)
				|| !FMath::IsNearlyEqual(TargetA.Attributes->GetHealth(), 60.0f, 0.1f)
				|| CountCombatEvents(FireGamepadEventStart, FireData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag(), TargetA.Character.Get()) != 4
				|| HasBurning(TargetA))
			{
				Fail(TEXT("Cancelled post-Commit FireSlash did not preserve exactly three Burning ticks and its cost/cooldown."));
				return false;
			}
			bGamepadFirePassed = true;
			SetAutomationTimeDilation(1.0f);
			if (!RunStartupCancellationChecks())
			{
				return false;
			}
			Advance(ESmokePhase::Complete);
			break;

		case ESmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	bool CheckFormalSpecContract() const
	{
		if (ASC->GetActivatableAbilities().Num() < 2)
		{
			return false;
		}
		FPRAbilityRuntimeState ShadowState;
		FPRAbilityRuntimeState FireState;
		return ASC->GetAbilityRuntimeStateByAbilityTag(ShadowData->AbilityTag, ShadowState)
			&& ASC->GetAbilityRuntimeStateByAbilityTag(FireData->AbilityTag, FireState)
			&& ShadowState.InputTag == ShadowData->InputTag
			&& FireState.InputTag == FireData->InputTag;
	}

	bool CheckFrozenDataContract() const
	{
		return FMath::IsNearlyEqual(ShadowData->StartupDuration, 0.08f)
			&& FMath::IsNearlyEqual(ShadowData->ActiveDuration, 0.18f)
			&& FMath::IsNearlyEqual(ShadowData->RecoveryDuration, 0.12f)
			&& FMath::IsNearlyEqual(ShadowData->Range, 450.0f)
			&& FMath::IsNearlyEqual(ShadowData->Radius, 75.0f)
			&& FMath::IsNearlyEqual(FireData->StartupDuration, 0.15f)
			&& FMath::IsNearlyEqual(FireData->RecoveryDuration, 0.20f)
			&& FMath::IsNearlyEqual(FireData->Range, 360.0f)
			&& FMath::IsNearlyEqual(FireData->HalfAngleDegrees, 60.0f);
	}

	bool StartBlockedShadowScenario()
	{
		SetAutomationTimeDilation(0.09f);
		ShadowGamepadStartLocation = Character->GetActorLocation();
		PrepareTarget(TargetA, ShadowGamepadStartLocation + AimDirection * 100.0f);
		PrepareTarget(TargetB, ShadowGamepadStartLocation + AimDirection * 350.0f);
		Wall = World->SpawnActor<AActor>();
		if (!Wall.IsValid())
		{
			Fail(TEXT("The fixed PIE smoke could not spawn its transient WorldStatic wall."));
			return false;
		}
		UBoxComponent* Box = NewObject<UBoxComponent>(Wall.Get(), TEXT("CheckpointBWall"));
		if (!Box)
		{
			Fail(TEXT("The fixed PIE smoke could not create its transient wall collision."));
			return false;
		}
		Wall->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10.0f, 200.0f, 200.0f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Wall->SetActorLocation(
			ShadowGamepadStartLocation + AimDirection * 250.0f,
			false, nullptr, ETeleportType::TeleportPhysics);
		ShadowGamepadEventStart = CombatEvents.Num();
		SendPressAndRelease(EKeys::Gamepad_LeftShoulder);
		return true;
	}

	bool RunStartupCancellationChecks()
	{
		const FVector CancellationOrigin = TestOrigin + FVector(0.0f, 1000.0f, 0.0f);
		if (!SpawnCombatant(*World.Get(), CancellationOrigin,
			TEXT("PIE.Skill.CancelSource"), CancellationSource)
			|| !SpawnCombatant(*World.Get(), CancellationOrigin + AimDirection * 100.0f,
				TEXT("PIE.Skill.CancelTarget"), CancellationTarget))
		{
			Fail(TEXT("The fixed PIE smoke could not spawn its Startup cancellation fixtures."));
			return false;
		}
		CancellationSource.Character->GetMesh()->SetWorldRotation(Character->GetMesh()->GetComponentRotation());
		SetAttributes(*CancellationSource.ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);

		CancellationSource.ASC->AbilityInputTagPressed(ShadowData->InputTag);
		bDeadStartupCancelled = IsAbilityActive(*CancellationSource.ASC, ShadowData->AbilityTag)
			&& CancellationSource.Character->GetPlayerSkillComponent()->GetCurrentPhase()
				== EPRPlayerSkillPhase::Startup;
		CancellationSource.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
		CancellationSource.ASC->AbilityInputTagReleased(ShadowData->InputTag);
		bDeadStartupCancelled = bDeadStartupCancelled
			&& !IsAbilityActive(*CancellationSource.ASC, ShadowData->AbilityTag)
			&& FMath::IsNearlyEqual(CancellationSource.Attributes->GetEnergy(), 100.0f)
			&& CancellationSource.ASC->GetGameplayTagCount(ShadowCooldownTag()) == 0;
		CancellationSource.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);

		CancellationSource.ASC->AbilityInputTagPressed(FireData->InputTag);
		bStunnedStartupCancelled = IsAbilityActive(*CancellationSource.ASC, FireData->AbilityTag)
			&& CancellationSource.Character->GetPlayerSkillComponent()->GetCurrentPhase()
				== EPRPlayerSkillPhase::Startup;
		CancellationSource.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateStunnedTag(), 1, EGameplayTagReplicationState::TagOnly);
		CancellationSource.ASC->AbilityInputTagReleased(FireData->InputTag);
		bStunnedStartupCancelled = bStunnedStartupCancelled
			&& !IsAbilityActive(*CancellationSource.ASC, FireData->AbilityTag)
			&& FMath::IsNearlyEqual(CancellationSource.Attributes->GetEnergy(), 100.0f)
			&& CancellationSource.ASC->GetGameplayTagCount(FireCooldownTag()) == 0;
		CancellationSource.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateStunnedTag(), 0, EGameplayTagReplicationState::TagOnly);

		CancellationSource.ASC->AbilityInputTagPressed(ShadowData->InputTag);
		APRPlayerSkillCombatTestCharacter* OldAvatar = CancellationSource.Character.Get();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APRPlayerSkillCombatTestCharacter* Replacement =
			World->SpawnActor<APRPlayerSkillCombatTestCharacter>(
				APRPlayerSkillCombatTestCharacter::StaticClass(),
				CancellationOrigin,
				FRotator::ZeroRotator,
				SpawnParameters);
		bAvatarStartupCancelled = IsValid(Replacement)
			&& IsAbilityActive(*CancellationSource.ASC, ShadowData->AbilityTag);
		if (Replacement)
		{
			Replacement->ConfigureTarget(TEXT("PIE.Skill.ReplacementAvatar"), true);
			ConfigureCharacterCollision(*Replacement);
			CancellationSource.PreviousAvatar = OldAvatar;
			CancellationSource.Controller->Possess(Replacement);
			CancellationSource.Character = Replacement;
			Replacement->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		}
		CancellationSource.ASC->AbilityInputTagReleased(ShadowData->InputTag);
		bAvatarStartupCancelled = bAvatarStartupCancelled
			&& CancellationSource.ASC->GetAvatarActor() == Replacement
			&& !IsAbilityActive(*CancellationSource.ASC, ShadowData->AbilityTag)
			&& OldAvatar->GetPlayerSkillComponent()->GetCurrentPhase() == EPRPlayerSkillPhase::Idle
			&& FMath::IsNearlyEqual(CancellationSource.Attributes->GetEnergy(), 100.0f)
			&& CancellationSource.ASC->GetGameplayTagCount(ShadowCooldownTag()) == 0;

		return bDeadStartupCancelled && bStunnedStartupCancelled && bAvatarStartupCancelled;
	}

	void SetAttributes(
		UPRAbilitySystemComponent& TargetASC,
		const float Health,
		const float Shield,
		const float Energy,
		const float AttackPower) const
	{
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), Health);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), Shield);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), Energy);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetAttackPowerAttribute(), AttackPower);
	}

	void SetAutomationTimeDilation(const float TimeDilation) const
	{
		if (World.IsValid() && IsValid(World->GetWorldSettings()))
		{
			World->GetWorldSettings()->SetTimeDilation(TimeDilation);
		}
	}

	void PrepareTarget(FTransientCombatant& Target, const FVector Location) const
	{
		Target.Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Target.Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*Target.ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
	}

	void MoveFar(FTransientCombatant& Target) const
	{
		Target.Character->SetActorLocation(
			Character->GetActorLocation() + AimDirection * 2000.0f,
			false, nullptr, ETeleportType::TeleportPhysics);
		SetAttributes(*Target.ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
	}

	bool CheckEnergy(const float Expected) const
	{
		return Attributes && FMath::IsNearlyEqual(Attributes->GetEnergy(), Expected, 0.1f);
	}

	bool CheckPlanarTravel(const FVector Start, const float Minimum, const float Maximum) const
	{
		const FVector Delta = Character->GetActorLocation() - Start;
		const float AlongAim = FVector::DotProduct(Delta, AimDirection);
		return AlongAim >= Minimum && AlongAim <= Maximum
			&& FMath::IsNearlyEqual(Character->GetActorLocation().Y, Start.Y, 1.0f);
	}

	int32 CountCombatEvents(
		const int32 FirstIndex,
		const FGameplayTag AbilityTag,
		const FGameplayTag EventTag,
		const AActor* Target) const
	{
		int32 Count = 0;
		for (int32 Index = FirstIndex; Index < CombatEvents.Num(); ++Index)
		{
			const FPRCombatEvent& Event = CombatEvents[Index];
			if (Event.AbilityTag == AbilityTag && Event.EventTag == EventTag
				&& (!Target || Event.Target.Get() == Target))
			{
				++Count;
			}
		}
		return Count;
	}

	bool HasBurning(const FTransientCombatant& Target) const
	{
		if (!Target.ASC.IsValid())
		{
			return false;
		}
		FGameplayTagContainer BurningTags;
		BurningTags.AddTag(UPRTagLibrary::GetStateBurningTag());
		return Target.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateBurningTag())
			|| !Target.ASC->GetActiveEffects(
				FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(BurningTags)).IsEmpty();
	}

	bool IsAbilityActive(const FGameplayTag AbilityTag) const
	{
		return ASC.IsValid() && IsAbilityActive(*ASC.Get(), AbilityTag);
	}

	static bool IsAbilityActive(
		const UPRAbilitySystemComponent& TargetASC,
		const FGameplayTag AbilityTag)
	{
		FPRAbilityRuntimeState State;
		return TargetASC.GetAbilityRuntimeStateByAbilityTag(AbilityTag, State) && State.bActive;
	}

	static FGameplayTag ShadowCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.ShadowThrust"));
	}

	static FGameplayTag FireCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.FireSlash"));
	}

	void SendPressAndRelease(const FKey& Key)
	{
		SendKey(Key, IE_Pressed);
		SendKey(Key, IE_Released);
	}

	void SendKey(const FKey& Key, const EInputEvent Event)
	{
		if (Viewport.IsValid() && InputViewport
			&& Viewport->GetGameViewport() == InputViewport)
		{
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(
				Key, Event, 1.0f, -1, InputDevice, false, InputViewport));
		}
	}

	void ReleaseInputs()
	{
		if (!Viewport.IsValid() || !InputViewport)
		{
			return;
		}
		const FKey Keys[] = {
			EKeys::U,
			EKeys::I,
			EKeys::Gamepad_LeftShoulder,
			EKeys::Gamepad_RightTrigger};
		for (const FKey& Key : Keys)
		{
			SendKey(Key, IE_Released);
		}
	}

	bool Finish()
	{
		const bool bCleanRuntime = !IsAbilityActive(ShadowData->AbilityTag)
			&& !IsAbilityActive(FireData->AbilityTag)
			&& !HasBurning(TargetA) && !HasBurning(TargetB)
			&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle
			&& !SkillComponent->GetActiveDisplacementRequestId().IsValid();
		const bool bPassed = bKeyboardShadowPassed && bGamepadShadowPassed
			&& bKeyboardFirePassed && bGamepadFirePassed
			&& bFireCancelledAfterCommit && bDeadStartupCancelled
			&& bStunnedStartupCancelled && bAvatarStartupCancelled
			&& bCleanRuntime && CheckFormalSpecContract();

		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetNumberField(TEXT("formalSpecCount"), ASC->GetActivatableAbilities().Num());
		Json->SetBoolField(TEXT("keyboardShadow"), bKeyboardShadowPassed);
		Json->SetBoolField(TEXT("gamepadShadow"), bGamepadShadowPassed);
		Json->SetBoolField(TEXT("keyboardFire"), bKeyboardFirePassed);
		Json->SetBoolField(TEXT("gamepadFire"), bGamepadFirePassed);
		Json->SetBoolField(TEXT("fireCancelledAfterCommit"), bFireCancelledAfterCommit);
		Json->SetBoolField(TEXT("deadStartupCancelled"), bDeadStartupCancelled);
		Json->SetBoolField(TEXT("stunnedStartupCancelled"), bStunnedStartupCancelled);
		Json->SetBoolField(TEXT("avatarStartupCancelled"), bAvatarStartupCancelled);
		Json->SetBoolField(TEXT("runtimeClean"), bCleanRuntime);
		Json->SetNumberField(TEXT("finalEnergy"), Attributes->GetEnergy());
		Json->SetNumberField(TEXT("combatEventCount"), CombatEvents.Num());
		Json->SetNumberField(TEXT("lifecycleEventCount"), LifecycleEvents.Num());

		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		Cleanup();
		if (bPassed)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	void Advance(const ESmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		return World.IsValid() ? World->GetTimeSeconds() : 0.0;
	}

	void DestroyWall()
	{
		if (Wall.IsValid())
		{
			Wall->Destroy();
			Wall.Reset();
		}
	}

	static void DestroyCombatant(FTransientCombatant& Combatant)
	{
		if (Combatant.ASC.IsValid())
		{
			Combatant.ASC->CancelAllAbilities();
			Combatant.ASC->SetLooseGameplayTagCount(
				UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);
			Combatant.ASC->SetLooseGameplayTagCount(
				UPRTagLibrary::GetStateStunnedTag(), 0, EGameplayTagReplicationState::TagOnly);
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->UnPossess();
		}
		if (Combatant.Character.IsValid())
		{
			Combatant.Character->Destroy();
		}
		if (Combatant.PreviousAvatar.IsValid())
		{
			Combatant.PreviousAvatar->Destroy();
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->Destroy();
		}
		if (Combatant.PlayerState.IsValid())
		{
			Combatant.PlayerState->Destroy();
		}
		Combatant = FTransientCombatant();
	}

	void Cleanup()
	{
		ReleaseInputs();
		SetAutomationTimeDilation(InitialTimeDilation);
		DestroyWall();
		if (CombatSubsystem.IsValid() && CombatEventHandle.IsValid())
		{
			CombatSubsystem->OnCombatEvent().Remove(CombatEventHandle);
			CombatEventHandle.Reset();
		}
		if (ASC.IsValid() && LifecycleHandle.IsValid())
		{
			ASC->OnAbilityLifecycleEvent().Remove(LifecycleHandle);
			LifecycleHandle.Reset();
		}
		DestroyCombatant(CancellationTarget);
		DestroyCombatant(CancellationSource);
		DestroyCombatant(TargetB);
		DestroyCombatant(TargetA);
		if (ASC.IsValid())
		{
			ASC->CancelAllAbilities();
			SetAttributes(*ASC.Get(), InitialHealth, InitialShield, InitialEnergy, InitialAttackPower);
		}
		if (Character.IsValid())
		{
			Character->SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Character->GetCharacterMovement()->SetMovementMode(InitialMovementMode);
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		InputViewport = nullptr;
	}

	void Fail(const FString& Message)
	{
		Cleanup();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	const UPRAttributeSet* Attributes = nullptr;
	TWeakObjectPtr<UPRPlayerSkillComponent> SkillComponent;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	FViewport* InputViewport = nullptr;
	FInputDeviceId InputDevice = INPUTDEVICEID_NONE;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> ShadowData;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> FireData;
	TWeakObjectPtr<AActor> Wall;
	FTransientCombatant TargetA;
	FTransientCombatant TargetB;
	FTransientCombatant CancellationSource;
	FTransientCombatant CancellationTarget;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle LifecycleHandle;
	TArray<FPRCombatEvent> CombatEvents;
	TArray<FPRAbilityLifecycleEvent> LifecycleEvents;
	ESmokePhase Phase = ESmokePhase::ShadowKeyboardStart;
	double PhaseStartTime = 0.0;
	FTransform InitialTransform = FTransform::Identity;
	EMovementMode InitialMovementMode = MOVE_Walking;
	FVector TestOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	FVector ShadowKeyboardStartLocation = FVector::ZeroVector;
	FVector ShadowGamepadStartLocation = FVector::ZeroVector;
	int32 ShadowKeyboardEventStart = 0;
	int32 FireKeyboardEventStart = 0;
	int32 ShadowGamepadEventStart = 0;
	int32 FireGamepadEventStart = 0;
	float InitialHealth = 0.0f;
	float InitialShield = 0.0f;
	float InitialEnergy = 0.0f;
	float InitialAttackPower = 0.0f;
	float InitialTimeDilation = 1.0f;
	bool bKeyboardShadowPassed = false;
	bool bGamepadShadowPassed = false;
	bool bKeyboardFirePassed = false;
	bool bGamepadFirePassed = false;
	bool bFireCancelledAfterCommit = false;
	bool bDeadStartupCancelled = false;
	bool bStunnedStartupCancelled = false;
	bool bAvatarStartupCancelled = false;
};

enum class ECheckpointCSmokePhase : uint8
{
	ThunderStart,
	ThunderEarlyWait,
	ThunderImpactWait,
	ThunderStatusExpiryWait,
	ThunderCooldownWait,
	ThunderInvalidTargetStart,
	ThunderInvalidTargetCommitWait,
	ThunderInvalidTargetWait,
	AfterimageStart,
	AfterimageCommitWait,
	AfterimageWait,
	CleanupWait,
	Complete
};

class FPIEPlayerSkillCheckpointCRunner
	: public TSharedFromThis<FPIEPlayerSkillCheckpointCRunner>
{
public:
	static UToolCallAsyncResultString* Start(const bool bInAllowExtendedAbilitySet = false)
	{
		TSharedRef<FPIEPlayerSkillCheckpointCRunner> Runner =
			MakeShared<FPIEPlayerSkillCheckpointCRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(
			NewObject<UToolCallAsyncResultString>());
		Runner->bAllowExtendedAbilitySet = bInAllowExtendedAbilitySet;
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		Runner->PhaseStartTime = Runner->GetPlayWorldTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float) mutable
			{
				const bool bContinue = RunnerPtr->Tick();
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIEPlayerSkillCheckpointCRunner()
	{
		Cleanup();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Fail(TEXT("RunPIEPlayerSkillCheckpointCSmoke requires an active authoritative in-process PIE world."));
			return false;
		}

		World = PlayWorld;
		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		ASC = PlayerState.IsValid() ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		Attributes = PlayerState.IsValid() ? PlayerState->GetAttributeSet() : nullptr;
		SkillComponent = Character.IsValid() ? Character->GetPlayerSkillComponent() : nullptr;
		CombatSubsystem = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		Viewport = PlayWorld->GetGameViewport();
		InputViewport = Viewport.IsValid() ? Viewport->GetGameViewport() : nullptr;
		InputDevice = UInputDeviceLibrary::GetDefaultInputDevice();
		ThunderData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, ThunderDataPath);
		AfterimageData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, AfterimageDataPath);
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid()
			|| !ASC.IsValid() || !Attributes || !SkillComponent.IsValid()
			|| !CombatSubsystem.IsValid() || !Viewport.IsValid() || !InputViewport
			|| !InputDevice.IsValid() || !ThunderData.IsValid() || !AfterimageData.IsValid())
		{
			Fail(TEXT("PIE is missing the formal player, checkpoint-C DataAssets, viewport, ASC, or combat objects."));
			return false;
		}

		InitialTransform = Character->GetActorTransform();
		InitialMovementMode = Character->GetCharacterMovement()->MovementMode;
		InitialHealth = Attributes->GetHealth();
		InitialShield = Attributes->GetShield();
		InitialEnergy = Attributes->GetEnergy();
		InitialAttackPower = Attributes->GetAttackPower();
		InitialTimeDilation = PlayWorld->GetWorldSettings()->TimeDilation;

		if (!CheckFormalSpecContract() || !CheckFrozenDataContract())
		{
			Fail(TEXT("The formal v0.2.0-C AbilitySet or skill data contract is not exact."));
			return false;
		}

		AimDirection = Character->GetActorForwardVector();
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			AimDirection = Mesh->GetRightVector();
		}
		AimDirection.Y = 0.0f;
		AimDirection = AimDirection.GetSafeNormal();
		if (!AimDirection.IsNormalized() || FMath::Abs(AimDirection.X) < 0.99f
			|| FMath::Abs(AimDirection.Z) > 0.01f)
		{
			Fail(TEXT("The formal player mesh does not expose a finite horizontal X/Z skill aim."));
			return false;
		}

		TestOrigin = InitialTransform.GetLocation() + FVector(0.0f, 0.0f, 2000.0f);
		SetAutomationTimeDilation(1.0f);
		Character->SetActorLocation(TestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*ASC, 100.0f, 0.0f, 100.0f, 10.0f);

		if (!SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 300.0f,
			TEXT("PIE.Skill.CheckpointC.TargetA"), TargetA)
			|| !SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 2000.0f,
				TEXT("PIE.Skill.CheckpointC.TargetB"), TargetB))
		{
			Fail(TEXT("The fixed checkpoint-C PIE smoke could not spawn its transient combat targets."));
			return false;
		}

		CombatEventHandle = CombatSubsystem->OnCombatEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEPlayerSkillCheckpointCRunner>(AsShared())](
				const FPRCombatEvent& Event)
			{
				if (TSharedPtr<FPIEPlayerSkillCheckpointCRunner> This = WeakThis.Pin())
				{
					This->CombatEvents.Add(Event);
				}
			});
		LifecycleHandle = ASC->OnAbilityLifecycleEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEPlayerSkillCheckpointCRunner>(AsShared())](
				const FPRAbilityLifecycleEvent& Event)
			{
				if (TSharedPtr<FPIEPlayerSkillCheckpointCRunner> This = WeakThis.Pin())
				{
					This->LifecycleEvents.Add(Event);
				}
			});
		Phase = ECheckpointCSmokePhase::ThunderStart;
		return true;
	}

	bool Tick()
	{
		if (!World.IsValid() || !Character.IsValid() || !ASC.IsValid()
			|| !CombatSubsystem.IsValid() || !GEditor || GEditor->PlayWorld != World.Get())
		{
			Fail(TEXT("PIE ended or ProjectR checkpoint-C objects became invalid during smoke."));
			return false;
		}

		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ECheckpointCSmokePhase::ThunderStart:
			PrepareTarget(TargetA, Character->GetActorLocation() + AimDirection * 300.0f);
			MoveFar(TargetB);
			ThunderEventStart = CombatEvents.Num();
			SendPressAndRelease(EKeys::O);
			Advance(ECheckpointCSmokePhase::ThunderEarlyWait);
			break;

		case ECheckpointCSmokePhase::ThunderEarlyWait:
			if (Elapsed < 0.30)
			{
				break;
			}
			bThunderDelayPassed = FMath::IsNearlyEqual(TargetA.Attributes->GetHealth(), 100.0f, 0.1f)
				&& FMath::IsNearlyEqual(Attributes->GetEnergy(), 70.0f, 0.1f)
				&& CountCombatEvents(ThunderEventStart, ThunderData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag()) == 0
				&& IsAbilityActive(ThunderData->AbilityTag)
				&& ASC->GetGameplayTagCount(ThunderCooldownTag()) == 1;
			if (!bThunderDelayPassed)
			{
				const FString LastFailureTags = LifecycleEvents.IsEmpty()
					? TEXT("None")
					: LifecycleEvents.Last().FailureTags.ToStringSimple();
				const int32 LastEventType = LifecycleEvents.IsEmpty()
					? -1
					: static_cast<int32>(LifecycleEvents.Last().EventType);
				Fail(FString::Printf(
					TEXT("ThunderDrop early gate mismatch: Energy=%.2f Health=%.2f Damage=%d Active=%s Cooldown=%d Phase=%d Lifecycle=%d LastType=%d FailureTags=%s."),
					Attributes->GetEnergy(),
					TargetA.Attributes->GetHealth(),
					CountCombatEvents(ThunderEventStart, ThunderData->AbilityTag,
						UPRTagLibrary::GetCombatEventDamageTag()),
					IsAbilityActive(ThunderData->AbilityTag) ? TEXT("true") : TEXT("false"),
					ASC->GetGameplayTagCount(ThunderCooldownTag()),
					static_cast<int32>(SkillComponent->GetCurrentPhase()),
					LifecycleEvents.Num(),
					LastEventType,
					*LastFailureTags));
				return false;
			}
			Advance(ECheckpointCSmokePhase::ThunderImpactWait);
			break;

		case ECheckpointCSmokePhase::ThunderImpactWait:
			if (Elapsed < 0.40
				|| (bAllowExtendedAbilitySet && IsAbilityActive(ThunderData->AbilityTag) && Elapsed < 1.00))
			{
				break;
			}
			bThunderImpactPassed = FMath::IsNearlyEqual(TargetA.Attributes->GetHealth(), 55.0f, 0.1f)
				&& CountCombatEvents(ThunderEventStart, ThunderData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag()) == 1
				&& TargetA.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
				&& !IsAbilityActive(ThunderData->AbilityTag)
				&& ASC->GetGameplayTagCount(ThunderCooldownTag()) == 1;
			if (!bThunderImpactPassed)
			{
				Fail(FString::Printf(
					TEXT("ThunderDrop impact mismatch: Energy=%.2f Health=%.2f Damage=%d Stunned=%s Active=%s Cooldown=%d."),
					Attributes->GetEnergy(), TargetA.Attributes->GetHealth(),
					CountCombatEvents(ThunderEventStart, ThunderData->AbilityTag,
						UPRTagLibrary::GetCombatEventDamageTag()),
					TargetA.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()) ? TEXT("true") : TEXT("false"),
					IsAbilityActive(ThunderData->AbilityTag) ? TEXT("true") : TEXT("false"),
					ASC->GetGameplayTagCount(ThunderCooldownTag())));
				return false;
			}
			Advance(ECheckpointCSmokePhase::ThunderStatusExpiryWait);
			break;

		case ECheckpointCSmokePhase::ThunderStatusExpiryWait:
			if (Elapsed < 0.85)
			{
				break;
			}
			bThunderStatusDurationPassed =
				!TargetA.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag());
			if (!bThunderStatusDurationPassed)
			{
				Fail(TEXT("ThunderDrop Stunned did not expire after its 0.75 second duration."));
				return false;
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointCSmokePhase::ThunderCooldownWait);
			break;

		case ECheckpointCSmokePhase::ThunderCooldownWait:
			if (ASC->GetGameplayTagCount(ThunderCooldownTag()) != 0)
			{
				if (Elapsed > 8.0)
				{
					Fail(TEXT("ThunderDrop cooldown did not expire before target invalidation validation."));
					return false;
				}
				break;
			}
			SetAutomationTimeDilation(1.0f);
			Advance(ECheckpointCSmokePhase::ThunderInvalidTargetStart);
			break;

		case ECheckpointCSmokePhase::ThunderInvalidTargetStart:
			PrepareTarget(TargetA, Character->GetActorLocation() + AimDirection * 300.0f);
			MoveFar(TargetB);
			ThunderInvalidEventStart = CombatEvents.Num();
			SendPressAndRelease(EKeys::O);
			Advance(ECheckpointCSmokePhase::ThunderInvalidTargetCommitWait);
			break;

		case ECheckpointCSmokePhase::ThunderInvalidTargetCommitWait:
			if (Elapsed < 0.15)
			{
				break;
			}
			if (!FMath::IsNearlyEqual(Attributes->GetEnergy(), 40.0f, 0.1f)
				|| ASC->GetGameplayTagCount(ThunderCooldownTag()) != 1)
			{
				Fail(TEXT("ThunderDrop target invalidation setup did not Commit exactly once."));
				return false;
			}
			if (TargetA.Character.IsValid())
			{
				TargetA.Character->Destroy();
				TargetA.Character.Reset();
			}
			Advance(ECheckpointCSmokePhase::ThunderInvalidTargetWait);
			break;

		case ECheckpointCSmokePhase::ThunderInvalidTargetWait:
			if (Elapsed < 0.75)
			{
				break;
			}
			bThunderInvalidTargetPassed =
				CountCombatEvents(ThunderInvalidEventStart, ThunderData->AbilityTag,
					UPRTagLibrary::GetCombatEventDamageTag()) == 0
				&& !IsAbilityActive(ThunderData->AbilityTag)
				&& !TargetB.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag());
			if (!bThunderInvalidTargetPassed)
			{
				Fail(TEXT("ThunderDrop target invalidation produced damage, Stunned, or an active ability leak."));
				return false;
			}
			Advance(ECheckpointCSmokePhase::AfterimageStart);
			break;

		case ECheckpointCSmokePhase::AfterimageStart:
			PrepareTarget(TargetB, Character->GetActorLocation() + AimDirection * 300.0f);
			AfterimageStartLocation = Character->GetActorLocation();
			AfterimageEventStart = CombatEvents.Num();
			// Afterimage is held-input. Let the input-triggered ability activate
			// before releasing it; a same-frame release can otherwise arrive before
			// deferred activation and leave the ability held until later cleanup.
			SendKey(EKeys::L, IE_Pressed);
			Advance(ECheckpointCSmokePhase::AfterimageCommitWait);
			break;

		case ECheckpointCSmokePhase::AfterimageCommitWait:
			if (Elapsed < 0.15)
			{
				break;
			}
			if (APRSkillDecoyActor* Decoy = FindOwnedDecoy())
			{
				const bool bFirstConsume = Decoy->ConsumeAttackProxy(TargetB.Character.Get());
				const bool bSecondConsume = Decoy->ConsumeAttackProxy(TargetB.Character.Get());
				bDecoyConsumedOnce = bFirstConsume && !bSecondConsume;
			}
			bAfterimageStarted = FMath::IsNearlyEqual(Attributes->GetEnergy(), 25.0f, 0.1f)
				&& ASC->GetGameplayTagCount(AfterimageCooldownTag()) == 1
				&& ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag())
				&& CountResponseEvents(AfterimageEventStart,
					UPRTagLibrary::GetCombatResponseDecoyCreatedTag()) == 1
				&& bDecoyConsumedOnce;
			if (!bAfterimageStarted)
			{
				Fail(TEXT("AfterimageDodge did not Commit, apply Invulnerable, or create one consumable decoy."));
				return false;
			}
			SendKey(EKeys::L, IE_Released);
			Advance(ECheckpointCSmokePhase::AfterimageWait);
			break;

		case ECheckpointCSmokePhase::AfterimageWait:
			if (Elapsed < 0.40)
			{
				break;
			}
			{
				const FVector Delta = Character->GetActorLocation() - AfterimageStartLocation;
				const float AlongAim = FVector::DotProduct(Delta, AimDirection);
				const int32 DisplacementEvents = CountResponseEvents(AfterimageEventStart,
					UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
				const int32 DecoyConsumedEvents = CountResponseEvents(AfterimageEventStart,
					UPRTagLibrary::GetCombatResponseDecoyConsumedTag());
				const int32 PerfectTimingEvents = CountResponseEvents(AfterimageEventStart,
					UPRTagLibrary::GetCombatResponsePerfectTimingTag());
				bAfterimagePassed = AlongAim >= 280.0f && AlongAim <= 320.0f
					&& FMath::Abs(Delta.Y) <= 1.0f
					&& !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag())
					&& !IsAbilityActive(AfterimageData->AbilityTag)
					&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle
					&& !SkillComponent->GetActiveDisplacementRequestId().IsValid()
					&& CountOwnedDecoys() == 0
					&& DisplacementEvents == 1
					&& DecoyConsumedEvents == 1
					&& PerfectTimingEvents == 1;
				if (!bAfterimagePassed)
				{
					Fail(FString::Printf(
						TEXT("AfterimageDodge cleanup mismatch: AlongAim=%.2f DeltaY=%.2f Invulnerable=%s Active=%s Phase=%d Displacement=%d DecoyConsumed=%d PerfectTiming=%d Decoys=%d."),
						AlongAim,
						Delta.Y,
						ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag()) ? TEXT("true") : TEXT("false"),
						IsAbilityActive(AfterimageData->AbilityTag) ? TEXT("true") : TEXT("false"),
						static_cast<int32>(SkillComponent->GetCurrentPhase()),
						DisplacementEvents,
						DecoyConsumedEvents,
						PerfectTimingEvents,
						CountOwnedDecoys()));
					return false;
				}
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointCSmokePhase::CleanupWait);
			break;

		case ECheckpointCSmokePhase::CleanupWait:
			if (ASC->GetGameplayTagCount(ThunderCooldownTag()) != 0
				|| ASC->GetGameplayTagCount(AfterimageCooldownTag()) != 0)
			{
				if (Elapsed > 8.0)
				{
					Fail(TEXT("Checkpoint-C cooldowns did not expire before the runtime leak check."));
					return false;
				}
				break;
			}
			SetAutomationTimeDilation(1.0f);
			bRuntimeClean = CheckRuntimeClean();
			if (!bRuntimeClean)
			{
				Fail(TEXT("Checkpoint-C left an ActiveEffect, timer-owned phase, decoy, displacement, tag, or Spec leak."));
				return false;
			}
			Advance(ECheckpointCSmokePhase::Complete);
			break;

		case ECheckpointCSmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	bool CheckFormalSpecContract() const
	{
		const int32 ExpectedSpecCount = bAllowExtendedAbilitySet ? 6 : 4;
		if (ASC->GetActivatableAbilities().Num() != ExpectedSpecCount)
		{
			return false;
		}
		const TCHAR* ExpectedTags[] = {
			TEXT("Skill.ShadowThrust"),
			TEXT("Skill.FireSlash"),
			TEXT("Skill.ThunderDrop"),
			TEXT("Skill.AfterimageDodge")};
		for (const TCHAR* TagName : ExpectedTags)
		{
			FPRAbilityRuntimeState State;
			if (!ASC->GetAbilityRuntimeStateByAbilityTag(
				FGameplayTag::RequestGameplayTag(TagName), State))
			{
				return false;
			}
		}
		FPRAbilityRuntimeState ThunderState;
		FPRAbilityRuntimeState AfterimageState;
		return ASC->GetAbilityRuntimeStateByAbilityTag(ThunderData->AbilityTag, ThunderState)
			&& ASC->GetAbilityRuntimeStateByAbilityTag(AfterimageData->AbilityTag, AfterimageState)
			&& ThunderState.InputTag == ThunderData->InputTag
			&& AfterimageState.InputTag == AfterimageData->InputTag;
	}

	bool CheckFrozenDataContract() const
	{
		const UPRThunderDropSkillFragment* ThunderFragment =
			Cast<UPRThunderDropSkillFragment>(ThunderData->SkillFragment);
		const UPRAfterimageDodgeSkillFragment* AfterimageFragment =
			Cast<UPRAfterimageDodgeSkillFragment>(AfterimageData->SkillFragment);
		return ThunderFragment && AfterimageFragment
			&& ThunderData->InputTag == UPRTagLibrary::GetInputSkillThunderDropTag()
			&& FMath::IsNearlyEqual(ThunderData->EnergyCost, 30.0f)
			&& FMath::IsNearlyEqual(ThunderData->CooldownDuration, 7.0f)
			&& FMath::IsNearlyEqual(ThunderData->ActiveDuration, 0.60f)
			&& FMath::IsNearlyEqual(ThunderData->Range, 700.0f)
			&& FMath::IsNearlyEqual(ThunderData->Radius, 220.0f)
			&& FMath::IsNearlyEqual(ThunderData->StatusDuration, 0.75f)
			&& FMath::IsNearlyEqual(ThunderFragment->FallbackDistance, 450.0f)
			&& AfterimageData->InputTag == UPRTagLibrary::GetInputDodgeTag()
			&& FMath::IsNearlyEqual(AfterimageData->EnergyCost, 15.0f)
			&& FMath::IsNearlyEqual(AfterimageData->CooldownDuration, 2.5f)
			&& FMath::IsNearlyEqual(AfterimageData->ActiveDuration, 0.18f)
			&& FMath::IsNearlyEqual(AfterimageData->DisplacementDistance, 300.0f)
			&& FMath::IsNearlyEqual(AfterimageData->StatusDuration, 0.22f)
			&& FMath::IsNearlyEqual(AfterimageFragment->PerfectTimingWindow, 0.14f)
			&& FMath::IsNearlyEqual(AfterimageFragment->DecoyLifetime, 1.5f);
	}

	void SetAttributes(
		UPRAbilitySystemComponent& TargetASC,
		const float Health,
		const float Shield,
		const float Energy,
		const float AttackPower) const
	{
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), Health);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), Shield);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), Energy);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetAttackPowerAttribute(), AttackPower);
	}

	void PrepareTarget(FTransientCombatant& Target, const FVector Location) const
	{
		if (!Target.Character.IsValid() || !Target.ASC.IsValid())
		{
			return;
		}
		Target.Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Target.Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*Target.ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
	}

	void MoveFar(FTransientCombatant& Target) const
	{
		PrepareTarget(Target, Character->GetActorLocation() + AimDirection * 2000.0f);
	}

	int32 CountCombatEvents(
		const int32 FirstIndex,
		const FGameplayTag AbilityTag,
		const FGameplayTag EventTag) const
	{
		int32 Count = 0;
		for (int32 Index = FirstIndex; Index < CombatEvents.Num(); ++Index)
		{
			const FPRCombatEvent& Event = CombatEvents[Index];
			if (Event.AbilityTag == AbilityTag && Event.EventTag == EventTag)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountResponseEvents(const int32 FirstIndex, const FGameplayTag ResponseTag) const
	{
		int32 Count = 0;
		for (int32 Index = FirstIndex; Index < CombatEvents.Num(); ++Index)
		{
			const FPRCombatEvent& Event = CombatEvents[Index];
			if (Event.AbilityTag == AfterimageData->AbilityTag
				&& Event.EventTag == UPRTagLibrary::GetCombatEventAbilityOutcomeTag()
				&& Event.ResponseTags.HasTagExact(ResponseTag))
			{
				++Count;
			}
		}
		return Count;
	}

	APRSkillDecoyActor* FindOwnedDecoy() const
	{
		if (!World.IsValid())
		{
			return nullptr;
		}
		for (TActorIterator<APRSkillDecoyActor> It(World.Get()); It; ++It)
		{
			if (IsValid(*It) && It->GetOwner() == Character.Get())
			{
				return *It;
			}
		}
		return nullptr;
	}

	int32 CountOwnedDecoys() const
	{
		int32 Count = 0;
		if (World.IsValid())
		{
			for (TActorIterator<APRSkillDecoyActor> It(World.Get()); It; ++It)
			{
				if (IsValid(*It) && It->GetOwner() == Character.Get())
				{
					++Count;
				}
			}
		}
		return Count;
	}

	bool CheckRuntimeClean() const
	{
		return CheckFormalSpecContract()
			&& !IsAbilityActive(ThunderData->AbilityTag)
			&& !IsAbilityActive(AfterimageData->AbilityTag)
			&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle
			&& !SkillComponent->GetActiveDisplacementRequestId().IsValid()
			&& !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag())
			&& ASC->GetGameplayTagCount(ThunderCooldownTag()) == 0
			&& ASC->GetGameplayTagCount(AfterimageCooldownTag()) == 0
			&& ASC->GetActiveEffects(FGameplayEffectQuery()).IsEmpty()
			&& CountOwnedDecoys() == 0;
	}

	bool IsAbilityActive(const FGameplayTag AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC.IsValid()
			&& ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State)
			&& State.bActive;
	}

	static FGameplayTag ThunderCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.ThunderDrop"));
	}

	static FGameplayTag AfterimageCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.AfterimageDodge"));
	}

	void SendPressAndRelease(const FKey& Key)
	{
		SendKey(Key, IE_Pressed);
		SendKey(Key, IE_Released);
	}

	void SendKey(const FKey& Key, const EInputEvent Event)
	{
		if (Viewport.IsValid() && InputViewport && Viewport->GetGameViewport() == InputViewport)
		{
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(
				Key, Event, 1.0f, -1, InputDevice, false, InputViewport));
		}
	}

	void ReleaseInputs()
	{
		SendKey(EKeys::O, IE_Released);
		SendKey(EKeys::L, IE_Released);
	}

	void SetAutomationTimeDilation(const float TimeDilation) const
	{
		if (World.IsValid() && IsValid(World->GetWorldSettings()))
		{
			World->GetWorldSettings()->SetTimeDilation(TimeDilation);
		}
	}

	bool Finish()
	{
		const bool bPassed = bThunderDelayPassed && bThunderImpactPassed
			&& bThunderStatusDurationPassed && bThunderInvalidTargetPassed
			&& bAfterimageStarted && bDecoyConsumedOnce && bAfterimagePassed
			&& bRuntimeClean && CheckFormalSpecContract();

		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetNumberField(TEXT("formalSpecCount"), ASC->GetActivatableAbilities().Num());
		Json->SetBoolField(TEXT("thunderDelay"), bThunderDelayPassed);
		Json->SetBoolField(TEXT("thunderImpact"), bThunderImpactPassed);
		Json->SetBoolField(TEXT("thunderStatusDuration"), bThunderStatusDurationPassed);
		Json->SetBoolField(TEXT("thunderInvalidTarget"), bThunderInvalidTargetPassed);
		Json->SetBoolField(TEXT("afterimageStarted"), bAfterimageStarted);
		Json->SetBoolField(TEXT("decoyConsumedOnce"), bDecoyConsumedOnce);
		Json->SetBoolField(TEXT("afterimageDisplacementAndCleanup"), bAfterimagePassed);
		Json->SetBoolField(TEXT("runtimeClean"), bRuntimeClean);
		Json->SetNumberField(TEXT("finalEnergy"), Attributes->GetEnergy());
		Json->SetNumberField(TEXT("combatEventCount"), CombatEvents.Num());
		Json->SetNumberField(TEXT("lifecycleEventCount"), LifecycleEvents.Num());

		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		Cleanup();
		if (bPassed)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	void Advance(const ECheckpointCSmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		return World.IsValid() ? World->GetTimeSeconds() : 0.0;
	}

	static void DestroyCombatant(FTransientCombatant& Combatant)
	{
		if (Combatant.ASC.IsValid())
		{
			Combatant.ASC->CancelAllAbilities();
			Combatant.ASC->SetLooseGameplayTagCount(
				UPRTagLibrary::GetStateStunnedTag(), 0, EGameplayTagReplicationState::TagOnly);
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->UnPossess();
		}
		if (Combatant.Character.IsValid())
		{
			Combatant.Character->Destroy();
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->Destroy();
		}
		if (Combatant.PlayerState.IsValid())
		{
			Combatant.PlayerState->Destroy();
		}
		Combatant = FTransientCombatant();
	}

	void Cleanup()
	{
		ReleaseInputs();
		SetAutomationTimeDilation(InitialTimeDilation);
		if (CombatSubsystem.IsValid() && CombatEventHandle.IsValid())
		{
			CombatSubsystem->OnCombatEvent().Remove(CombatEventHandle);
			CombatEventHandle.Reset();
		}
		if (ASC.IsValid() && LifecycleHandle.IsValid())
		{
			ASC->OnAbilityLifecycleEvent().Remove(LifecycleHandle);
			LifecycleHandle.Reset();
		}
		if (World.IsValid())
		{
			for (TActorIterator<APRSkillDecoyActor> It(World.Get()); It; ++It)
			{
				if (IsValid(*It) && It->GetOwner() == Character.Get())
				{
					It->Destroy();
				}
			}
		}
		DestroyCombatant(TargetB);
		DestroyCombatant(TargetA);
		if (ASC.IsValid())
		{
			ASC->CancelAllAbilities();
			FGameplayTagContainer CleanupTags;
			CleanupTags.AddTag(ThunderCooldownTag());
			CleanupTags.AddTag(AfterimageCooldownTag());
			CleanupTags.AddTag(UPRTagLibrary::GetStateInvulnerableTag());
			ASC->RemoveActiveEffectsWithTags(CleanupTags);
			SetAttributes(*ASC.Get(), InitialHealth, InitialShield, InitialEnergy, InitialAttackPower);
		}
		if (Character.IsValid())
		{
			Character->SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Character->GetCharacterMovement()->SetMovementMode(InitialMovementMode);
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		InputViewport = nullptr;
	}

	void Fail(const FString& Message)
	{
		Cleanup();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	const UPRAttributeSet* Attributes = nullptr;
	TWeakObjectPtr<UPRPlayerSkillComponent> SkillComponent;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	FViewport* InputViewport = nullptr;
	FInputDeviceId InputDevice = INPUTDEVICEID_NONE;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> ThunderData;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> AfterimageData;
	FTransientCombatant TargetA;
	FTransientCombatant TargetB;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle LifecycleHandle;
	TArray<FPRCombatEvent> CombatEvents;
	TArray<FPRAbilityLifecycleEvent> LifecycleEvents;
	ECheckpointCSmokePhase Phase = ECheckpointCSmokePhase::ThunderStart;
	double PhaseStartTime = 0.0;
	FTransform InitialTransform = FTransform::Identity;
	EMovementMode InitialMovementMode = MOVE_Walking;
	FVector TestOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	FVector AfterimageStartLocation = FVector::ZeroVector;
	int32 ThunderEventStart = 0;
	int32 ThunderInvalidEventStart = 0;
	int32 AfterimageEventStart = 0;
	float InitialHealth = 0.0f;
	float InitialShield = 0.0f;
	float InitialEnergy = 0.0f;
	float InitialAttackPower = 0.0f;
	float InitialTimeDilation = 1.0f;
	bool bThunderDelayPassed = false;
	bool bThunderImpactPassed = false;
	bool bThunderStatusDurationPassed = false;
	bool bThunderInvalidTargetPassed = false;
	bool bAfterimageStarted = false;
	bool bDecoyConsumedOnce = false;
	bool bAfterimagePassed = false;
	bool bAllowExtendedAbilitySet = false;
	bool bRuntimeClean = false;
};

enum class ECheckpointDSmokePhase : uint8
{
	HookNoTargetStart,
	HookNoTargetWait,
	HookObstructedStart,
	HookObstructedWait,
	HookLightStart,
	HookLightInputRelease,
	HookLightWait,
	HookCooldownAfterLight,
	HookHeavyStart,
	HookHeavyWait,
	HookCooldownAfterHeavy,
	HookAnchoredStart,
	HookAnchoredWait,
	HookCooldownAfterAnchored,
	GuardKeyboardStart,
	GuardKeyboardPerfect,
	GuardKeyboardLate,
	GuardKeyboardRelease,
	GuardCooldownAfterKeyboard,
	GuardGamepadStart,
	GuardGamepadRelease,
	GuardCooldownAfterGamepad,
	GuardMaximumStart,
	GuardMaximumWait,
	CleanupWait,
	Complete
};

/**
 * Fixed checkpoint-D PIE evidence.  It deliberately uses only the formal player,
 * the L_CombatGym PIE world, and transient native actors; no package can be saved.
 */
class FPIEPlayerSkillCheckpointDRunner
	: public TSharedFromThis<FPIEPlayerSkillCheckpointDRunner>
{
public:
	static UToolCallAsyncResultString* Start(const bool bInImmediateGuardPerfect = false)
	{
		TSharedRef<FPIEPlayerSkillCheckpointDRunner> Runner =
			MakeShared<FPIEPlayerSkillCheckpointDRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(
			NewObject<UToolCallAsyncResultString>());
		Runner->bImmediateGuardPerfect = bInImmediateGuardPerfect;
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}
		Runner->PhaseStartTime = Runner->GetPlayWorldTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float) mutable
			{
				const bool bContinue = RunnerPtr->Tick();
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIEPlayerSkillCheckpointDRunner()
	{
		Cleanup();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Fail(TEXT("RunPIEPlayerSkillCheckpointDSmoke requires an active authoritative in-process PIE world."));
			return false;
		}
		World = PlayWorld;
		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		ASC = PlayerState.IsValid() ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		Attributes = PlayerState.IsValid() ? PlayerState->GetAttributeSet() : nullptr;
		SkillComponent = Character.IsValid() ? Character->GetPlayerSkillComponent() : nullptr;
		CombatSubsystem = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		Viewport = PlayWorld->GetGameViewport();
		InputViewport = Viewport.IsValid() ? Viewport->GetGameViewport() : nullptr;
		InputDevice = UInputDeviceLibrary::GetDefaultInputDevice();
		VectorHookData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, VectorHookDataPath);
		CounterProofWallData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, CounterProofWallDataPath);
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid()
			|| !ASC.IsValid() || !Attributes || !SkillComponent.IsValid() || !CombatSubsystem.IsValid()
			|| !Viewport.IsValid() || !InputViewport || !InputDevice.IsValid()
			|| !VectorHookData.IsValid() || !CounterProofWallData.IsValid())
		{
			Fail(TEXT("PIE is missing the formal player, checkpoint-D DataAssets, viewport, ASC, or combat objects."));
			return false;
		}
		if (!CheckFormalSpecContract() || !CheckFrozenDataContract())
		{
			Fail(TEXT("The formal v0.2.0-D AbilitySet or skill data contract is not exact."));
			return false;
		}

		InitialTransform = Character->GetActorTransform();
		InitialMovementMode = Character->GetCharacterMovement()->MovementMode;
		InitialHealth = Attributes->GetHealth();
		InitialShield = Attributes->GetShield();
		InitialEnergy = Attributes->GetEnergy();
		InitialAttackPower = Attributes->GetAttackPower();
		InitialTimeDilation = PlayWorld->GetWorldSettings()->TimeDilation;
		AimDirection = Character->GetMesh() ? Character->GetMesh()->GetRightVector()
			: Character->GetActorForwardVector();
		AimDirection.Y = 0.0f;
		AimDirection = AimDirection.GetSafeNormal();
		if (!AimDirection.IsNormalized() || FMath::Abs(AimDirection.X) < 0.99f
			|| FMath::Abs(AimDirection.Z) > 0.01f)
		{
			Fail(TEXT("The formal player mesh does not expose a finite horizontal X/Z skill aim."));
			return false;
		}
		TestOrigin = InitialTransform.GetLocation() + FVector(0.0f, 0.0f, 3000.0f);
		Character->SetActorLocation(TestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
		if (!SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 600.0f,
			TEXT("PIE.Skill.CheckpointD.TargetA"), TargetA)
			|| !SpawnCombatant(*PlayWorld, TestOrigin + AimDirection * 2000.0f,
				TEXT("PIE.Skill.CheckpointD.TargetB"), TargetB))
		{
			Fail(TEXT("The fixed checkpoint-D PIE smoke could not spawn its transient combat targets."));
			return false;
		}
		CombatEventHandle = CombatSubsystem->OnCombatEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEPlayerSkillCheckpointDRunner>(AsShared())](const FPRCombatEvent& Event)
			{
				if (TSharedPtr<FPIEPlayerSkillCheckpointDRunner> This = WeakThis.Pin())
				{
					This->CombatEvents.Add(Event);
				}
			});
		Phase = ECheckpointDSmokePhase::HookNoTargetStart;
		return true;
	}

	bool Tick()
	{
		if (!World.IsValid() || !Character.IsValid() || !ASC.IsValid() || !CombatSubsystem.IsValid()
			|| !GEditor || GEditor->PlayWorld != World.Get())
		{
			Fail(TEXT("PIE ended or ProjectR checkpoint-D objects became invalid during smoke."));
			return false;
		}
		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ECheckpointDSmokePhase::HookNoTargetStart:
			MoveFar(TargetA);
			MoveFar(TargetB);
			SetAttributes(*ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
			SendPressAndRelease(EKeys::H);
			Advance(ECheckpointDSmokePhase::HookNoTargetWait);
			break;

		case ECheckpointDSmokePhase::HookNoTargetWait:
			if (Elapsed < 0.10)
			{
				break;
			}
			bHookNoTarget = FMath::IsNearlyEqual(Attributes->GetEnergy(), 100.0f, 0.1f)
				&& !IsAbilityActive(VectorHookData->AbilityTag)
				&& ASC->GetGameplayTagCount(HookCooldownTag()) == 0;
			if (!bHookNoTarget)
			{
				return FailAndStop(TEXT("VectorHook NoTarget committed Energy, Cooldown, or an active execution."));
			}
			Advance(ECheckpointDSmokePhase::HookObstructedStart);
			break;

		case ECheckpointDSmokePhase::HookObstructedStart:
			PrepareTarget(TargetA, TestOrigin + AimDirection * 600.0f, EPRAbilityTargetMobility::Light);
			MoveFar(TargetB);
			if (!CreateWall(TestOrigin + AimDirection * 300.0f))
			{
				return false;
			}
			SendPressAndRelease(EKeys::H);
			Advance(ECheckpointDSmokePhase::HookObstructedWait);
			break;

		case ECheckpointDSmokePhase::HookObstructedWait:
			if (Elapsed < 0.10)
			{
				break;
			}
			bHookObstructed = FMath::IsNearlyEqual(Attributes->GetEnergy(), 100.0f, 0.1f)
				&& !IsAbilityActive(VectorHookData->AbilityTag)
				&& ASC->GetGameplayTagCount(HookCooldownTag()) == 0;
			DestroyWall();
			if (!bHookObstructed)
			{
				return FailAndStop(TEXT("VectorHook obstruction committed Energy, Cooldown, or an active execution."));
			}
			Advance(ECheckpointDSmokePhase::HookLightStart);
			break;

		case ECheckpointDSmokePhase::HookLightStart:
			ResetPlayerAtOrigin();
			// The transient target is a normal character target, not a second
			// locally possessed player. Release its fixture controller before the
			// controlled RootMotionSource moves it.
			if (TargetA.Controller.IsValid())
			{
				TargetA.Controller->UnPossess();
			}
			PrepareTarget(TargetA, TestOrigin + AimDirection * 600.0f, EPRAbilityTargetMobility::Light);
			MoveFar(TargetB);
			HookTargetStart = TargetA.Character->GetActorLocation();
			HookPlayerStart = Character->GetActorLocation();
			HookEventStart = CombatEvents.Num();
			// Enhanced Input samples a triggered action on the following game
			// tick. Keep the synthetic press across that boundary so the fixed
			// D-pad mapping is exercised rather than being released in the same
			// editor tick.
			SendKey(EKeys::Gamepad_DPad_Up, IE_Pressed);
			Advance(ECheckpointDSmokePhase::HookLightInputRelease);
			break;

		case ECheckpointDSmokePhase::HookLightInputRelease:
			if (Elapsed < 0.05)
			{
				break;
			}
			SendKey(EKeys::Gamepad_DPad_Up, IE_Released);
			Advance(ECheckpointDSmokePhase::HookLightWait);
			break;

		case ECheckpointDSmokePhase::HookLightWait:
			if (IsAbilityActive(VectorHookData->AbilityTag) || SkillComponent->GetActiveDisplacementRequestId().IsValid())
			{
				if (Elapsed < 1.0)
				{
					break;
				}
			}
			bHookLight = VerifyHookResult(true, HookTargetStart, HookPlayerStart);
			if (!bHookLight)
			{
				const FVector PlayerEnd = Character->GetActorLocation();
				const FVector TargetEnd = TargetA.Character.IsValid()
					? TargetA.Character->GetActorLocation() : FVector::ZeroVector;
				return FailAndStop(FString::Printf(
					TEXT("VectorHook Light mismatch. PlayerStart=%s TargetStart=%s PlayerEnd=%s TargetEnd=%s Energy=%.2f Cooldown=%d Outcomes=%d Active=%s Request=%s %s."),
					*HookPlayerStart.ToString(), *HookTargetStart.ToString(), *PlayerEnd.ToString(),
					*TargetEnd.ToString(), Attributes->GetEnergy(), ASC->GetGameplayTagCount(HookCooldownTag()), CountResponseEvents(HookEventStart, VectorHookData->AbilityTag,
						UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()),
					IsAbilityActive(VectorHookData->AbilityTag) ? TEXT("true") : TEXT("false"),
					SkillComponent->GetActiveDisplacementRequestId().IsValid() ? TEXT("true") : TEXT("false"),
					*DescribeHookPreflight()));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::HookCooldownAfterLight);
			break;

		case ECheckpointDSmokePhase::HookCooldownAfterLight:
			if (ASC->GetGameplayTagCount(HookCooldownTag()) != 0)
			{
				if (Elapsed < 8.0)
				{
					break;
				}
				return FailAndStop(TEXT("VectorHook cooldown did not expire before Heavy validation."));
			}
			SetAutomationTimeDilation(1.0f);
			Advance(ECheckpointDSmokePhase::HookHeavyStart);
			break;

		case ECheckpointDSmokePhase::HookHeavyStart:
			ResetPlayerAtOrigin();
			PrepareTarget(TargetA, TestOrigin + AimDirection * 600.0f, EPRAbilityTargetMobility::Heavy);
			HookTargetStart = TargetA.Character->GetActorLocation();
			HookPlayerStart = Character->GetActorLocation();
			HookEventStart = CombatEvents.Num();
			SendPressAndRelease(EKeys::H);
			Advance(ECheckpointDSmokePhase::HookHeavyWait);
			break;

		case ECheckpointDSmokePhase::HookHeavyWait:
			if (IsAbilityActive(VectorHookData->AbilityTag) || SkillComponent->GetActiveDisplacementRequestId().IsValid())
			{
				if (Elapsed < 1.0)
				{
					break;
				}
			}
			bHookHeavy = VerifyHookResult(false, HookTargetStart, HookPlayerStart);
			if (!bHookHeavy)
			{
				return FailAndStop(TEXT("VectorHook Heavy did not move only the player to the 120cm stop distance."));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::HookCooldownAfterHeavy);
			break;

		case ECheckpointDSmokePhase::HookCooldownAfterHeavy:
			if (ASC->GetGameplayTagCount(HookCooldownTag()) != 0)
			{
				if (Elapsed < 8.0)
				{
					break;
				}
				return FailAndStop(TEXT("VectorHook cooldown did not expire before Anchored validation."));
			}
			SetAutomationTimeDilation(1.0f);
			Advance(ECheckpointDSmokePhase::HookAnchoredStart);
			break;

		case ECheckpointDSmokePhase::HookAnchoredStart:
			ResetPlayerAtOrigin();
			PrepareTarget(TargetA, TestOrigin + AimDirection * 600.0f, EPRAbilityTargetMobility::Anchored);
			HookTargetStart = TargetA.Character->GetActorLocation();
			HookPlayerStart = Character->GetActorLocation();
			HookEventStart = CombatEvents.Num();
			SendPressAndRelease(EKeys::H);
			Advance(ECheckpointDSmokePhase::HookAnchoredWait);
			break;

		case ECheckpointDSmokePhase::HookAnchoredWait:
			if (IsAbilityActive(VectorHookData->AbilityTag) || SkillComponent->GetActiveDisplacementRequestId().IsValid())
			{
				if (Elapsed < 1.0)
				{
					break;
				}
			}
			bHookAnchored = VerifyHookResult(false, HookTargetStart, HookPlayerStart)
				&& TargetA.Character->GetActorLocation().Equals(HookTargetStart, 1.0f);
			if (!bHookAnchored)
			{
				return FailAndStop(TEXT("VectorHook Anchored moved its target or failed its player pull."));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::HookCooldownAfterAnchored);
			break;

		case ECheckpointDSmokePhase::HookCooldownAfterAnchored:
			if (ASC->GetGameplayTagCount(HookCooldownTag()) != 0)
			{
				if (Elapsed < 8.0)
				{
					break;
				}
				return FailAndStop(TEXT("VectorHook cooldown did not expire before CounterProofWall validation."));
			}
			SetAutomationTimeDilation(1.0f);
			SetAttributes(*ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
			ResetPlayerAtOrigin();
			PrepareTarget(TargetA, TestOrigin + AimDirection * 200.0f, EPRAbilityTargetMobility::Light);
			Advance(ECheckpointDSmokePhase::GuardKeyboardStart);
			break;

		case ECheckpointDSmokePhase::GuardKeyboardStart:
			GuardEventStart = CombatEvents.Num();
			SendKey(EKeys::Semicolon, IE_Pressed);
			if (bImmediateGuardPerfect)
			{
				if (!ValidateGuardOpeningBlock())
				{
					return false;
				}
				Advance(ECheckpointDSmokePhase::GuardKeyboardLate);
				break;
			}
			Advance(ECheckpointDSmokePhase::GuardKeyboardPerfect);
			break;

		case ECheckpointDSmokePhase::GuardKeyboardPerfect:
			if (Elapsed < 0.05)
			{
				break;
			}
			if (!ValidateGuardOpeningBlock())
			{
				return false;
			}
			Advance(ECheckpointDSmokePhase::GuardKeyboardLate);
			break;

		case ECheckpointDSmokePhase::GuardKeyboardLate:
		{
			if (Elapsed < 0.16)
			{
				break;
			}
			bGuardMultipleBlocks = ApplyGuardDamage(-AimDirection, EPRCombatRequestStatus::RejectedBlocked)
				&& LastDamageEventHas(UPRTagLibrary::GetCombatResponseGuardBlockedTag())
				&& !LastDamageEventHas(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
			const float HealthBeforeBack = Attributes->GetHealth();
			bGuardBackAndInvalid = ApplyGuardDamage(AimDirection, EPRCombatRequestStatus::Applied)
				&& Attributes->GetHealth() < HealthBeforeBack;
			SetAttributes(*ASC.Get(), 100.0f, 0.0f, 80.0f, 10.0f);
			FPRDamageRequest ZeroRequest = MakeGuardDamage(FVector::ZeroVector);
			FGameplayTagContainer ZeroTags;
			bGuardBackAndInvalid = bGuardBackAndInvalid
				&& Character->EvaluateIncomingDamage(ZeroRequest, ZeroTags) == EPRCombatMitigationResult::NotHandled
				&& ZeroTags.IsEmpty();
			if (!bGuardMultipleBlocks || !bGuardBackAndInvalid)
			{
				return FailAndStop(TEXT("CounterProofWall multiple, back, or invalid-direction mitigation was incorrect."));
			}
			Advance(ECheckpointDSmokePhase::GuardKeyboardRelease);
			break;
		}

		case ECheckpointDSmokePhase::GuardKeyboardRelease:
			SendKey(EKeys::Semicolon, IE_Released);
			if (Elapsed < 0.05)
			{
				break;
			}
			bGuardRelease = !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
				&& !IsAbilityActive(CounterProofWallData->AbilityTag)
				&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle;
			if (!bGuardRelease)
			{
				return FailAndStop(TEXT("CounterProofWall keyboard release left Guarding or an active phase behind."));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::GuardCooldownAfterKeyboard);
			break;

		case ECheckpointDSmokePhase::GuardCooldownAfterKeyboard:
			if (ASC->GetGameplayTagCount(GuardCooldownTag()) != 0)
			{
				if (Elapsed < 10.0)
				{
					break;
				}
				return FailAndStop(TEXT("CounterProofWall cooldown did not expire before D-Pad validation."));
			}
			SetAutomationTimeDilation(1.0f);
			Advance(ECheckpointDSmokePhase::GuardGamepadStart);
			break;

		case ECheckpointDSmokePhase::GuardGamepadStart:
			SendKey(EKeys::Gamepad_DPad_Down, IE_Pressed);
			Advance(ECheckpointDSmokePhase::GuardGamepadRelease);
			break;

		case ECheckpointDSmokePhase::GuardGamepadRelease:
			if (Elapsed < 0.05)
			{
				break;
			}
			bGuardGamepad = ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
				&& FMath::IsNearlyEqual(Attributes->GetEnergy(), 60.0f, 0.1f);
			SendKey(EKeys::Gamepad_DPad_Down, IE_Released);
			if (!bGuardGamepad)
			{
				return FailAndStop(TEXT("CounterProofWall D-Pad Down did not reach its unique formal Spec."));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::GuardCooldownAfterGamepad);
			break;

		case ECheckpointDSmokePhase::GuardCooldownAfterGamepad:
			if (ASC->GetGameplayTagCount(GuardCooldownTag()) != 0)
			{
				if (Elapsed < 10.0)
				{
					break;
				}
				return FailAndStop(TEXT("CounterProofWall cooldown did not expire before maximum-held validation."));
			}
			SetAutomationTimeDilation(1.0f);
			Advance(ECheckpointDSmokePhase::GuardMaximumStart);
			break;

		case ECheckpointDSmokePhase::GuardMaximumStart:
			SendKey(EKeys::Semicolon, IE_Pressed);
			Advance(ECheckpointDSmokePhase::GuardMaximumWait);
			break;

		case ECheckpointDSmokePhase::GuardMaximumWait:
			if (Elapsed < 1.05)
			{
				break;
			}
			SendKey(EKeys::Semicolon, IE_Released);
			bGuardMaximum = !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
				&& !IsAbilityActive(CounterProofWallData->AbilityTag)
				&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle;
			if (!bGuardMaximum)
			{
				return FailAndStop(TEXT("CounterProofWall exceeded its one-second held maximum."));
			}
			SetAutomationTimeDilation(10.0f);
			Advance(ECheckpointDSmokePhase::CleanupWait);
			break;

		case ECheckpointDSmokePhase::CleanupWait:
			if (ASC->GetGameplayTagCount(HookCooldownTag()) != 0
				|| ASC->GetGameplayTagCount(GuardCooldownTag()) != 0)
			{
				if (Elapsed < 10.0)
				{
					break;
				}
				return FailAndStop(TEXT("Checkpoint-D cooldowns did not expire before runtime leak validation."));
			}
			SetAutomationTimeDilation(1.0f);
			bBCRegression = CheckBCRegression();
			bRuntimeClean = CheckRuntimeClean();
			if (!bBCRegression || !bRuntimeClean)
			{
				return FailAndStop(TEXT("Checkpoint-D B/C regression or runtime cleanup validation failed."));
			}
			Advance(ECheckpointDSmokePhase::Complete);
			break;

		case ECheckpointDSmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	bool CheckFormalSpecContract() const
	{
		if (!ASC.IsValid() || ASC->GetActivatableAbilities().Num() != 6)
		{
			return false;
		}
		const TCHAR* ExpectedTags[] = {
			TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"), TEXT("Skill.ThunderDrop"),
			TEXT("Skill.AfterimageDodge"), TEXT("Skill.VectorHook"), TEXT("Skill.CounterProofWall")};
		const TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
		if (Specs.Num() != UE_ARRAY_COUNT(ExpectedTags))
		{
			return false;
		}
		for (int32 Index = 0; Index < Specs.Num(); ++Index)
		{
			const UPRGameplayAbility* Ability = Cast<UPRGameplayAbility>(Specs[Index].Ability);
			FPRAbilityRuntimeState State;
			if (!Ability || Ability->GetProjectRAbilityTag().ToString() != ExpectedTags[Index]
				|| !ASC->GetAbilityRuntimeStateByAbilityTag(Ability->GetProjectRAbilityTag(), State)
				|| State.InputTag != (Index == 4 ? VectorHookData->InputTag
					: Index == 5 ? CounterProofWallData->InputTag : State.InputTag))
			{
				return false;
			}
		}
		return true;
	}

	bool CheckFrozenDataContract() const
	{
		const UPRVectorHookSkillFragment* HookFragment =
			Cast<UPRVectorHookSkillFragment>(VectorHookData->SkillFragment);
		const UPRCounterProofWallSkillFragment* GuardFragment =
			Cast<UPRCounterProofWallSkillFragment>(CounterProofWallData->SkillFragment);
		return HookFragment && GuardFragment
			&& VectorHookData->InputTag == UPRTagLibrary::GetInputSkillVectorHookTag()
			&& VectorHookData->ActivationPolicy == EPRAbilityActivationPolicy::OnInputTriggered
			&& VectorHookData->TargetPolicy == EPRPlayerSkillTargetPolicy::SingleTarget
			&& FMath::IsNearlyEqual(VectorHookData->EnergyCost, 15.0f)
			&& FMath::IsNearlyEqual(VectorHookData->CooldownDuration, 4.0f)
			&& FMath::IsNearlyZero(VectorHookData->BaseDamage)
			&& FMath::IsNearlyZero(VectorHookData->AttackPowerScale)
			&& FMath::IsNearlyEqual(VectorHookData->ActiveDuration, 0.30f)
			&& FMath::IsNearlyEqual(VectorHookData->Range, 800.0f)
			&& FMath::IsNearlyEqual(VectorHookData->HalfAngleDegrees, 45.0f)
			&& FMath::IsNearlyEqual(HookFragment->StopDistance, 120.0f)
			&& CounterProofWallData->InputTag == UPRTagLibrary::GetInputSkillCounterProofWallTag()
			&& CounterProofWallData->ActivationPolicy == EPRAbilityActivationPolicy::WhileInputActive
			&& CounterProofWallData->TargetPolicy == EPRPlayerSkillTargetPolicy::Self
			&& FMath::IsNearlyEqual(CounterProofWallData->EnergyCost, 20.0f)
			&& FMath::IsNearlyEqual(CounterProofWallData->CooldownDuration, 6.0f)
			&& FMath::IsNearlyZero(CounterProofWallData->BaseDamage)
			&& FMath::IsNearlyEqual(CounterProofWallData->ActiveDuration, 1.0f)
			&& FMath::IsNearlyEqual(CounterProofWallData->HalfAngleDegrees, 60.0f)
			&& FMath::IsNearlyEqual(GuardFragment->PerfectTimingWindow, 0.15f);
	}

	bool CheckBCRegression() const
	{
		const TCHAR* FrozenTags[] = {
			TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"),
			TEXT("Skill.ThunderDrop"), TEXT("Skill.AfterimageDodge")};
		for (const TCHAR* TagName : FrozenTags)
		{
			FPRAbilityRuntimeState State;
			if (!ASC->GetAbilityRuntimeStateByAbilityTag(FGameplayTag::RequestGameplayTag(TagName), State)
				|| State.bActive)
			{
				return false;
			}
		}
		return !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
			&& !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag());
	}

	void SetAttributes(UPRAbilitySystemComponent& TargetASC, const float Health, const float Shield,
		const float Energy, const float AttackPower) const
	{
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), Health);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), Shield);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), Energy);
		TargetASC.SetNumericAttributeBase(UPRAttributeSet::GetAttackPowerAttribute(), AttackPower);
	}

	void ResetPlayerAtOrigin() const
	{
		Character->SetActorLocation(TestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	void PrepareTarget(FTransientCombatant& Target, const FVector Location,
		const EPRAbilityTargetMobility Mobility) const
	{
		if (!Target.Character.IsValid() || !Target.ASC.IsValid())
		{
			return;
		}
		Target.Character->ConfigureTarget(TEXT("PIE.Skill.CheckpointD.TargetA"), true, Mobility);
		Target.Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Target.Character->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		Target.Character->GetCharacterMovement()->StopMovementImmediately();
		SetAttributes(*Target.ASC.Get(), 100.0f, 0.0f, 100.0f, 10.0f);
	}

	void MoveFar(FTransientCombatant& Target) const
	{
		PrepareTarget(Target, Character->GetActorLocation() + AimDirection * 2000.0f,
			EPRAbilityTargetMobility::Light);
	}

	bool CreateWall(const FVector Location)
	{
		DestroyWall();
		AActor* NewWall = World->SpawnActor<AActor>();
		if (!NewWall)
		{
			Fail(TEXT("The fixed checkpoint-D PIE smoke could not spawn its transient WorldStatic wall."));
			return false;
		}
		UBoxComponent* Box = NewObject<UBoxComponent>(NewWall, TEXT("CheckpointDWall"));
		if (!Box)
		{
			NewWall->Destroy();
			Fail(TEXT("The fixed checkpoint-D PIE smoke could not create its transient wall collision."));
			return false;
		}
		NewWall->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10.0f, 200.0f, 200.0f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		NewWall->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Wall = NewWall;
		return true;
	}

	bool VerifyHookResult(const bool bTargetMoves, const FVector TargetStart,
		const FVector PlayerStart) const
	{
		if (!TargetA.Character.IsValid())
		{
			return false;
		}
		// ProjectR's 2.5D gameplay plane is X/Z; FVector::Size2D measures X/Y
		// and would silently ignore vertical-in-world hook travel.
		const auto PlanarMagnitude = [](const FVector& Vector)
		{
			return FVector(Vector.X, 0.0f, Vector.Z).Size();
		};
		const float Distance = PlanarMagnitude(
			Character->GetActorLocation() - TargetA.Character->GetActorLocation());
		const FVector PlayerDelta = Character->GetActorLocation() - PlayerStart;
		const FVector TargetDelta = TargetA.Character->GetActorLocation() - TargetStart;
		const int32 OutcomeCount = CountResponseEvents(HookEventStart,
			VectorHookData->AbilityTag, UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
		return FMath::IsNearlyEqual(Distance, 120.0f, 8.0f)
			&& FMath::Abs(PlayerDelta.Y) <= 1.0f && FMath::Abs(TargetDelta.Y) <= 1.0f
			&& (bTargetMoves ? PlanarMagnitude(PlayerDelta) <= 2.0f && PlanarMagnitude(TargetDelta) > 400.0f
				: PlanarMagnitude(PlayerDelta) > 400.0f && PlanarMagnitude(TargetDelta) <= 2.0f)
			&& OutcomeCount == 1 && !IsAbilityActive(VectorHookData->AbilityTag)
			&& !SkillComponent->GetActiveDisplacementRequestId().IsValid();
	}

	FString DescribeHookPreflight() const
	{
		UPRPlayerSkillSubsystem* Subsystem = World.IsValid()
			? World->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr;
		if (!Subsystem || !VectorHookData.IsValid())
		{
			return TEXT("Preflight=Unavailable");
		}
		FPRAbilityTargetQuery Query;
		Query.SourceActor = Character.Get();
		Query.AbilityTag = VectorHookData->AbilityTag;
		Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardArea;
		Query.Origin = Character->GetActorLocation();
		Query.AimDirection = AimDirection;
		Query.AreaCenter = Query.Origin;
		Query.Range = VectorHookData->Range;
		Query.HalfAngleDegrees = VectorHookData->HalfAngleDegrees;
		FPRAbilityTargetQueryResult Targets;
		FGameplayTag Failure;
		const bool bResolved = Subsystem->QueryTargets(Query, Targets, Failure);
		return FString::Printf(TEXT("Preflight=%s Count=%d Failure=%s"),
			bResolved ? TEXT("true") : TEXT("false"), Targets.Targets.Num(), *Failure.ToString());
	}

	FPRDamageRequest MakeGuardDamage(const FVector IncomingDirection) const
	{
		FPRDamageRequest Request;
		Request.SourceId = TEXT("PIE.Skill.CheckpointD.Attacker");
		Request.DamageSource = TargetA.PlayerState.Get();
		Request.Instigator = TargetA.Character.Get();
		Request.Target = Character.Get();
		Request.AbilityTag = CounterProofWallData->AbilityTag;
		Request.RawDamage = 10.0f;
		Request.ImpactOrigin = TargetA.Character.IsValid() ? TargetA.Character->GetActorLocation() : FVector::ZeroVector;
		Request.IncomingDirection = IncomingDirection;
		return Request;
	}

	bool ValidateGuardOpeningBlock()
	{
		bGuardPress = ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
			&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Active
			&& FMath::IsNearlyEqual(Attributes->GetEnergy(), 80.0f, 0.1f);
		if (!bGuardPress || !ApplyGuardDamage(-AimDirection, EPRCombatRequestStatus::RejectedBlocked))
		{
			return FailAndStop(TEXT("CounterProofWall keyboard press did not commit one Guarding block."));
		}
		bGuardPerfect = LastDamageEventHas(UPRTagLibrary::GetCombatResponseGuardBlockedTag())
			&& LastDamageEventHas(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
		if (!bGuardPerfect)
		{
			const FGameplayTagContainer LastTags = CombatEvents.IsEmpty()
				? FGameplayTagContainer() : CombatEvents.Last().ResponseTags;
			return FailAndStop(FString::Printf(
				TEXT("CounterProofWall opening front block mismatch: Events=%d LastEvent=%s ResponseTags=%s."),
				CombatEvents.Num(), CombatEvents.IsEmpty()
					? TEXT("None") : *CombatEvents.Last().EventTag.ToString(),
				*LastTags.ToStringSimple()));
		}
		return true;
	}

	bool ApplyGuardDamage(const FVector IncomingDirection, const EPRCombatRequestStatus Expected) const
	{
		return CombatSubsystem->ApplyDamage(MakeGuardDamage(IncomingDirection)) == Expected;
	}

	bool LastDamageEventHas(const FGameplayTag ResponseTag) const
	{
		return !CombatEvents.IsEmpty()
			&& CombatEvents.Last().EventTag == UPRTagLibrary::GetCombatEventDamageRejectedTag()
			&& CombatEvents.Last().ResponseTags.HasTagExact(ResponseTag);
	}

	int32 CountResponseEvents(const int32 FirstIndex, const FGameplayTag AbilityTag,
		const FGameplayTag ResponseTag) const
	{
		int32 Count = 0;
		for (int32 Index = FirstIndex; Index < CombatEvents.Num(); ++Index)
		{
			const FPRCombatEvent& Event = CombatEvents[Index];
			if (Event.AbilityTag == AbilityTag
				&& Event.EventTag == UPRTagLibrary::GetCombatEventAbilityOutcomeTag()
				&& Event.ResponseTags.HasTagExact(ResponseTag))
			{
				++Count;
			}
		}
		return Count;
	}

	bool IsAbilityActive(const FGameplayTag AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC.IsValid() && ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State) && State.bActive;
	}

	bool CheckRuntimeClean() const
	{
		return CheckFormalSpecContract() && !IsAbilityActive(VectorHookData->AbilityTag)
			&& !IsAbilityActive(CounterProofWallData->AbilityTag)
			&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle
			&& !SkillComponent->GetActiveDisplacementRequestId().IsValid()
			&& !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
			&& ASC->GetGameplayTagCount(HookCooldownTag()) == 0
			&& ASC->GetGameplayTagCount(GuardCooldownTag()) == 0
			&& ASC->GetActiveEffects(FGameplayEffectQuery()).IsEmpty()
			&& !Wall.IsValid();
	}

	static FGameplayTag HookCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.VectorHook"));
	}

	static FGameplayTag GuardCooldownTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill.CounterProofWall"));
	}

	void SendPressAndRelease(const FKey& Key)
	{
		SendKey(Key, IE_Pressed);
		SendKey(Key, IE_Released);
	}

	void SendKey(const FKey& Key, const EInputEvent Event)
	{
		if (Viewport.IsValid() && InputViewport && Viewport->GetGameViewport() == InputViewport)
		{
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(
				Key, Event, 1.0f, -1, InputDevice, false, InputViewport));
		}
		DispatchFixedSkillInput(Key, Event);
	}

	void DispatchFixedSkillInput(const FKey& Key, const EInputEvent Event)
	{
		FGameplayTag InputTag;
		if (Key == EKeys::H || Key == EKeys::Gamepad_DPad_Up)
		{
			InputTag = VectorHookData.IsValid() ? VectorHookData->InputTag : FGameplayTag();
		}
		else if (Key == EKeys::Semicolon || Key == EKeys::Gamepad_DPad_Down)
		{
			InputTag = CounterProofWallData.IsValid() ? CounterProofWallData->InputTag : FGameplayTag();
		}
		if (InputTag.IsValid() && ASC.IsValid())
		{
			if (Event == IE_Pressed)
			{
				ASC->AbilityInputTagPressed(InputTag);
			}
			else if (Event == IE_Released)
			{
				ASC->AbilityInputTagReleased(InputTag);
			}
		}
	}

	void ReleaseInputs()
	{
		SendKey(EKeys::H, IE_Released);
		SendKey(EKeys::Gamepad_DPad_Up, IE_Released);
		SendKey(EKeys::Semicolon, IE_Released);
		SendKey(EKeys::Gamepad_DPad_Down, IE_Released);
	}

	void SetAutomationTimeDilation(const float TimeDilation) const
	{
		if (World.IsValid() && IsValid(World->GetWorldSettings()))
		{
			World->GetWorldSettings()->SetTimeDilation(TimeDilation);
		}
	}

	bool Finish()
	{
		const bool bPassed = bHookNoTarget && bHookObstructed && bHookLight && bHookHeavy
			&& bHookAnchored && bGuardPress && bGuardPerfect && bGuardMultipleBlocks
			&& bGuardBackAndInvalid && bGuardRelease && bGuardGamepad && bGuardMaximum
			&& bBCRegression && bRuntimeClean && CheckFormalSpecContract();
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetNumberField(TEXT("formalSpecCount"), ASC.IsValid() ? ASC->GetActivatableAbilities().Num() : 0);
		Json->SetBoolField(TEXT("hookNoTargetPreCommit"), bHookNoTarget);
		Json->SetBoolField(TEXT("hookObstructedPreCommit"), bHookObstructed);
		Json->SetBoolField(TEXT("hookLight120cm"), bHookLight);
		Json->SetBoolField(TEXT("hookHeavy120cm"), bHookHeavy);
		Json->SetBoolField(TEXT("hookAnchored120cm"), bHookAnchored);
		Json->SetBoolField(TEXT("guardPress"), bGuardPress);
		Json->SetBoolField(TEXT("guardPerfect"), bGuardPerfect);
		Json->SetBoolField(TEXT("guardMultipleBlocks"), bGuardMultipleBlocks);
		Json->SetBoolField(TEXT("guardBackAndInvalid"), bGuardBackAndInvalid);
		Json->SetBoolField(TEXT("guardRelease"), bGuardRelease);
		Json->SetBoolField(TEXT("guardGamepad"), bGuardGamepad);
		Json->SetBoolField(TEXT("guardMaximum"), bGuardMaximum);
		Json->SetBoolField(TEXT("bcRegression"), bBCRegression);
		Json->SetBoolField(TEXT("runtimeClean"), bRuntimeClean);
		Json->SetNumberField(TEXT("combatEventCount"), CombatEvents.Num());
		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		Cleanup();
		if (bPassed)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	void Advance(const ECheckpointDSmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		return World.IsValid() ? World->GetTimeSeconds() : 0.0;
	}

	void DestroyWall()
	{
		if (Wall.IsValid())
		{
			Wall->Destroy();
			Wall.Reset();
		}
	}

	static void DestroyCombatant(FTransientCombatant& Combatant)
	{
		if (Combatant.ASC.IsValid())
		{
			Combatant.ASC->CancelAllAbilities();
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->UnPossess();
		}
		if (Combatant.Character.IsValid())
		{
			Combatant.Character->Destroy();
		}
		if (Combatant.Controller.IsValid())
		{
			Combatant.Controller->Destroy();
		}
		if (Combatant.PlayerState.IsValid())
		{
			Combatant.PlayerState->Destroy();
		}
		Combatant = FTransientCombatant();
	}

	void Cleanup()
	{
		ReleaseInputs();
		SetAutomationTimeDilation(InitialTimeDilation);
		DestroyWall();
		if (CombatSubsystem.IsValid() && CombatEventHandle.IsValid())
		{
			CombatSubsystem->OnCombatEvent().Remove(CombatEventHandle);
			CombatEventHandle.Reset();
		}
		DestroyCombatant(TargetB);
		DestroyCombatant(TargetA);
		if (ASC.IsValid())
		{
			ASC->CancelAllAbilities();
			FGameplayTagContainer CleanupTags;
			CleanupTags.AddTag(HookCooldownTag());
			CleanupTags.AddTag(GuardCooldownTag());
			CleanupTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
			ASC->RemoveActiveEffectsWithTags(CleanupTags);
			SetAttributes(*ASC.Get(), InitialHealth, InitialShield, InitialEnergy, InitialAttackPower);
		}
		if (Character.IsValid())
		{
			Character->SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Character->GetCharacterMovement()->SetMovementMode(InitialMovementMode);
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		InputViewport = nullptr;
	}

	bool FailAndStop(const FString& Message)
	{
		Fail(Message);
		return false;
	}

	void Fail(const FString& Message)
	{
		Cleanup();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	const UPRAttributeSet* Attributes = nullptr;
	TWeakObjectPtr<UPRPlayerSkillComponent> SkillComponent;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	FViewport* InputViewport = nullptr;
	FInputDeviceId InputDevice = INPUTDEVICEID_NONE;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> VectorHookData;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> CounterProofWallData;
	TWeakObjectPtr<AActor> Wall;
	FTransientCombatant TargetA;
	FTransientCombatant TargetB;
	FDelegateHandle CombatEventHandle;
	TArray<FPRCombatEvent> CombatEvents;
	ECheckpointDSmokePhase Phase = ECheckpointDSmokePhase::HookNoTargetStart;
	double PhaseStartTime = 0.0;
	FTransform InitialTransform = FTransform::Identity;
	EMovementMode InitialMovementMode = MOVE_Walking;
	FVector TestOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	FVector HookTargetStart = FVector::ZeroVector;
	FVector HookPlayerStart = FVector::ZeroVector;
	int32 HookEventStart = 0;
	int32 GuardEventStart = 0;
	float InitialHealth = 0.0f;
	float InitialShield = 0.0f;
	float InitialEnergy = 0.0f;
	float InitialAttackPower = 0.0f;
	float InitialTimeDilation = 1.0f;
	bool bHookNoTarget = false;
	bool bHookObstructed = false;
	bool bHookLight = false;
	bool bHookHeavy = false;
	bool bHookAnchored = false;
	bool bGuardPress = false;
	bool bGuardPerfect = false;
	bool bGuardMultipleBlocks = false;
	bool bGuardBackAndInvalid = false;
	bool bGuardRelease = false;
	bool bGuardGamepad = false;
	bool bGuardMaximum = false;
	bool bBCRegression = false;
	bool bImmediateGuardPerfect = false;
	bool bRuntimeClean = false;
};

enum class ECheckpointESmokePhase : uint8
{
	WaitForPreload,
	AfterimageStart,
	AfterimagePlaybackWait,
	AfterimageCleanupWait,
	Complete
};

/**
 * Narrow E-only smoke. The existing B/C/D runners remain the exhaustive gameplay
 * checks; this runner proves the shared data-driven presentation seam with formal
 * six-skill data and a real locally controlled Afterimage activation.
 */
class FPIEPlayerSkillCheckpointERunner
	: public TSharedFromThis<FPIEPlayerSkillCheckpointERunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIEPlayerSkillCheckpointERunner> Runner =
			MakeShared<FPIEPlayerSkillCheckpointERunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(
			NewObject<UToolCallAsyncResultString>());
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			return Fail(TEXT("RunPIEPlayerSkillCheckpointESmoke requires an active authoritative in-process PIE world."));
		}

		World = PlayWorld;
		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		ASC = PlayerState.IsValid() ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		SkillComponent = Character.IsValid() ? Character->GetPlayerSkillComponent() : nullptr;
		AfterimageData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, AfterimageDataPath);
		const TCHAR* DataPaths[] = {
			ShadowDataPath, FireDataPath, ThunderDataPath, AfterimageDataPath,
			VectorHookDataPath, CounterProofWallDataPath};
		for (const TCHAR* DataPath : DataPaths)
		{
			if (UPRPlayerSkillDataAsset* SkillData = LoadObject<UPRPlayerSkillDataAsset>(nullptr, DataPath))
			{
				SkillDataAssets.Add(SkillData);
			}
		}
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid() || !ASC.IsValid()
			|| !SkillComponent.IsValid() || !AfterimageData.IsValid() || SkillDataAssets.Num() != 6)
		{
			return Fail(TEXT("PIE is missing the formal player, six Skill DataAssets, ASC, or PlayerSkillComponent."));
		}
		if (!CheckFormalSpecContract() || !CheckPresentationReferences())
		{
			return Fail(TEXT("The formal v0.2.0-E AbilitySet or six presentation references are not exact."));
		}
		// Re-enter the ordinary PlayerState-owned Avatar lifecycle after the final
		// AbilitySet is known to be present.  This is the same fixed re-possession
		// path exercised by the foundation regressions and makes the smoke verify
		// the real component preload table rather than loading presentation assets
		// directly from this editor-only tool.
		Controller->UnPossess();
		Controller->Possess(Character.Get());
		if (Controller->GetPawn() != Character.Get())
		{
			return Fail(TEXT("Checkpoint-E could not restore the fixed formal player after its controlled Avatar re-possession."));
		}
		PhaseStartTime = GetPlayWorldTimeSeconds();
		return true;
	}

	bool Tick(float)
	{
		if (!World.IsValid() || !Character.IsValid() || !ASC.IsValid())
		{
			return FailAndStop(TEXT("The PIE world or formal player disappeared during checkpoint-E presentation smoke."));
		}

		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ECheckpointESmokePhase::WaitForPreload:
			if (ArePresentationAssetsLoaded())
			{
				bPreloadReady = true;
				Advance(ECheckpointESmokePhase::AfterimageStart);
			}
			// The PlayerSkillComponent owns the asynchronous request.  This smoke only
			// observes that request completing; a slow local DDC must not turn the
			// fixed presentation contract into a false negative.
			else if (Elapsed > 20.0)
			{
				return FailAndStop(TEXT("Checkpoint-E did not asynchronously preload all six VFX/SFX soft references."));
			}
			break;

		case ECheckpointESmokePhase::AfterimageStart:
			InitialNiagaraCount = CountAttachedNiagaraComponents();
			InitialAudioCount = CountAttachedAudioComponents();
			ASC->AbilityInputTagPressed(AfterimageData->InputTag);
			Advance(ECheckpointESmokePhase::AfterimagePlaybackWait);
			break;

		case ECheckpointESmokePhase::AfterimagePlaybackWait:
			if (Elapsed >= 0.03)
			{
				const int32 CurrentNiagaraCount = CountAttachedNiagaraComponents();
				const int32 CurrentAudioCount = CountAttachedAudioComponents();
				bLocalPlayback = CurrentNiagaraCount > InitialNiagaraCount
					&& CurrentAudioCount > InitialAudioCount;
				if (!bLocalPlayback)
				{
					FPRAbilityRuntimeState AbilityState;
					const bool bHasAbilityState = ASC->GetAbilityRuntimeStateByAbilityTag(
						AfterimageData->AbilityTag, AbilityState);
					return FailAndStop(FString::Printf(
						TEXT("Checkpoint-E did not create local Niagara and audio components after a committed formal skill. "
							"LocallyControlled=%s AbilityState=%s Active=%s Phase=%d Niagara=%d/%d Audio=%d/%d."),
						Character->IsLocallyControlled() ? TEXT("true") : TEXT("false"),
						bHasAbilityState ? TEXT("true") : TEXT("false"),
						bHasAbilityState && AbilityState.bActive ? TEXT("true") : TEXT("false"),
						static_cast<int32>(SkillComponent->GetCurrentPhase()),
						CurrentNiagaraCount, InitialNiagaraCount, CurrentAudioCount, InitialAudioCount));
				}
				// AfterimageDodge is held-input.  Release only after observing the
				// presentation created by the committed ability; releasing in the same
				// frame as Press cancels the ability before this assertion can run.
				ASC->AbilityInputTagReleased(AfterimageData->InputTag);
				Advance(ECheckpointESmokePhase::AfterimageCleanupWait);
			}
			break;

		case ECheckpointESmokePhase::AfterimageCleanupWait:
			bCleanup = CountAttachedNiagaraComponents() == InitialNiagaraCount
				&& CountAttachedAudioComponents() == InitialAudioCount
				&& !IsAbilityActive(AfterimageData->AbilityTag)
				&& SkillComponent->GetCurrentPhase() == EPRPlayerSkillPhase::Idle;
			if (bCleanup)
			{
				Advance(ECheckpointESmokePhase::Complete);
			}
			else if (Elapsed > 3.0)
			{
				return FailAndStop(FString::Printf(
					TEXT("Checkpoint-E did not clean local presentation and Afterimage runtime state within 3 seconds. "
						"Active=%s Phase=%d Niagara=%d/%d Audio=%d/%d."),
					IsAbilityActive(AfterimageData->AbilityTag) ? TEXT("true") : TEXT("false"),
					static_cast<int32>(SkillComponent->GetCurrentPhase()),
					CountAttachedNiagaraComponents(), InitialNiagaraCount,
					CountAttachedAudioComponents(), InitialAudioCount));
			}
			break;

		case ECheckpointESmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	bool CheckFormalSpecContract() const
	{
		if (!ASC.IsValid() || ASC->GetActivatableAbilities().Num() != 6)
		{
			return false;
		}
		const TCHAR* ExpectedTags[] = {
			TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"), TEXT("Skill.ThunderDrop"),
			TEXT("Skill.AfterimageDodge"), TEXT("Skill.VectorHook"), TEXT("Skill.CounterProofWall")};
		const TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
		for (int32 Index = 0; Index < Specs.Num(); ++Index)
		{
			const UPRGameplayAbility* Ability = Cast<UPRGameplayAbility>(Specs[Index].Ability);
			if (!Ability || Ability->GetProjectRAbilityTag().ToString() != ExpectedTags[Index])
			{
				return false;
			}
		}
		return true;
	}

	bool CheckPresentationReferences() const
	{
		if (SkillDataAssets.Num() != 6)
		{
			return false;
		}
		for (const UPRPlayerSkillDataAsset* SkillData : SkillDataAssets)
		{
			if (!IsValid(SkillData) || SkillData->VFX.IsNull() || SkillData->SFX.IsNull())
			{
				return false;
			}
		}
		return true;
	}

	bool ArePresentationAssetsLoaded() const
	{
		for (const UPRPlayerSkillDataAsset* SkillData : SkillDataAssets)
		{
			if (!IsValid(SkillData) || !IsValid(SkillData->VFX.ToSoftObjectPath().ResolveObject())
				|| !IsValid(SkillData->SFX.ToSoftObjectPath().ResolveObject()))
			{
				return false;
			}
		}
		return true;
	}

	int32 CountAttachedNiagaraComponents() const
	{
		TInlineComponentArray<UActorComponent*> Components(Character.Get());
		return Components.FilterByPredicate([](const UActorComponent* Component)
		{
			return IsValid(Component) && Component->GetClass()->GetFName() == TEXT("NiagaraComponent");
		}).Num();
	}

	int32 CountAttachedAudioComponents() const
	{
		TInlineComponentArray<UAudioComponent*> Components(Character.Get());
		return Components.Num();
	}

	bool IsAbilityActive(const FGameplayTag AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC.IsValid() && ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State) && State.bActive;
	}

	void Advance(const ECheckpointESmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		return World.IsValid() ? World->GetTimeSeconds() : 0.0;
	}

	bool Finish()
	{
		const bool bPassed = bPreloadReady && bLocalPlayback && bCleanup && CheckFormalSpecContract();
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetNumberField(TEXT("formalSpecCount"), ASC.IsValid() ? ASC->GetActivatableAbilities().Num() : 0);
		Json->SetBoolField(TEXT("sixPresentationReferences"), CheckPresentationReferences());
		Json->SetBoolField(TEXT("preloadReady"), bPreloadReady);
		Json->SetBoolField(TEXT("localPlayback"), bLocalPlayback);
		Json->SetBoolField(TEXT("runtimeClean"), bCleanup);
		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		if (bPassed)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	bool FailAndStop(const FString& Message)
	{
		if (ASC.IsValid())
		{
			ASC->CancelAllAbilities();
		}
		return Fail(Message);
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	TWeakObjectPtr<UPRPlayerSkillComponent> SkillComponent;
	TWeakObjectPtr<UPRPlayerSkillDataAsset> AfterimageData;
	TArray<TObjectPtr<UPRPlayerSkillDataAsset>> SkillDataAssets;
	ECheckpointESmokePhase Phase = ECheckpointESmokePhase::WaitForPreload;
	double PhaseStartTime = 0.0;
	int32 InitialNiagaraCount = 0;
	int32 InitialAudioCount = 0;
	bool bPreloadReady = false;
	bool bLocalPlayback = false;
	bool bCleanup = false;
};
} // namespace PRPlayerSkillAutomationToolset

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointBSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointBRunner::Start();
}

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointCSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointCRunner::Start();
}

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointECRegressionSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointCRunner::Start(true);
}

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointDSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointDRunner::Start();
}

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointEDRegressionSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointDRunner::Start(true);
}

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointESmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointERunner::Start();
}
