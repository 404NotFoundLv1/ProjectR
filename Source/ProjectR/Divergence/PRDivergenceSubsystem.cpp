// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceSubsystem.h"

#include "Characters/PRPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Divergence/PRDivergenceComponent.h"
#include "Divergence/PRDivergenceDataAsset.h"
#include "Dialogue/PRCompanionDialogueSubsystem.h"
#include "Dialogue/PRDialogueTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Save/PRSaveSubsystem.h"
#include "UI/PRDivergenceCacheWidget.h"

namespace PRDivergence
{
constexpr float InputReconcileSeconds = 0.25f;
const FName SourceId(TEXT("Divergence.Cache"));
}

void UPRDivergenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadDefinition();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		CompanionSubsystem = GameInstance->GetSubsystem<UPRCompanionSubsystem>();
		SaveSubsystem = GameInstance->GetSubsystem<UPRSaveSubsystem>();
		if (UPRCompanionSubsystem* Companions = CompanionSubsystem.Get())
		{
			PrimarySyncHandle = Companions->OnPrimarySyncChanged().AddUObject(this, &UPRDivergenceSubsystem::HandlePrimarySyncChanged);
		}
		if (UPRSaveSubsystem* Save = SaveSubsystem.Get())
		{
			SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRDivergenceSubsystem::HandleSaveOperation);
		}
		BindWorld(GameInstance->GetWorld());
	}
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRDivergenceSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRDivergenceSubsystem::HandleWorldCleanup);
}

void UPRDivergenceSubsystem::Deinitialize()
{
	if (PostWorldInitializationHandle.IsValid()) FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
	if (WorldCleanupHandle.IsValid()) FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	UnbindWorld(BoundWorld.Get());
	if (UPRCompanionSubsystem* Companions = CompanionSubsystem.Get(); Companions && PrimarySyncHandle.IsValid())
	{
		Companions->OnPrimarySyncChanged().Remove(PrimarySyncHandle);
	}
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get(); Save && SaveOperationHandle.IsValid())
	{
		Save->OnSaveOperation().Remove(SaveOperationHandle);
	}
	PrimarySyncHandle.Reset();
	CompanionSubsystem.Reset();
	SaveSubsystem.Reset();
	DefinitionAsset.Reset();
	Super::Deinitialize();
}

bool UPRDivergenceSubsystem::IsDefinitionReady() const { return bDefinitionReady; }
FPRDivergenceRuntimeState UPRDivergenceSubsystem::GetRuntimeState() const { return RuntimeState; }
FPRDivergenceResult UPRDivergenceSubsystem::GetLastResult() const { return LastResult; }
FPRDivergenceStateChangedNative& UPRDivergenceSubsystem::OnDivergenceStateChanged() { return StateChanged; }
FPRDivergenceResultNative& UPRDivergenceSubsystem::OnDivergenceResult() { return ResultPublished; }

#if WITH_DEV_AUTOMATION_TESTS
void UPRDivergenceSubsystem::ConfigureAutomationProfile(const FGameplayTag CompanionId, const FPRRelationshipState& Relationship)
{
	bUseAutomationProfile = CompanionId.IsValid();
	AutomationRelationship.CompanionId = CompanionId;
	AutomationRelationship.State = Relationship;
}

void UPRDivergenceSubsystem::ResetAutomationProfile()
{
	bUseAutomationProfile = false;
	AutomationRelationship = FPRCompanionRelationshipRecord();
}

void UPRDivergenceSubsystem::ResetAutomationRun()
{
	bRunProtectionConsumed = false;
	ClearActive(false);
}
#endif

void UPRDivergenceSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	if (World && World->GetGameInstance() == GetGameInstance() && World->IsGameWorld())
	{
		BindWorld(World);
	}
}

void UPRDivergenceSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World && World == BoundWorld.Get())
	{
		if (RuntimeState.State == EPRDivergenceState::AwaitingChoice)
		{
			FPRDivergenceResult Result;
			Result.ResultId = FGuid::NewGuid();
			Result.RequestId = RuntimeState.RequestId;
			Result.DeathEventId = RuntimeState.DeathEventId;
			Result.CompanionId = RuntimeState.CompanionId;
			Result.Resolution = EPRDivergenceResolution::Cancelled;
			Result.WorldTimeSeconds = GetWorldTimeSeconds();
			PublishResult(Result);
			ClearActive(true);
		}
		UnbindWorld(World);
	}
}

void UPRDivergenceSubsystem::BindWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	// A GameInstance subsystem can be initialized before this world's
	// subsystems exist.  Treat a same-world binding with no Combat subsystem as
	// incomplete, so PostWorldInitialization repairs it rather than silently
	// missing the formal death event stream.
	if (BoundWorld.Get() == World && CombatSubsystem.IsValid())
	{
		return;
	}
	UnbindWorld(BoundWorld.Get());
	BoundWorld = World;
	CombatSubsystem = World->GetSubsystem<UPRCombatSubsystem>();
	DialogueSubsystem = World->GetSubsystem<UPRCompanionDialogueSubsystem>();
	if (UPRCombatSubsystem* Combat = CombatSubsystem.Get())
	{
		CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRDivergenceSubsystem::HandleCombatEvent);
	}
	if (UPRCompanionDialogueSubsystem* Dialogue = DialogueSubsystem.Get())
	{
		DialogueResultHandle = Dialogue->OnDialogueResult().AddUObject(this, &UPRDivergenceSubsystem::HandleDialogueResult);
	}
	World->GetTimerManager().SetTimer(InputReconcileTimer, this, &UPRDivergenceSubsystem::ReconcileInputBridge, PRDivergence::InputReconcileSeconds, true);
	ReconcileInputBridge();
}

void UPRDivergenceSubsystem::UnbindWorld(UWorld* World)
{
	if (UPRCombatSubsystem* Combat = CombatSubsystem.Get(); Combat && CombatEventHandle.IsValid())
	{
		Combat->OnCombatEvent().Remove(CombatEventHandle);
	}
	CombatEventHandle.Reset();
	if (UPRCompanionDialogueSubsystem* Dialogue = DialogueSubsystem.Get(); Dialogue && DialogueResultHandle.IsValid())
	{
		Dialogue->OnDialogueResult().Remove(DialogueResultHandle);
	}
	DialogueResultHandle.Reset();
	if (World)
	{
		World->GetTimerManager().ClearTimer(InputReconcileTimer);
		World->GetTimerManager().ClearTimer(ExpiryTimer);
	}
	InputBridge.Reset();
	BoundPlayerPawn.Reset();
	CombatSubsystem.Reset();
	DialogueSubsystem.Reset();
	BoundWorld.Reset();
}

void UPRDivergenceSubsystem::ReconcileInputBridge()
{
	UWorld* World = BoundWorld.Get();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APRPlayerCharacter* Pawn = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
	if (!Controller || !Pawn)
	{
		return;
	}
	// A pending divergence belongs to one concrete player avatar.  A replacement
	// must never inherit its input bridge or leave a stale prompt alive.
	if (BoundPlayerPawn.IsValid() && BoundPlayerPawn.Get() != Pawn && RuntimeState.State == EPRDivergenceState::AwaitingChoice)
	{
		FPRDivergenceResult Result;
		Result.ResultId = FGuid::NewGuid();
		Result.RequestId = RuntimeState.RequestId;
		Result.DeathEventId = RuntimeState.DeathEventId;
		Result.CompanionId = RuntimeState.CompanionId;
		Result.Resolution = EPRDivergenceResolution::Cancelled;
		Result.WorldTimeSeconds = GetWorldTimeSeconds();
		PublishResult(Result);
		ClearActive(true);
	}
	UPRDivergenceComponent* Component = Controller->FindComponentByClass<UPRDivergenceComponent>();
	if (!Component)
	{
		Component = NewObject<UPRDivergenceComponent>(Controller, TEXT("DivergenceCacheComponent"));
		Component->RegisterComponent();
	}
	InputBridge = Component;
	Component->InitializeForSubsystem(this);
	if (UPRDivergenceDataAsset* Asset = DefinitionAsset.Get()) Component->SetWidgetClass(Asset->WidgetClass.LoadSynchronous());
	Component->RebindPlayerPawn(Pawn);
	BoundPlayerPawn = Pawn;
}

void UPRDivergenceSubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (Event.EventTag.MatchesTagExact(UPRTagLibrary::GetCombatEventDeathTag()) && Event.TargetId == TEXT("Player"))
	{
		TryBeginFromDeath(Event);
	}
}

void UPRDivergenceSubsystem::HandleDialogueResult(const FPRDialogueResult& Result)
{
	if (!Result.ResultId.IsValid() || (Result.Resolution != EPRDialogueChoiceResolution::Applied && Result.Resolution != EPRDialogueChoiceResolution::NoRelationshipChange))
	{
		return;
	}
	UPRCompanionSubsystem* Companions = CompanionSubsystem.Get();
	const FGameplayTag Primary = Companions ? Companions->GetSyncState().PrimaryCompanionId : FGameplayTag();
	if (Primary.IsValid() && Result.CompanionId.MatchesTagExact(Primary))
	{
		LastDialogueResultId = Result.ResultId;
		LastDialogueChoiceId = Result.ChoiceId;
		LastDialogueCompanionId = Result.CompanionId;
	}
}

void UPRDivergenceSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if ((Event.Operation == EPRSaveOperationType::Create || Event.Operation == EPRSaveOperationType::Load)
		&& (Event.Result == EPRSaveResult::Success || Event.Result == EPRSaveResult::RecoveredFromAlternate || Event.Result == EPRSaveResult::AlreadyLoaded))
	{
		bRunProtectionConsumed = false;
		ClearActive(false);
		LastDialogueResultId.Invalidate();
		LastDialogueChoiceId = NAME_None;
		LastDialogueCompanionId = FGameplayTag();
	}
}

void UPRDivergenceSubsystem::HandlePrimarySyncChanged(const FPRPrimaryCompanionSyncChangedEvent& Event)
{
	if (RuntimeState.State == EPRDivergenceState::AwaitingChoice)
	{
		FPRDivergenceResult Result;
		Result.ResultId = FGuid::NewGuid();
		Result.RequestId = RuntimeState.RequestId;
		Result.DeathEventId = RuntimeState.DeathEventId;
		Result.CompanionId = RuntimeState.CompanionId;
		Result.Resolution = EPRDivergenceResolution::Cancelled;
		Result.WorldTimeSeconds = GetWorldTimeSeconds();
		PublishResult(Result);
		ClearActive(true);
	}
	LastDialogueResultId.Invalidate();
	LastDialogueChoiceId = NAME_None;
	LastDialogueCompanionId = FGameplayTag();
}

bool UPRDivergenceSubsystem::BuildEligibility(FPRDivergenceEligibilityInput& OutInput, FPRCompanionRelationshipRecord& OutRelationship, FGameplayTag& OutPrimary) const
{
	OutInput = FPRDivergenceEligibilityInput();
	OutRelationship = FPRCompanionRelationshipRecord();
	OutPrimary = FGameplayTag();
	APRPlayerCharacter* Pawn = BoundPlayerPawn.Get();
	UPRCompanionSubsystem* Companions = CompanionSubsystem.Get();
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRProfileSaveData Profile;
	OutInput.bAuthority = Pawn && Pawn->HasAuthority();
	OutInput.bCurrentPlayerDead = Pawn && Pawn->GetAbilitySystemComponent() && Pawn->GetAbilitySystemComponent()->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
	OutInput.bHasLoadedProfile = Save && Save->GetLoadedProfileSnapshot(Profile);
	OutInput.bRunProtectionConsumed = bRunProtectionConsumed;

#if WITH_DEV_AUTOMATION_TESTS
	if (bUseAutomationProfile)
	{
		OutPrimary = AutomationRelationship.CompanionId;
		OutRelationship = AutomationRelationship;
		OutInput.bHasPrimaryCompanion = OutPrimary.IsValid();
		OutInput.bHasLoadedProfile = true;
		OutInput.Trust = OutRelationship.State.Trust;
		OutInput.Overload = OutRelationship.State.Overload;
		return FPRDivergenceContract::IsEligible(OutInput);
	}
#endif

	if (Companions)
	{
		OutPrimary = Companions->GetSyncState().PrimaryCompanionId;
		OutInput.bHasPrimaryCompanion = OutPrimary.IsValid() && Companions->GetRelationshipSnapshot(OutPrimary, OutRelationship);
	}
	OutInput.Trust = OutRelationship.State.Trust;
	OutInput.Overload = OutRelationship.State.Overload;
	return FPRDivergenceContract::IsEligible(OutInput);
}

bool UPRDivergenceSubsystem::TryBeginFromDeath(const FPRCombatEvent& Event)
{
	if (RuntimeState.State == EPRDivergenceState::AwaitingChoice || !bDefinitionReady)
	{
		return false;
	}
	FPRDivergenceEligibilityInput Eligibility;
	FPRCompanionRelationshipRecord Relationship;
	FGameplayTag Primary;
	if (!BuildEligibility(Eligibility, Relationship, Primary))
	{
		return false;
	}
	UPRDivergenceDataAsset* Asset = DefinitionAsset.Get();
	const FPRDivergencePresentationDefinition* Presentation = Asset ? Asset->FindPresentation(Primary) : nullptr;
	if (!Presentation)
	{
		return false;
	}
	bRunProtectionConsumed = true;
	RuntimeState = FPRDivergenceRuntimeState();
	RuntimeState.RequestId = FGuid::NewGuid();
	RuntimeState.DeathEventId = Event.EventId;
	RuntimeState.CompanionId = Primary;
	RuntimeState.State = EPRDivergenceState::AwaitingChoice;
	RuntimeState.SpeakerText = Presentation->SpeakerText;
	RuntimeState.PromptText = Presentation->PromptText;
	RuntimeState.StartTimeSeconds = GetWorldTimeSeconds();
	RuntimeState.ExpireTimeSeconds = RuntimeState.StartTimeSeconds + Asset->ChoiceWindowSeconds;
	RuntimeState.bRunProtectionConsumed = true;
	for (const FPRDivergenceChoiceDefinition& Definition : Presentation->Choices)
	{
		FPRDivergenceChoicePresentation Choice;
		Choice.Choice = Definition.Choice;
		Choice.DisplayText = Definition.DisplayText;
		Choice.InputHintText = FPRDivergenceContract::GetChoiceInputHint(Definition.Choice);
		RuntimeState.Choices.Add(Choice);
	}
	if (UWorld* World = BoundWorld.Get()) World->GetTimerManager().SetTimer(ExpiryTimer, this, &UPRDivergenceSubsystem::HandleExpired, Asset->ChoiceWindowSeconds, false);
	BroadcastState();
	return true;
}

bool UPRDivergenceSubsystem::SubmitChoice(const EPRDivergenceChoice Choice)
{
	if (RuntimeState.State != EPRDivergenceState::AwaitingChoice || Choice == EPRDivergenceChoice::None)
	{
		return false;
	}
	FPRDivergenceResult Result;
	if (!ApplyChoice(Choice, Result))
	{
		return false;
	}
	PublishResult(Result);
	ClearActive(true);
	return true;
}

bool UPRDivergenceSubsystem::ApplyChoice(const EPRDivergenceChoice Choice, FPRDivergenceResult& OutResult)
{
	OutResult = FPRDivergenceResult();
	OutResult.ResultId = FGuid::NewGuid();
	OutResult.RequestId = RuntimeState.RequestId;
	OutResult.DeathEventId = RuntimeState.DeathEventId;
	OutResult.CompanionId = RuntimeState.CompanionId;
	OutResult.Choice = Choice;
	OutResult.FutureDisposition = FPRDivergenceContract::GetFutureDisposition(Choice);
	if (LastDialogueCompanionId.MatchesTagExact(RuntimeState.CompanionId))
	{
		OutResult.DialogueResultId = LastDialogueResultId;
		OutResult.DialogueChoiceId = LastDialogueChoiceId;
	}
	OutResult.WorldTimeSeconds = GetWorldTimeSeconds();
	if (!FPRDivergenceContract::GetFixedRelationshipDelta(RuntimeState.CompanionId, Choice, OutResult.RelationshipDeltaRequest))
	{
		OutResult.Resolution = EPRDivergenceResolution::RejectedInvalid;
		return true;
	}
	UPRCompanionSubsystem* Companions = CompanionSubsystem.Get();
	FPRCompanionRelationshipRecord Before;

#if WITH_DEV_AUTOMATION_TESTS
	if (bUseAutomationProfile)
	{
		Before = AutomationRelationship;
	}
	else
#endif
	if (!Companions || !Companions->GetRelationshipSnapshot(RuntimeState.CompanionId, Before))
	{
		OutResult.Resolution = EPRDivergenceResolution::RejectedIneligible;
		return true;
	}
	OutResult.PreviousRelationship = Before.State;
	const float HealthFraction = Choice == EPRDivergenceChoice::Rescue ? FPRDivergenceContract::RescueHealthFraction : FPRDivergenceContract::ChallengeHealthFraction;
	if (Choice != EPRDivergenceChoice::Leave)
	{
		APRPlayerCharacter* Pawn = BoundPlayerPawn.Get();
		UPRCombatSubsystem* Combat = CombatSubsystem.Get();
		if (!Pawn || !Combat)
		{
			OutResult.Resolution = EPRDivergenceResolution::Cancelled;
			return true;
		}
		FPRReviveRequest Request;
		Request.SourceId = PRDivergence::SourceId;
		Request.DamageSource = this;
		Request.Instigator = Pawn;
		Request.Target = Pawn;
		Request.HealthFraction = HealthFraction;
		Request.ShieldFraction = FPRDivergenceContract::ReviveShieldFraction;
		if (Combat->Revive(Request) != EPRCombatRequestStatus::Applied)
		{
			OutResult.Resolution = EPRDivergenceResolution::RejectedCombat;
			return true;
		}
		OutResult.bReviveApplied = true;
		OutResult.AppliedHealthFraction = HealthFraction;
		OutResult.AppliedShieldFraction = FPRDivergenceContract::ReviveShieldFraction;
	}
	ApplyRelationshipAndSave(OutResult);
	return true;
}

bool UPRDivergenceSubsystem::ApplyRelationshipAndSave(FPRDivergenceResult& InOutResult)
{
	UPRCompanionSubsystem* Companions = CompanionSubsystem.Get();
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRCompanionRelationshipRecord After;

#if WITH_DEV_AUTOMATION_TESTS
	if (bUseAutomationProfile)
	{
		FPRRelationshipState Updated = AutomationRelationship.State;
		if (!FPRCompanionContract::ApplyDelta(Updated, InOutResult.RelationshipDeltaRequest))
		{
			InOutResult.Resolution = EPRDivergenceResolution::RejectedInvalid;
			return false;
		}
		AutomationRelationship.State = Updated;
		InOutResult.CurrentRelationship = Updated;
		InOutResult.SaveRequestId = FGuid::NewGuid();
		InOutResult.Resolution = EPRDivergenceResolution::Applied;
		return true;
	}
#endif

	if (!Companions || !Save || !Companions->GetRelationshipSnapshot(InOutResult.CompanionId, After))
	{
		InOutResult.Resolution = EPRDivergenceResolution::RejectedIneligible;
		return false;
	}
	Companions->ApplyRelationshipDelta(InOutResult.RelationshipDeltaRequest);
	if (!Companions->GetRelationshipSnapshot(InOutResult.CompanionId, After))
	{
		InOutResult.Resolution = EPRDivergenceResolution::RejectedIneligible;
		return false;
	}
	InOutResult.CurrentRelationship = After.State;
	if (IsRelationshipStateDifferent(InOutResult.PreviousRelationship, After.State))
	{
		Save->RequestSaveCurrentProfile(InOutResult.SaveRequestId);
	}
	InOutResult.Resolution = EPRDivergenceResolution::Applied;
	return true;
}

bool UPRDivergenceSubsystem::IsRelationshipStateDifferent(const FPRRelationshipState& Left, const FPRRelationshipState& Right) const
{
	return Left.Trust != Right.Trust || Left.Affection != Right.Affection || Left.Evaluation != Right.Evaluation || Left.Overload != Right.Overload;
}

void UPRDivergenceSubsystem::HandleExpired()
{
	if (RuntimeState.State != EPRDivergenceState::AwaitingChoice)
	{
		return;
	}
	FPRDivergenceResult Result;
	Result.ResultId = FGuid::NewGuid();
	Result.RequestId = RuntimeState.RequestId;
	Result.DeathEventId = RuntimeState.DeathEventId;
	Result.CompanionId = RuntimeState.CompanionId;
	Result.Resolution = EPRDivergenceResolution::Expired;
	Result.WorldTimeSeconds = GetWorldTimeSeconds();
	PublishResult(Result);
	ClearActive(true);
}

void UPRDivergenceSubsystem::BroadcastState() { StateChanged.Broadcast(RuntimeState); }
void UPRDivergenceSubsystem::PublishResult(const FPRDivergenceResult& Result) { LastResult = Result; ResultPublished.Broadcast(Result); }

void UPRDivergenceSubsystem::ClearActive(const bool bKeepRunConsumption)
{
	if (UWorld* World = BoundWorld.Get()) World->GetTimerManager().ClearTimer(ExpiryTimer);
	RuntimeState = FPRDivergenceRuntimeState();
	RuntimeState.bRunProtectionConsumed = bKeepRunConsumption && bRunProtectionConsumed;
	BroadcastState();
}

void UPRDivergenceSubsystem::LoadDefinition()
{
	DefinitionAsset = TSoftObjectPtr<UPRDivergenceDataAsset>(FSoftObjectPath(TEXT("/Game/ProjectR/Data/Divergence/DA_DivergenceCache.DA_DivergenceCache")));
	UPRDivergenceDataAsset* Asset = DefinitionAsset.LoadSynchronous();
	FString Error;
	bDefinitionReady = Asset && Asset->ValidateDefinition(Error);
}

double UPRDivergenceSubsystem::GetWorldTimeSeconds() const
{
	return BoundWorld.IsValid() ? BoundWorld->GetTimeSeconds() : 0.0;
}
