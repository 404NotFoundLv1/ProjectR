// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealityHub/PRRealityHubSubsystem.h"

#include "Core/PRGameInstance.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "RealityHub/PRRealityHubForecastPolicy.h"
#include "RealityHub/PRRealityHubTerminalDataAsset.h"
#include "RealityHub/PRRealityHubTerminalRegistryDataAsset.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "UI/PRRealityHubTrainingReturnWidget.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

void UPRRealityHubSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	RunStateSubsystem = GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get()) SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRRealityHubSubsystem::HandleSaveOperation);
	if (UPRRunStateSubsystem* Runs = RunStateSubsystem.Get()) AccountOperationHandle = Runs->OnAccountOperation().AddUObject(this, &UPRRealityHubSubsystem::HandleAccountOperation);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPRRealityHubSubsystem::HandlePostLoadMap);
	RebuildSnapshot();
}

void UPRRealityHubSubsystem::Deinitialize()
{
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get()) Save->OnSaveOperation().Remove(SaveOperationHandle);
	if (UPRRunStateSubsystem* Runs = RunStateSubsystem.Get()) Runs->OnAccountOperation().Remove(AccountOperationHandle);
	if (PostLoadMapHandle.IsValid()) FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	ClearTrainingReturnWidget();
	SaveOperationHandle.Reset();
	AccountOperationHandle.Reset();
	PendingTerminal = EPRRealityHubTerminal::None;
	StateChanged.Clear();
	Operation.Clear();
	Super::Deinitialize();
}

bool UPRRealityHubSubsystem::GetSnapshot(FPRRealityHubSnapshot& OutSnapshot) const { OutSnapshot = Snapshot; return true; }

FPRRealityHubForecast UPRRealityHubSubsystem::GetForecast() const
{
	FPRRealityHubForecast Forecast;
	UPRPlayerProfileSubsystem* Profiles = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRPlayerProfileSubsystem>() : nullptr;
	FPRPlayerProfileSnapshot Profile;
	const UPRRealityHubTerminalRegistryDataAsset* Registry = LoadRegistry();
	if (!Profiles || !Profiles->GetSnapshot(Profile)) { FPRRealityHubForecastPolicy::BuildUnavailable(Forecast); return Forecast; }
	if (!Registry || !Registry->IsRegistryReady())
	{
		Forecast.Result = EPRRealityHubForecastResult::UnavailableRegistry;
		Forecast.Explanation = FText::FromString(TEXT("No forecast is available."));
		return Forecast;
	}
	TArray<FGameplayTag> Candidates;
	Registry->GetForecastRuleIds(Candidates);
	FPRRealityHubForecastPolicy::BuildForecast(Profile, Candidates, Forecast);
	return Forecast;
}

void UPRRealityHubSubsystem::GetGraveyardSnapshot(TArray<FPRAccountRecord>& OutRecords) const
{
	OutRecords.Reset();
	if (const UPRRunStateSubsystem* Runs = RunStateSubsystem.Get()) Runs->GetGraveyardSnapshot(OutRecords);
}

bool UPRRealityHubSubsystem::GetProgressionSnapshot(FPRProgressionSnapshot& OutSnapshot) const
{
	if (const UPRProgressionSubsystem* Progression = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRProgressionSubsystem>() : nullptr)
	{
		return Progression->GetProgressionSnapshot(OutSnapshot);
	}
	OutSnapshot = FPRProgressionSnapshot();
	return false;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestLoadDefaultProfile()
{
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FGuid RequestId;
	if (!Save) return EPRRealityHubOperationResult::ProfileUnavailable;
	const EPRSaveResult Result = Save->LoadDefaultProfile(RequestId);
	if (Result == EPRSaveResult::Success || Result == EPRSaveResult::RecoveredFromAlternate || Result == EPRSaveResult::AlreadyLoaded)
	{
		RebuildSnapshot();
		PublishOperation(EPRRealityHubTerminal::CassetteSlot, EPRRealityHubOperationResult::Succeeded, FText::FromString(TEXT("Profile is ready.")));
		return EPRRealityHubOperationResult::Succeeded;
	}
	const EPRRealityHubOperationResult HubResult = Result == EPRSaveResult::NotFound ? EPRRealityHubOperationResult::ProfileCreationRequired : Result == EPRSaveResult::Busy ? EPRRealityHubOperationResult::Busy : EPRRealityHubOperationResult::ProfileUnavailable;
	PublishOperation(EPRRealityHubTerminal::CassetteSlot, HubResult, FText::FromString(TEXT("Profile could not be loaded.")));
	return HubResult;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestCreateDefaultProfile()
{
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FGuid RequestId;
	if (!Save) return EPRRealityHubOperationResult::ProfileUnavailable;
	const EPRSaveResult Result = Save->CreateNewDefaultProfile(RequestId);
	const EPRRealityHubOperationResult HubResult = Result == EPRSaveResult::Success ? EPRRealityHubOperationResult::Succeeded : Result == EPRSaveResult::Busy ? EPRRealityHubOperationResult::Busy : EPRRealityHubOperationResult::ProfileUnavailable;
	RebuildSnapshot();
	PublishOperation(EPRRealityHubTerminal::CassetteSlot, HubResult, HubResult == EPRRealityHubOperationResult::Succeeded ? FText::FromString(TEXT("Profile was created.")) : FText::FromString(TEXT("Profile could not be created.")));
	return HubResult;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestCreateFixedIdentityAccount(const EPRRealityHubIdentity Identity)
{
	UPRRunStateSubsystem* Runs = RunStateSubsystem.Get();
	FGuid RequestId;
	if (!Runs) return EPRRealityHubOperationResult::RunStateUnavailable;
	const EPRAccountOperationResult Result = Runs->RequestCreateAccount(GetFixedIdentityId(Identity), RequestId);
	const EPRRealityHubOperationResult HubResult = FromAccountResult(Result);
	if (HubResult == EPRRealityHubOperationResult::Succeeded) PendingTerminal = EPRRealityHubTerminal::CassetteSlot;
	else PublishOperation(EPRRealityHubTerminal::CassetteSlot, HubResult, FText::FromString(TEXT("Account request was rejected.")));
	RebuildSnapshot();
	return HubResult;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestStartRun()
{
	UPRRunStateSubsystem* Runs = RunStateSubsystem.Get();
	FPRActiveAccountSaveData Account;
	FGuid RequestId;
	if (!Runs || !Runs->GetActiveAccountSnapshot(Account)) return EPRRealityHubOperationResult::RunStateUnavailable;
	const EPRRealityHubOperationResult HubResult = FromAccountResult(Runs->RequestStartRun(MakeFixedRunSeed(Account.AccountId), RequestId));
	if (HubResult == EPRRealityHubOperationResult::Succeeded) PendingTerminal = EPRRealityHubTerminal::CassetteSlot;
	else PublishOperation(EPRRealityHubTerminal::CassetteSlot, HubResult, FText::FromString(TEXT("Network entry was rejected.")));
	RebuildSnapshot();
	return HubResult;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RetryPendingOperation()
{
	UPRRunStateSubsystem* Runs = RunStateSubsystem.Get();
	FGuid RequestId;
	if (!Runs) return EPRRealityHubOperationResult::RunStateUnavailable;
	const EPRRealityHubOperationResult HubResult = FromAccountResult(Runs->RetryPendingPersistence(RequestId));
	if (HubResult == EPRRealityHubOperationResult::Succeeded) PendingTerminal = EPRRealityHubTerminal::CassetteSlot;
	return HubResult;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestTrainingTravel()
{
	ClearTrainingReturnWidget();
	bTrainingTravelActive = true;
	const bool bOpened = Cast<UPRGameInstance>(GetGameInstance()) && Cast<UPRGameInstance>(GetGameInstance())->OpenMap(EPRMapId::CombatGym);
	if (!bOpened) bTrainingTravelActive = false;
	const EPRRealityHubOperationResult Result = bOpened ? EPRRealityHubOperationResult::Succeeded : EPRRealityHubOperationResult::TravelFailed;
	PublishOperation(EPRRealityHubTerminal::TrainingSimulator, Result, bOpened ? FText::FromString(TEXT("Training opened.")) : FText::FromString(TEXT("Training could not open.")));
	return Result;
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::RequestReturnToRealityHub()
{
	bTrainingTravelActive = false;
	ClearTrainingReturnWidget();
	const bool bOpened = Cast<UPRGameInstance>(GetGameInstance()) && Cast<UPRGameInstance>(GetGameInstance())->OpenMap(EPRMapId::RealityHub);
	const EPRRealityHubOperationResult Result = bOpened ? EPRRealityHubOperationResult::Succeeded : EPRRealityHubOperationResult::TravelFailed;
	PublishOperation(EPRRealityHubTerminal::TrainingSimulator, Result, bOpened ? FText::FromString(TEXT("Returned to Reality Hub.")) : FText::FromString(TEXT("Return travel failed.")));
	return Result;
}

FPrimaryAssetId UPRRealityHubSubsystem::GetFixedIdentityId(const EPRRealityHubIdentity Identity)
{
	static const FName Names[] = { TEXT("Technician"), TEXT("Security"), TEXT("Exile"), TEXT("Observer"), TEXT("Blank") };
	const uint8 Index = static_cast<uint8>(Identity);
	return Index < UE_ARRAY_COUNT(Names) ? FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRAccountIdentity")), Names[Index]) : FPrimaryAssetId();
}

int32 UPRRealityHubSubsystem::MakeFixedRunSeed(const FGuid& AccountId)
{
	return FMath::Max(1, static_cast<int32>(GetTypeHash(AccountId) & 0x7fffffffU));
}

UPRRealityHubSubsystem::FPRRealityHubStateChangedNative& UPRRealityHubSubsystem::OnStateChanged() { return StateChanged; }
UPRRealityHubSubsystem::FPRRealityHubOperationNative& UPRRealityHubSubsystem::OnOperation() { return Operation; }

void UPRRealityHubSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event) { RebuildSnapshot(); }

void UPRRealityHubSubsystem::HandleAccountOperation(const FPRAccountOperationEvent& Event)
{
	if (PendingTerminal == EPRRealityHubTerminal::None) return;
	if (Event.Result == EPRAccountOperationResult::Started) return;
	const EPRRealityHubOperationResult Result = FromAccountResult(Event.Result);
	PublishOperation(PendingTerminal, Result, Result == EPRRealityHubOperationResult::Succeeded ? FText::FromString(TEXT("Account operation completed.")) : FText::FromString(TEXT("Account operation failed.")));
	PendingTerminal = EPRRealityHubTerminal::None;
	RebuildSnapshot();
}

void UPRRealityHubSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	ClearTrainingReturnWidget();
	if (!bTrainingTravelActive || !LoadedWorld || !LoadedWorld->GetMapName().Contains(TEXT("L_CombatGym"))) return;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(LoadedWorld, 0);
	if (!PlayerController) return;
	static const TSoftClassPtr<UPRRealityHubTrainingReturnWidget> ReturnWidgetClass(FSoftObjectPath(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubTrainingReturn.WBP_RealityHubTrainingReturn_C")));
	if (TSubclassOf<UPRRealityHubTrainingReturnWidget> Class = ReturnWidgetClass.LoadSynchronous())
	{
		TrainingReturnWidget = CreateWidget<UPRRealityHubTrainingReturnWidget>(PlayerController, Class);
		if (UPRRealityHubTrainingReturnWidget* Widget = TrainingReturnWidget.Get()) Widget->AddToPlayerScreen();
	}
}

void UPRRealityHubSubsystem::ClearTrainingReturnWidget()
{
	if (UPRRealityHubTrainingReturnWidget* Widget = TrainingReturnWidget.Get()) Widget->RemoveFromParent();
	TrainingReturnWidget = nullptr;
}

void UPRRealityHubSubsystem::RebuildSnapshot()
{
	Snapshot.Sequence++;
	Snapshot.bProfileLoaded = SaveSubsystem.IsValid() && SaveSubsystem->GetSaveRuntimeState().bHasLoadedProfile;
	Snapshot.bOperationPending = PendingTerminal != EPRRealityHubTerminal::None;
	Snapshot.Terminals.Reset();
	const UPRRealityHubTerminalRegistryDataAsset* Registry = LoadRegistry();
	for (uint8 Value = static_cast<uint8>(EPRRealityHubTerminal::CassetteSlot); Value <= static_cast<uint8>(EPRRealityHubTerminal::DirectorForecaster); ++Value)
	{
		FPRRealityHubTerminalSnapshot Terminal;
		Terminal.Terminal = static_cast<EPRRealityHubTerminal>(Value);
		const UPRRealityHubTerminalDataAsset* Definition = Registry ? Registry->FindTerminal(Terminal.Terminal) : nullptr;
		Terminal.bAvailable = Definition != nullptr;
		Terminal.DisplayName = Definition ? Definition->DisplayName : FText::FromString(TEXT("Unavailable terminal"));
		Terminal.StatusText = Definition ? Definition->Description : FText::FromString(TEXT("Terminal registry unavailable."));
		Snapshot.Terminals.Add(Terminal);
	}
	StateChanged.Broadcast(Snapshot);
}

void UPRRealityHubSubsystem::PublishOperation(const EPRRealityHubTerminal Terminal, const EPRRealityHubOperationResult Result, const FText& Message)
{
	FPRRealityHubOperationEvent Event;
	Event.Sequence = ++OperationSequence;
	Event.Terminal = Terminal;
	Event.Result = Result;
	Event.Message = Message;
	Operation.Broadcast(Event);
}

EPRRealityHubOperationResult UPRRealityHubSubsystem::FromAccountResult(const EPRAccountOperationResult Result) const
{
	switch (Result)
	{
	case EPRAccountOperationResult::Started: return EPRRealityHubOperationResult::Succeeded;
	case EPRAccountOperationResult::RejectedNoProfile: return EPRRealityHubOperationResult::ProfileUnavailable;
	case EPRAccountOperationResult::RejectedRegistryUnavailable:
	case EPRAccountOperationResult::RejectedUnknownIdentity: return EPRRealityHubOperationResult::IdentityUnavailable;
	case EPRAccountOperationResult::PersistenceFailed: return EPRRealityHubOperationResult::PersistenceFailed;
	case EPRAccountOperationResult::RejectedBusy:
	case EPRAccountOperationResult::AlreadyPending: return EPRRealityHubOperationResult::Busy;
	default: return EPRRealityHubOperationResult::OperationUnavailable;
	}
}

const UPRRealityHubTerminalRegistryDataAsset* UPRRealityHubSubsystem::LoadRegistry() const
{
	return LoadObject<UPRRealityHubTerminalRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/RealityHub/DA_RealityHubTerminalRegistry.DA_RealityHubTerminalRegistry"));
}
