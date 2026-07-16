// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillComponent.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/AssetManager.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Combat/PRCombatTypes.h"
#include "Core/Logging/PRLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Core/PRTagLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UPRPlayerSkillComponent::UPRPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPRPlayerSkillComponent::BeginPhase(
	const FGameplayTag AbilityTag,
	const EPRPlayerSkillPhase Phase,
	const float Duration)
{
	if (!AbilityTag.IsValid() || !AbilityTag.ToString().StartsWith(TEXT("Skill."))
		|| Phase == EPRPlayerSkillPhase::Idle
		|| !FMath::IsFinite(Duration)
		|| Duration < 0.0f
		|| (CurrentAbilityTag.IsValid() && CurrentAbilityTag != AbilityTag))
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CurrentAbilityTag = AbilityTag;
	CurrentPhase = Phase;
	if (Duration > 0.0f)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			ClearExecution();
			return false;
		}
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&UPRPlayerSkillComponent::HandlePhaseExpired,
			Duration,
			false);
	}
	return true;
}

void UPRPlayerSkillComponent::CancelExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CancelOwnedDisplacement();
	if (CurrentAbilityTag.IsValid())
	{
		CurrentPhase = EPRPlayerSkillPhase::Cancelled;
	}
}

void UPRPlayerSkillComponent::ClearExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CancelOwnedDisplacement();
	ClearCounterProofGuard(CurrentAbilityTag);
	CurrentAbilityTag = FGameplayTag();
	CurrentPhase = EPRPlayerSkillPhase::Idle;
}

FGameplayTag UPRPlayerSkillComponent::GetCurrentAbilityTag() const
{
	return CurrentAbilityTag;
}

EPRPlayerSkillPhase UPRPlayerSkillComponent::GetCurrentPhase() const
{
	return CurrentPhase;
}

FGuid UPRPlayerSkillComponent::GetActiveDisplacementRequestId() const
{
	return ActiveDisplacementRequestId;
}

FPRPlayerSkillPhaseExpiredNative& UPRPlayerSkillComponent::OnPhaseExpired()
{
	return PhaseExpiredEvent;
}

bool UPRPlayerSkillComponent::RequestOwnedDisplacement(
	const FPRAbilityDisplacementRequest& Request,
	FGameplayTag& OutFailureTag)
{
	OutFailureTag = FGameplayTag();
	if (ActiveDisplacementRequestId.IsValid() || !GetWorld())
	{
		return false;
	}
	UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>();
	if (!Subsystem)
	{
		return false;
	}
	EnsureSubsystemBinding();
	return Subsystem->RequestDisplacement(Request, ActiveDisplacementRequestId, OutFailureTag);
}

void UPRPlayerSkillComponent::CancelOwnedDisplacement()
{
	if (!ActiveDisplacementRequestId.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UPRPlayerSkillSubsystem* Subsystem = World->GetSubsystem<UPRPlayerSkillSubsystem>())
		{
			Subsystem->CancelDisplacement(ActiveDisplacementRequestId);
		}
	}
	ActiveDisplacementRequestId.Invalidate();
}

EPRCombatMitigationResult UPRPlayerSkillComponent::EvaluateIncomingDamage(
	const FPRDamageRequest& Request,
	FGameplayTagContainer& OutResponseTags) const
{
	if (!CounterProofGuard.bActive
		|| CurrentAbilityTag != CounterProofGuard.AbilityTag
		|| CurrentPhase != EPRPlayerSkillPhase::Active
		|| !FMath::IsFinite(Request.IncomingDirection.X)
		|| !FMath::IsFinite(Request.IncomingDirection.Y)
		|| !FMath::IsFinite(Request.IncomingDirection.Z))
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* ASC = AbilityInterface
		? AbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Character || !ASC || ASC->GetAvatarActor() != GetOwner()
		|| !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()) || !Mesh)
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	FVector Facing = Mesh->GetRightVector();
	Facing.Y = 0.0f;
	FVector AttackDirection = -Request.IncomingDirection;
	AttackDirection.Y = 0.0f;
	if (!Facing.Normalize() || !AttackDirection.Normalize())
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	const float MinimumFrontDot = FMath::Cos(FMath::DegreesToRadians(CounterProofGuard.HalfAngleDegrees));
	if (FVector::DotProduct(Facing, AttackDirection) + UE_KINDA_SMALL_NUMBER < MinimumFrontDot)
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	OutResponseTags.AddTag(UPRTagLibrary::GetCombatResponseGuardBlockedTag());
	if (const UWorld* World = GetWorld(); World
		&& World->GetTimeSeconds() <= CounterProofGuard.PerfectTimingEndTime + UE_KINDA_SMALL_NUMBER)
	{
		OutResponseTags.AddTag(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	}
	return EPRCombatMitigationResult::Blocked;
}

void UPRPlayerSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearExecution();
	ClearSkillPresentation();
	if (DisplacementFinishedHandle.IsValid() && GetWorld())
	{
		if (UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>())
		{
			Subsystem->OnDisplacementFinished().Remove(DisplacementFinishedHandle);
		}
	}
	DisplacementFinishedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRPlayerSkillComponent::PreloadSkillPresentation(UPRAbilitySystemComponent* AbilitySystemComponent)
{
	ClearSkillPresentation();
	if (!IsValid(AbilitySystemComponent) || AbilitySystemComponent->GetAvatarActor() != GetOwner())
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		UPRPlayerSkillDataAsset* SkillData = Cast<UPRPlayerSkillDataAsset>(Spec.SourceObject.Get());
		if (!IsValid(SkillData) || !SkillData->AbilityTag.IsValid()
			|| !SkillData->AbilityTag.ToString().StartsWith(TEXT("Skill.")))
		{
			continue;
		}
		PresentationDataByTag.Add(SkillData->AbilityTag, SkillData);
		if (!SkillData->VFX.IsNull())
		{
			Paths.AddUnique(SkillData->VFX.ToSoftObjectPath());
		}
		if (!SkillData->SFX.IsNull())
		{
			Paths.AddUnique(SkillData->SFX.ToSoftObjectPath());
		}
	}

	if (Paths.IsEmpty())
	{
		return;
	}

	const uint32 LoadGeneration = PresentationLoadGeneration;
	PresentationLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateWeakLambda(
			this,
			[this, LoadGeneration]()
			{
				HandlePresentationPreloadCompleted(LoadGeneration);
			}));
	if (!PresentationLoadHandle.IsValid())
	{
		WarnPresentationOnce(TEXT("PresentationPreloadRequest"),
			TEXT("Player-skill presentation preload request could not be created; gameplay remains available."));
	}
}

void UPRPlayerSkillComponent::PlaySkillPresentation(const UPRPlayerSkillDataAsset* SkillData)
{
	if (!IsValid(SkillData) || !SkillData->AbilityTag.IsValid() || !ShouldPlayLocalPresentation())
	{
		return;
	}

	const FGameplayTag AbilityTag = SkillData->AbilityTag;
	StopSkillPresentation(AbilityTag);
	USceneComponent* RootComponent = GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
	if (!RootComponent)
	{
		WarnPresentationOnce(
			*FString::Printf(TEXT("PresentationRoot.%s"), *AbilityTag.ToString()),
			FString::Printf(TEXT("Player-skill presentation skipped for %s because its Avatar has no root component."),
				*AbilityTag.ToString()));
		return;
	}

	if (const TObjectPtr<UNiagaraSystem>* VFX = LoadedPresentationVFX.Find(AbilityTag);
		VFX && IsValid(VFX->Get()))
	{
		if (UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAttached(
			VFX->Get(),
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			true,
			ENCPoolMethod::None,
			true))
		{
			ActivePresentationVFX.Add(AbilityTag, Component);
		}
	}
	else
	{
		WarnPresentationOnce(
			*FString::Printf(TEXT("PresentationVFX.%s"), *AbilityTag.ToString()),
			FString::Printf(TEXT("Player-skill VFX for %s is unavailable; gameplay continues without it."),
				*AbilityTag.ToString()));
	}

	if (const TObjectPtr<USoundBase>* SFX = LoadedPresentationSFX.Find(AbilityTag);
		SFX && IsValid(SFX->Get()))
	{
		if (UAudioComponent* Component = UGameplayStatics::SpawnSoundAttached(
			SFX->Get(),
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			true,
			0.35f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			true))
		{
			ActivePresentationSFX.Add(AbilityTag, Component);
		}
	}
	else
	{
		WarnPresentationOnce(
			*FString::Printf(TEXT("PresentationSFX.%s"), *AbilityTag.ToString()),
			FString::Printf(TEXT("Player-skill SFX for %s is unavailable; gameplay continues without it."),
				*AbilityTag.ToString()));
	}
}

void UPRPlayerSkillComponent::StopSkillPresentation(const FGameplayTag AbilityTag)
{
	if (TWeakObjectPtr<UNiagaraComponent>* VFX = ActivePresentationVFX.Find(AbilityTag))
	{
		if (UNiagaraComponent* Component = VFX->Get())
		{
			Component->DeactivateImmediate();
			if (IsValid(Component))
			{
				Component->DestroyComponent();
			}
		}
		ActivePresentationVFX.Remove(AbilityTag);
	}
	if (TWeakObjectPtr<UAudioComponent>* SFX = ActivePresentationSFX.Find(AbilityTag))
	{
		if (UAudioComponent* Component = SFX->Get())
		{
			Component->Stop();
			if (IsValid(Component))
			{
				Component->DestroyComponent();
			}
		}
		ActivePresentationSFX.Remove(AbilityTag);
	}
}

void UPRPlayerSkillComponent::ClearSkillPresentation()
{
	++PresentationLoadGeneration;
	if (PresentationLoadHandle.IsValid())
	{
		PresentationLoadHandle->CancelHandle();
		PresentationLoadHandle.Reset();
	}
	TArray<FGameplayTag> ActiveTags;
	ActivePresentationVFX.GetKeys(ActiveTags);
	for (const FGameplayTag& AbilityTag : ActiveTags)
	{
		StopSkillPresentation(AbilityTag);
	}
	ActiveTags.Reset();
	ActivePresentationSFX.GetKeys(ActiveTags);
	for (const FGameplayTag& AbilityTag : ActiveTags)
	{
		StopSkillPresentation(AbilityTag);
	}
	PresentationDataByTag.Reset();
	LoadedPresentationVFX.Reset();
	LoadedPresentationSFX.Reset();
	PresentationWarningKeys.Reset();
}

void UPRPlayerSkillComponent::HandlePresentationPreloadCompleted(const uint32 LoadGeneration)
{
	if (LoadGeneration != PresentationLoadGeneration || !GetOwner())
	{
		return;
	}
	PresentationLoadHandle.Reset();
	for (const TPair<FGameplayTag, TObjectPtr<UPRPlayerSkillDataAsset>>& Entry : PresentationDataByTag)
	{
		UPRPlayerSkillDataAsset* SkillData = Entry.Value;
		if (!IsValid(SkillData))
		{
			continue;
		}
		if (UNiagaraSystem* VFX = SkillData->VFX.Get())
		{
			LoadedPresentationVFX.Add(Entry.Key, VFX);
		}
		else
		{
			WarnPresentationOnce(
				*FString::Printf(TEXT("PresentationLoadVFX.%s"), *Entry.Key.ToString()),
				FString::Printf(TEXT("Player-skill VFX preload failed for %s; gameplay remains available."),
					*Entry.Key.ToString()));
		}
		if (USoundBase* SFX = SkillData->SFX.Get())
		{
			LoadedPresentationSFX.Add(Entry.Key, SFX);
		}
		else
		{
			WarnPresentationOnce(
				*FString::Printf(TEXT("PresentationLoadSFX.%s"), *Entry.Key.ToString()),
				FString::Printf(TEXT("Player-skill SFX preload failed for %s; gameplay remains available."),
					*Entry.Key.ToString()));
		}
	}
}

void UPRPlayerSkillComponent::WarnPresentationOnce(const FName WarningKey, const FString& Message)
{
	if (!PresentationWarningKeys.Contains(WarningKey))
	{
		PresentationWarningKeys.Add(WarningKey);
		UE_LOG(LogProjectRAbility, Warning, TEXT("%s"), *Message);
	}
}

bool UPRPlayerSkillComponent::ShouldPlayLocalPresentation() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UWorld* World = GetWorld();
	return Pawn && Pawn->IsLocallyControlled() && World && World->GetNetMode() != NM_DedicatedServer;
}

void UPRPlayerSkillComponent::HandlePhaseExpired()
{
	const FGameplayTag ExpiredAbilityTag = CurrentAbilityTag;
	const EPRPlayerSkillPhase ExpiredPhase = CurrentPhase;
	PhaseTimerHandle.Invalidate();
	PhaseExpiredEvent.Broadcast(ExpiredAbilityTag, ExpiredPhase);
}

void UPRPlayerSkillComponent::HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result)
{
	if (Result.RequestId == ActiveDisplacementRequestId)
	{
		ActiveDisplacementRequestId.Invalidate();
	}
}

void UPRPlayerSkillComponent::EnsureSubsystemBinding()
{
	if (DisplacementFinishedHandle.IsValid() || !GetWorld())
	{
		return;
	}
	if (UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>())
	{
		DisplacementFinishedHandle = Subsystem->OnDisplacementFinished().AddUObject(
			this,
			&UPRPlayerSkillComponent::HandleDisplacementFinished);
	}
}

bool UPRPlayerSkillComponent::BeginCounterProofGuard(
	const FGameplayTag AbilityTag,
	const float HalfAngleDegrees,
	const double PerfectTimingEndTime)
{
	if (!AbilityTag.IsValid() || CurrentAbilityTag != AbilityTag
		|| CurrentPhase != EPRPlayerSkillPhase::Active
		|| !FMath::IsFinite(HalfAngleDegrees) || HalfAngleDegrees < 0.0f || HalfAngleDegrees > 180.0f
		|| !FMath::IsFinite(PerfectTimingEndTime))
	{
		return false;
	}
	CounterProofGuard.AbilityTag = AbilityTag;
	CounterProofGuard.HalfAngleDegrees = HalfAngleDegrees;
	CounterProofGuard.PerfectTimingEndTime = PerfectTimingEndTime;
	CounterProofGuard.bActive = true;
	return true;
}

void UPRPlayerSkillComponent::ClearCounterProofGuard(const FGameplayTag AbilityTag)
{
	if (!CounterProofGuard.bActive || CounterProofGuard.AbilityTag != AbilityTag)
	{
		return;
	}
	CounterProofGuard = FCounterProofGuardContext();
}
