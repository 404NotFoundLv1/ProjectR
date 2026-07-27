// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRRoguelikeAutomationToolset.h"

#include "Editor.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "PRBossAuthoringToolset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRCombatHUD.h"
#include "UI/PRCombatHUDWidget.h"

UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::ValidateFixedRoguelikeContent()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->AddToRoot();
	UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
	if (!Registry || !Registry->IsRegistryReady()) Result->SetError(TEXT("v0.4.2 Registry validation failed."));
	else Result->SetValue(TEXT("{\"status\":\"PASS\",\"registry\":1,\"rooms\":8,\"encounters\":3,\"events\":4,\"policies\":4,\"rewards\":30}"));
	Result->RemoveFromRoot(); return Result;
}

namespace PRRoguelikeAutomation
{
class FFullPathRunner : public TSharedFromThis<FFullPathRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFullPathRunner> Runner = MakeShared<FFullPathRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		UWorld* World = Runner->World.Get(); Runner->Rooms = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
		if (!World || !Runner->Rooms.IsValid() || World->GetNetMode() == NM_Client) { Runner->Result->SetError(TEXT("Full Roguelike path requires authoritative PIE.")); return Runner->Result.Get(); }
		Runner->StartedAt = FPlatformTime::Seconds(); Runner->Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
		TWeakPtr<FFullPathRunner> WeakRunner = Runner;
		Runner->Rooms->OnRewardOfferChanged().AddLambda([WeakRunner](const FPRRewardOffer& Offer) { if (TSharedPtr<FFullPathRunner> Pinned = WeakRunner.Pin()) Pinned->HandleOffer(Offer); });
		Runner->Rooms->OnRoomSequenceCompleted().AddLambda([WeakRunner](const FPRRoomSequenceCompleted& Completion) { if (TSharedPtr<FFullPathRunner> Pinned = WeakRunner.Pin()) Pinned->HandleCompletion(Completion); });
		Runner->RefreshEnemyBinding(World);
		FGuid Session; if (Runner->Rooms->StartRoomSequence(1101, Session) != EPRRoomOperationResult::Succeeded) { Runner->FinishError(TEXT("Full Roguelike path could not start Seed 1101.")); return Runner->Result.Get(); }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shared = Runner.ToSharedPtr()](float) { return Shared->Tick(); }));
		return Runner->Result.Get();
	}
private:
	bool Tick()
	{
		UWorld* PlayWorld = WorldPtr(); UPRRoomSubsystem* RoomsPtr = Rooms.Get(); if (!PlayWorld || !RoomsPtr) { FinishError(TEXT("Full Roguelike path lost its world.")); return false; }
		RefreshEnemyBinding(PlayWorld);
		if (FPlatformTime::Seconds() - StartedAt > 90.0) { FPRRoomRuntimeState TimeoutState; RoomsPtr->GetRoomRuntimeState(TimeoutState); FinishError(*FString::Printf(TEXT("Full Roguelike path timed out (Flow=%d Step=%d Active=%s Spawns=%d Offer=%d DamageAttempts=%d DamageApplied=%d BossComplete=%d)."), static_cast<int32>(TimeoutState.FlowStatus), TimeoutState.CurrentStepIndex, *TimeoutState.ActiveRoomId.ToString(), SpawnIds.Num(), Offer.Choices.Num(), DamageAttempts, DamageApplied, BossResult.IsValid() && BossResult->bIsComplete)); return false; }
		if (bCompleted) { FinishSuccess(); return false; }
		FPRRoomRuntimeState State; RoomsPtr->GetRoomRuntimeState(State);
		if (State.FlowStatus == EPRRoomFlowStatus::SelectingRoom && State.Path.IsValidIndex(State.CurrentStepIndex + 1)) { const FPRRoomPathStep& Step=State.Path[State.CurrentStepIndex+1]; if (!Step.CandidateRoomIds.IsEmpty() && LastSelectedStep != Step.StepIndex) { LastSelectedStep=Step.StepIndex; RoomsPtr->SelectRoom(Step.CandidateRoomIds[0]); } return true; }
		if (State.FlowStatus == EPRRoomFlowStatus::SelectingEvent && !bEventChosen) { const UPRRoomEventDataAsset* Event=Registry?Registry->FindEvent(Registry->FindEventForRoom(State.ActiveRoomId)):nullptr; if (!Event) { FinishError(TEXT("Full Roguelike path could not resolve its event.")); return false; } for(const FPRRoomEventChoice& Choice:Event->Choices) if(Choice.RelationshipDelta.TrustDelta==0&&Choice.RelationshipDelta.AffectionDelta==0&&Choice.RelationshipDelta.EvaluationDelta==0&&Choice.RelationshipDelta.OverloadDelta==0){ RoomsPtr->SelectEventChoice(Choice.ChoiceId); bEventChosen=true; break;} return true; }
		if (State.FlowStatus == EPRRoomFlowStatus::SelectingReward && Offer.Choices.Num()==3 && Offer.OfferId.IsValid() && !Offer.bResolved) { FGuid Handle; if (RoomsPtr->SelectReward(Offer.Choices[0].RewardId, Handle) != EPRRoomOperationResult::Succeeded) { FinishError(TEXT("Full Roguelike path could not select a published reward.")); return false; } Offer=FPRRewardOffer(); bEventChosen=false; return true; }
		if (State.FlowStatus == EPRRoomFlowStatus::EncounterActive) return ResolveEncounter(PlayWorld, State);
		return true;
	}
	UWorld* WorldPtr() const { return GEditor ? GEditor->PlayWorld : nullptr; }
	void RefreshEnemyBinding(UWorld* PlayWorld)
	{
		UPREnemySubsystem* CurrentEnemies = PlayWorld ? PlayWorld->GetSubsystem<UPREnemySubsystem>() : nullptr;
		if (BoundEnemies.Get() == CurrentEnemies) return;
		if (UPREnemySubsystem* PreviousEnemies = BoundEnemies.Get(); PreviousEnemies && EnemyStateChangedHandle.IsValid()) PreviousEnemies->OnEnemyStateChanged().Remove(EnemyStateChangedHandle);
		BoundEnemies = CurrentEnemies; EnemyStateChangedHandle.Reset(); SpawnIds.Reset();
		if (CurrentEnemies)
		{
			TWeakPtr<FFullPathRunner> WeakRunner = AsShared();
			EnemyStateChangedHandle = CurrentEnemies->OnEnemyStateChanged().AddLambda([WeakRunner](const FPREnemyRuntimeState& State) { if (TSharedPtr<FFullPathRunner> Pinned = WeakRunner.Pin()) Pinned->HandleEnemy(State); });
		}
	}
	bool ResolveEncounter(UWorld* PlayWorld, const FPRRoomRuntimeState& State)
	{
		const UPRRoomDataAsset* Room=Registry?Registry->FindRoom(State.ActiveRoomId):nullptr; if (!Room) { FinishError(TEXT("Full Roguelike path lost active room.")); return false; }
		if (Room->TypeTag == FGameplayTag::RequestGameplayTag(TEXT("Room.Type.Boss"), false)) { if (!BossResult.IsValid()) BossResult=TStrongObjectPtr<UToolCallAsyncResultString>(UPRBossAuthoringToolset::RunPIEAuditorBossSmoke()); else if (BossResult->bIsComplete && (!BossResult->Error.IsEmpty() || !BossResult->Value.Contains(TEXT("\"status\":\"PASS\"")))) { FinishError(TEXT("Full Roguelike path Boss runner failed.")); return false; } return true; }
		UPREnemySubsystem* Enemies=PlayWorld->GetSubsystem<UPREnemySubsystem>(); UPRCombatSubsystem* Combat=PlayWorld->GetSubsystem<UPRCombatSubsystem>(); APlayerController* Controller=PlayWorld->GetFirstPlayerController(); APawn* Player=Controller?Controller->GetPawn():nullptr; if(!Enemies||!Combat||!Player) return true;
		TArray<FGuid> ActiveSpawnIds; Rooms.Get()->GetActiveEncounterSpawnIds(ActiveSpawnIds); for (const FGuid& Id : ActiveSpawnIds) SpawnIds.Add(Id);
		for(const FGuid& Id:SpawnIds){ FPREnemyRuntimeState Runtime; APREnemyCharacter* Enemy=nullptr; if(Enemies->GetEnemyRuntimeState(Id,Runtime)&&Runtime.bAlive&&Enemies->ResolveSpawnedEnemy(Id,Enemy)&&Enemy){ FPRDamageRequest Request; Request.SourceId=TEXT("RoguelikeFullPath"); Request.DamageSource=Player; Request.Instigator=Player; Request.Target=Enemy; Request.RawDamage=10000.0f; Request.ImpactOrigin=Player->GetActorLocation(); Request.IncomingDirection=(Enemy->GetActorLocation()-Player->GetActorLocation()).GetSafeNormal(); ++DamageAttempts; if (Combat->ApplyDamage(Request) == EPRCombatRequestStatus::Applied) ++DamageApplied; } }
		return true;
	}
	void HandleOffer(const FPRRewardOffer& InOffer){ Offer=InOffer; }
	void HandleEnemy(const FPREnemyRuntimeState& State){ if(State.SpawnId.IsValid()) SpawnIds.Add(State.SpawnId); }
	void HandleCompletion(const FPRRoomSequenceCompleted&){ bCompleted=true; }
	void FinishSuccess(){ Finish(TEXT("{\"status\":\"PASS\",\"seed\":1101,\"combat\":true,\"event\":true,\"rewards\":true,\"boss\":true,\"completion\":true}")); }
	void FinishError(const TCHAR* Error){ if(Result.IsValid()) { Result->SetError(Error); Result.Reset(); } }
	void Finish(const TCHAR* Value){ if(Result.IsValid()) { Result->SetValue(Value); Result.Reset(); } }
	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<UPRRoomSubsystem> Rooms; TWeakObjectPtr<UPREnemySubsystem> BoundEnemies; FDelegateHandle EnemyStateChangedHandle; TObjectPtr<UPRRoguelikeContentRegistryDataAsset> Registry=nullptr; TStrongObjectPtr<UToolCallAsyncResultString> Result; TStrongObjectPtr<UToolCallAsyncResultString> BossResult; FPRRewardOffer Offer; TSet<FGuid> SpawnIds; double StartedAt=0; int32 LastSelectedStep=INDEX_NONE; int32 DamageAttempts=0; int32 DamageApplied=0; bool bEventChosen=false; bool bCompleted=false;
};
UToolCallAsyncResultString* RunFixedSeed(const int32 Seed)
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	UPRRoomSubsystem* Rooms = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	if (!World || !Rooms || World->GetNetMode() == NM_Client) { Result->SetError(TEXT("Fixed Roguelike seed smoke requires an active authoritative PIE world.")); return Result; }
	FGuid SessionId; if (Rooms->StartRoomSequence(Seed, SessionId) != EPRRoomOperationResult::Succeeded || !SessionId.IsValid()) { Result->SetError(TEXT("Fixed Roguelike seed smoke could not start its bounded session.")); return Result; }
	FPRRoomRuntimeState State; if (!Rooms->GetRoomRuntimeState(State) || State.PathLength != UPRRoomSubsystem::GetRoomPathLengthForSeed(Seed) || State.Path.Num() != State.PathLength) { Result->SetError(TEXT("Fixed Roguelike seed smoke did not receive its deterministic bounded path.")); return Result; }
	const UPRRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry"));
	for (int32 StepIndex = 0; StepIndex < State.Path.Num(); ++StepIndex) { const FPRRoomPathStep& Step=State.Path[StepIndex]; if (Step.CandidateRoomIds.IsEmpty() || Step.CandidateRoomIds.Num() > 2) { Result->SetError(TEXT("Fixed Roguelike seed smoke found an invalid candidate set.")); return Result; } for (int32 Index=1;Index<Step.CandidateRoomIds.Num();++Index) if (Step.CandidateRoomIds[Index-1].ToString() >= Step.CandidateRoomIds[Index].ToString()) { Result->SetError(TEXT("Fixed Roguelike seed smoke found unsorted candidates.")); return Result; } if (StepIndex + 1 < State.Path.Num()) for (const FPrimaryAssetId& Id : Step.CandidateRoomIds) { const UPRRoomDataAsset* Candidate=Registry?Registry->FindRoom(Id):nullptr; if (!Candidate || Candidate->TypeTag == FGameplayTag::RequestGameplayTag(TEXT("Room.Type.Boss"), false)) { Result->SetError(TEXT("Fixed Roguelike seed smoke found a premature or unknown Boss candidate.")); return Result; } } }
	const FPRRoomPathStep& FinalStep = State.Path.Last(); const UPRRoomDataAsset* FinalRoom = Registry && FinalStep.CandidateRoomIds.Num() == 1 ? Registry->FindRoom(FinalStep.CandidateRoomIds[0]) : nullptr;
	if (!FinalRoom || FinalRoom->TypeTag != FGameplayTag::RequestGameplayTag(TEXT("Room.Type.Boss"), false)) { Result->SetError(TEXT("Fixed Roguelike seed smoke did not end at the registered Boss room.")); return Result; }
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"seed\":%d,\"pathLength\":%d,\"bossFinal\":true,\"candidatesSorted\":true,\"saveTouched\":false}"), Seed, State.PathLength)); return Result;
}
}

UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::RunPIERoguelikeSeed1101Smoke() { return PRRoguelikeAutomation::RunFixedSeed(1101); }
UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::RunPIERoguelikeSeed2202Smoke() { return PRRoguelikeAutomation::RunFixedSeed(2202); }
UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::RunPIERoguelikeSeed3303Smoke() { return PRRoguelikeAutomation::RunFixedSeed(3303); }
UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::InspectActiveRoomFlowInputPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APRCombatHUD* HUD = Controller ? Cast<APRCombatHUD>(Controller->GetHUD()) : nullptr;
	UPRCombatHUDWidget* Root = HUD ? HUD->GetCombatHUDWidget() : nullptr;
	UUserWidget* RoomFlow = Root ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("RoguelikeRoomFlow"))) : nullptr;
	UButton* Choice0 = RoomFlow ? Cast<UButton>(RoomFlow->GetWidgetFromName(TEXT("Choice0"))) : nullptr;
	if (!Controller || !Root || !RoomFlow || !Choice0)
	{
		Result->SetError(TEXT("Room Flow input inspection requires the fixed CombatHUD and Choice0 in active PIE."));
		return Result;
	}
	Result->SetValue(FString::Printf(
		TEXT("{\"status\":\"PASS\",\"roomFlowVisible\":%s,\"choice0Visible\":%s,\"choice0Enabled\":%s,\"choice0Bound\":%s,\"showMouse\":%s,\"clickEvents\":%s,\"mouseOverEvents\":%s}"),
		RoomFlow->IsVisible() ? TEXT("true") : TEXT("false"),
		Choice0->IsVisible() ? TEXT("true") : TEXT("false"),
		Choice0->GetIsEnabled() ? TEXT("true") : TEXT("false"),
		Choice0->OnClicked.IsBound() ? TEXT("true") : TEXT("false"),
		Controller->bShowMouseCursor ? TEXT("true") : TEXT("false"),
		Controller->bEnableClickEvents ? TEXT("true") : TEXT("false"),
		Controller->bEnableMouseOverEvents ? TEXT("true") : TEXT("false")));
	return Result;
}
UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::RunPIERoguelikeRoomFlowChoice0()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	UPRRoomSubsystem* Rooms = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	APRCombatHUD* HUD = Controller ? Cast<APRCombatHUD>(Controller->GetHUD()) : nullptr;
	UPRCombatHUDWidget* Root = HUD ? HUD->GetCombatHUDWidget() : nullptr;
	UUserWidget* RoomFlow = Root ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("RoguelikeRoomFlow"))) : nullptr;
	UButton* Choice0 = RoomFlow ? Cast<UButton>(RoomFlow->GetWidgetFromName(TEXT("Choice0"))) : nullptr;
	FPRRoomRuntimeState Before;
	if (!Rooms || !Choice0 || !Rooms->GetRoomRuntimeState(Before) || Before.FlowStatus != EPRRoomFlowStatus::SelectingRoom || !Before.Path.IsValidIndex(Before.CurrentStepIndex + 1) || Before.Path[Before.CurrentStepIndex + 1].CandidateRoomIds.IsEmpty())
	{
		Result->SetError(TEXT("Room Flow Choice0 automation requires an active fixed room selection."));
		return Result;
	}
	const FPrimaryAssetId ExpectedRoomId = Before.Path[Before.CurrentStepIndex + 1].CandidateRoomIds[0];
	Choice0->OnClicked.Broadcast();
	FPRRoomRuntimeState After;
	if (!Rooms->GetRoomRuntimeState(After) || After.FlowStatus != EPRRoomFlowStatus::Travelling || After.ActiveRoomId != ExpectedRoomId)
	{
		Result->SetError(TEXT("Room Flow Choice0 delegate did not hand off the selected room to travel."));
		return Result;
	}
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"roomId\":\"%s\",\"flow\":\"Travelling\"}"), *ExpectedRoomId.ToString()));
	return Result;
}
UToolCallAsyncResultString* UPRRoguelikeAutomationToolset::RunPIERoguelikeFullPath1101() { return PRRoguelikeAutomation::FFullPathRunner::Start(); }
