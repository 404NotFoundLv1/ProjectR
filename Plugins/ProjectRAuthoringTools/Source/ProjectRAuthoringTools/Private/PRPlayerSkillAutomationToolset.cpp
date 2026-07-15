// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRPlayerSkillAutomationToolset.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Components/BoxComponent.h"
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
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputDeviceLibrary.h"
#include "GameplayEffect.h"
#include "InputCoreTypes.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRPlayerSkillAutomationToolset
{
constexpr TCHAR ShadowDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ShadowThrust.DA_Skill_ShadowThrust");
constexpr TCHAR FireDataPath[] =
	TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_FireSlash.DA_Skill_FireSlash");

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
		if (ASC->GetActivatableAbilities().Num() != 2)
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
} // namespace PRPlayerSkillAutomationToolset

UToolCallAsyncResultString* UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointBSmoke()
{
	return PRPlayerSkillAutomationToolset::FPIEPlayerSkillCheckpointBRunner::Start();
}
