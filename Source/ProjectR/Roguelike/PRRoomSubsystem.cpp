// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRoomSubsystem.h"

#include "AbilitySystemInterface.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Director/PRDirectorSubsystem.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardApplication.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "QTE/PRQTESubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace PRRoomRuntime
{
FGameplayTag FixedTag(const TCHAR* Name)
{
	return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

uint32 NextRandom(uint32& State)
{
	State += 0x9E3779B9U;
	uint32 Value = State;
	Value = (Value ^ (Value >> 16U)) * 0x85EBCA6BU;
	Value = (Value ^ (Value >> 13U)) * 0xC2B2AE35U;
	return Value ^ (Value >> 16U);
}

bool HasType(const UPRRoomDataAsset& Room, const TCHAR* Type)
{
	return Room.TypeTag == FixedTag(Type);
}
}

int32 UPRRoomSubsystem::GetRoomPathLengthForSeed(const int32 Seed)
{
	return 6 + static_cast<int32>(static_cast<uint32>(Seed) % 5U);
}

void UPRRoomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegistryAsset = TSoftObjectPtr<UPRRoguelikeContentRegistryDataAsset>(FSoftObjectPath(TEXT("/Game/ProjectR/Roguelike/DA_RoguelikeContentRegistry.DA_RoguelikeContentRegistry")));
	Registry = RegistryAsset.LoadSynchronous();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPRRoomSubsystem::HandlePostLoadMap);
	if (UWorld* World = GetWorld()) if (UPRQTESubsystem* QTE = World->GetSubsystem<UPRQTESubsystem>()) QTEResultHandle = QTE->OnQTEResult().AddUObject(this, &UPRRoomSubsystem::HandleQTEResult);
}

void UPRRoomSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid()) FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	ClearWorldBindings();
	ResetSession();
	Registry = nullptr;
	Super::Deinitialize();
}

EPRRoomOperationResult UPRRoomSubsystem::StartRoomSequence(const int32 Seed, FGuid& OutSessionId)
{
	OutSessionId.Invalidate();
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::Idle && RuntimeState.FlowStatus != EPRRoomFlowStatus::Completed && RuntimeState.FlowStatus != EPRRoomFlowStatus::Cancelled)
	{
		return EPRRoomOperationResult::AlreadyStarted;
	}
	if (!Registry) Registry = RegistryAsset.LoadSynchronous();
	if (!Registry || !Registry->IsRegistryReady()) return EPRRoomOperationResult::NotReady;
	ResetSession();
	RuntimeState.SessionId = FGuid::NewGuid();
	RuntimeState.Seed = Seed;
	RuntimeState.PathLength = GetRoomPathLengthForSeed(Seed);
	if (!BuildPath()) { ResetSession(); return EPRRoomOperationResult::NoEligibleContent; }
	RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingRoom;
	OutSessionId = RuntimeState.SessionId;
	BroadcastState();
	return EPRRoomOperationResult::Succeeded;
}

EPRRoomOperationResult UPRRoomSubsystem::SelectRoom(const FPrimaryAssetId RoomId)
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::SelectingRoom || !RuntimeState.Path.IsValidIndex(RuntimeState.CurrentStepIndex + 1)) return EPRRoomOperationResult::NotReady;
	FPRRoomPathStep& Step = RuntimeState.Path[RuntimeState.CurrentStepIndex + 1];
	if (!Step.CandidateRoomIds.Contains(RoomId) || !Registry || !Registry->FindRoom(RoomId)) return EPRRoomOperationResult::Rejected;
	if (RuntimeState.CurrentStepIndex >= 0 && RuntimeState.Path[RuntimeState.CurrentStepIndex].SelectedRoomId == RoomId) return EPRRoomOperationResult::Rejected;
	const UPRRoomDataAsset* Room = Registry->FindRoom(RoomId);
	if (!Room || !IsRoomEligible(*Room) || Room->LevelAsset.IsNull()) return EPRRoomOperationResult::Rejected;
	Step.SelectedRoomId = RoomId;
	RuntimeState.CurrentStepIndex = Step.StepIndex;
	RuntimeState.ActiveRoomId = RoomId;
	RuntimeState.bEncounterComplete = false;
	RuntimeState.FlowStatus = EPRRoomFlowStatus::Travelling;
	BroadcastState();
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Room->LevelAsset);
	return EPRRoomOperationResult::Succeeded;
}

EPRRoomOperationResult UPRRoomSubsystem::SelectEventChoice(const FName ChoiceId)
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::SelectingEvent || !Registry) return EPRRoomOperationResult::NotReady;
	const FPrimaryAssetId EventId = Registry->FindEventForRoom(RuntimeState.ActiveRoomId);
	const UPRRoomEventDataAsset* Event = Registry->FindEvent(EventId);
	if (!Event) return EPRRoomOperationResult::NotFound;
	const FPRRoomEventChoice* Choice = Event->Choices.FindByPredicate([ChoiceId](const FPRRoomEventChoice& Candidate) { return Candidate.ChoiceId == ChoiceId; });
	if (!Choice) return EPRRoomOperationResult::Rejected;
	if (Choice->bRequiresQTESuccess && LastQTEResultTag != UPRTagLibrary::GetQTEResultSuccessTag()) return EPRRoomOperationResult::Rejected;
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	if (!IsRelationshipDeltaEmpty(Choice->RelationshipDelta) && (!Save || !Save->GetSaveRuntimeState().bHasLoadedProfile)) return EPRRoomOperationResult::Rejected;
	FPRRoomEventResult Result;
	Result.ResolutionId = FGuid::NewGuid(); Result.RoomId = RuntimeState.ActiveRoomId; Result.EventId = EventId; Result.ChoiceId = ChoiceId; Result.bEpicWeightBoosted = Choice->bBoostEpicWeight;
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	if (Save && Save->GetSaveRuntimeState().bHasLoadedProfile && Companions)
	{
		FPRRelationshipDelta Delta = Choice->RelationshipDelta;
		Delta.CompanionId = Companions->GetSyncState().PrimaryCompanionId;
		Delta.SourceId = FName(TEXT("RoomEvent"));
		const EPRRelationshipApplyResult RelationshipResult = Delta.CompanionId.IsValid() ? Companions->ApplyRelationshipDelta(Delta) : EPRRelationshipApplyResult::Invalid;
		Result.bChoiceApplied = RelationshipResult == EPRRelationshipApplyResult::Applied || RelationshipResult == EPRRelationshipApplyResult::AppliedClamped;
	}
	bCurrentOfferEpicWeightBoosted = Choice->bBoostEpicWeight;
	EventResolved.Broadcast(Result);
	RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingReward;
	CreateRewardOffer();
	BroadcastState();
	return EPRRoomOperationResult::Succeeded;
}

EPRRoomOperationResult UPRRoomSubsystem::ConfirmSafeRoomExit()
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::SelectingReward || !Registry) return EPRRoomOperationResult::NotReady;
	const UPRRoomDataAsset* Room = Registry->FindRoom(RuntimeState.ActiveRoomId);
	if (!Room || !PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Safe"))) return EPRRoomOperationResult::Rejected;
	if (!ActiveOffer.OfferId.IsValid() || ActiveOffer.Choices.Num() != 3) CreateRewardOffer();
	return EPRRoomOperationResult::Succeeded;
}

EPRRoomOperationResult UPRRoomSubsystem::SelectReward(const FPrimaryAssetId RewardId, FGuid& OutHandleId)
{
	OutHandleId.Invalidate();
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::SelectingReward || ActiveOffer.bResolved || !ActiveOffer.Choices.ContainsByPredicate([RewardId](const FPRRewardOfferChoice& Choice) { return Choice.RewardId == RewardId; })) return EPRRoomOperationResult::Rejected;
	const UPRRewardDataAsset* Reward = Registry ? Registry->FindReward(RewardId) : nullptr;
	UAbilitySystemComponent* ASC = ResolvePlayerAbilitySystem();
	if (!Reward || !ASC) return EPRRoomOperationResult::NotReady;
	if (!IsRewardEligible(*Reward)) return EPRRoomOperationResult::Rejected;
	FActiveGameplayEffectHandle EffectHandle;
	const EPRRewardApplyResult Applied = FPRGASRewardApplication::Apply(*ASC, Reward->EffectSpec, EffectHandle);
	if (Applied != EPRRewardApplyResult::Applied) return EPRRoomOperationResult::Rejected;
	FPRRewardApplicationHandle Handle;
	Handle.HandleId = FGuid::NewGuid(); Handle.RewardId = RewardId; Handle.FamilyId = Reward->FamilyId; Handle.Tier = Reward->Tier; Handle.ApplicationId = Reward->ApplicationId; Handle.bPersistent = Reward->EffectSpec.Duration == EPRRewardEffectDuration::Session;
	for (int32 Index = AppliedRewards.Num() - 1; Index >= 0; --Index)
	{
		if (AppliedRewards[Index].FamilyId == Handle.FamilyId && AppliedRewards[Index].Tier <= Handle.Tier)
		{
			if (const FActiveGameplayEffectHandle* Existing = GameplayEffectHandles.Find(AppliedRewards[Index].HandleId)) FPRGASRewardApplication::Remove(*ASC, *Existing);
			GameplayEffectHandles.Remove(AppliedRewards[Index].HandleId); AppliedRewards.RemoveAt(Index);
		}
	}
	if (Handle.bPersistent) GameplayEffectHandles.Add(Handle.HandleId, EffectHandle);
	AppliedRewards.Add(Handle); OutHandleId = Handle.HandleId;
	ActiveOffer.bResolved = true; RewardOfferChanged.Broadcast(ActiveOffer); RuntimeState.ActiveRewardOfferId.Invalidate();
	if (RuntimeState.CurrentStepIndex + 1 >= RuntimeState.Path.Num()) CompleteSequence();
	else { RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingRoom; BroadcastState(); }
	return EPRRoomOperationResult::Succeeded;
}

bool UPRRoomSubsystem::GetRoomRuntimeState(FPRRoomRuntimeState& OutState) const { OutState = RuntimeState; return true; }
void UPRRoomSubsystem::GetActiveEncounterSpawnIds(TArray<FGuid>& OutSpawnIds) const { OutSpawnIds = ActiveEncounterSpawnIds; }
void UPRRoomSubsystem::GetAppliedRewards(TArray<FPRRewardApplicationHandle>& OutHandles) const { OutHandles = AppliedRewards; }
FPRRoomStateChangedNative& UPRRoomSubsystem::OnRoomStateChanged() { return StateChanged; }
FPRRewardOfferChangedNative& UPRRoomSubsystem::OnRewardOfferChanged() { return RewardOfferChanged; }
FPRRoomEventResolvedNative& UPRRoomSubsystem::OnRoomEventResolved() { return EventResolved; }
FPRRoomSequenceCompletedNative& UPRRoomSubsystem::OnRoomSequenceCompleted() { return SequenceCompleted; }

bool UPRRoomSubsystem::BuildPath()
{
	TArray<const UPRRoomDataAsset*> Rooms;
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Registry->Rooms) if (const UPRRoomDataAsset* Room = Reference.LoadSynchronous()) if (IsRoomEligible(*Room) && !PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Shop"))) Rooms.Add(Room);
	Rooms.Sort([](const UPRRoomDataAsset& A, const UPRRoomDataAsset& B) { return A.GetPrimaryAssetId().ToString() < B.GetPrimaryAssetId().ToString(); });
	if (Rooms.Num() < 5) return false;
	const TCHAR* RequiredTypes[] = { TEXT("Room.Type.Combat"), TEXT("Room.Type.Event"), TEXT("Room.Type.Safe") };
	uint32 Random = static_cast<uint32>(RuntimeState.Seed);
	for (int32 Index = 0; Index < RuntimeState.PathLength; ++Index)
	{
		FPRRoomPathStep& Step = RuntimeState.Path.AddDefaulted_GetRef(); Step.StepIndex = Index;
		const TCHAR* Required = Index == RuntimeState.PathLength - 1 ? TEXT("Room.Type.Boss") : (Index < UE_ARRAY_COUNT(RequiredTypes) ? RequiredTypes[Index] : nullptr);
		for (const UPRRoomDataAsset* Room : Rooms)
		{
			if ((!Required && PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Boss"))) || (Required && !PRRoomRuntime::HasType(*Room, Required))) continue;
			Step.CandidateRoomIds.Add(Room->GetPrimaryAssetId());
		}
		if (Step.CandidateRoomIds.IsEmpty()) return false;
		Step.CandidateRoomIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
		if (Step.CandidateRoomIds.Num() > 2)
		{
			TArray<FPrimaryAssetId> Remaining = Step.CandidateRoomIds;
			TArray<FPrimaryAssetId> Selected;
			while (Remaining.Num() && Selected.Num() < 2)
			{
				int32 TotalWeight = 0;
				for (const FPrimaryAssetId& CandidateId : Remaining) if (const UPRRoomDataAsset* Candidate = Registry->FindRoom(CandidateId)) TotalWeight += GetRoomWeight(*Candidate);
				if (TotalWeight <= 0) break;
				int32 Roll = static_cast<int32>(PRRoomRuntime::NextRandom(Random) % static_cast<uint32>(TotalWeight));
				int32 SelectedIndex = INDEX_NONE;
				for (int32 CandidateIndex = 0; CandidateIndex < Remaining.Num(); ++CandidateIndex)
				{
					const UPRRoomDataAsset* Candidate = Registry->FindRoom(Remaining[CandidateIndex]);
					const int32 Weight = Candidate ? GetRoomWeight(*Candidate) : 0;
					if (Roll < Weight) { SelectedIndex = CandidateIndex; break; }
					Roll -= Weight;
				}
				if (SelectedIndex == INDEX_NONE) break;
				Selected.Add(Remaining[SelectedIndex]); Remaining.RemoveAt(SelectedIndex);
			}
			if (Selected.Num() != 2) return false;
			Step.CandidateRoomIds = MoveTemp(Selected);
			Step.CandidateRoomIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
		}
	}
	return true;
}

bool UPRRoomSubsystem::IsRoomEligible(const UPRRoomDataAsset& Room) const
{
	for (const FPRRoomCondition& Condition : Room.EntryConditions) if (!DoesConditionPass(Condition)) return false;
	return true;
}

bool UPRRoomSubsystem::IsRewardEligible(const UPRRewardDataAsset& Reward) const
{
	if (!FPRRewardContract::CanSelectFamilyTier(AppliedRewards, Reward.FamilyId, Reward.Tier)) return false;
	for (const FPRRoomCondition& Condition : Reward.WeightConditions) if (!DoesConditionPass(Condition)) return false;
	for (const FPRRewardApplicationHandle& Handle : AppliedRewards)
	{
		const UPRRewardDataAsset* Existing = Registry ? Registry->FindReward(Handle.RewardId) : nullptr;
		if (Existing && Reward.MutualExclusionTags.HasAny(Existing->MutualExclusionTags)) return false;
	}
	return true;
}

bool UPRRoomSubsystem::IsRelationshipDeltaEmpty(const FPRRelationshipDelta& Delta) const
{
	return Delta.TrustDelta == 0 && Delta.AffectionDelta == 0 && Delta.EvaluationDelta == 0 && Delta.OverloadDelta == 0;
}

int32 UPRRoomSubsystem::GetRoomWeight(const UPRRoomDataAsset& Room) const
{
	int32 Weight = 1;
	const UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
	if (!Director || !Registry) return Weight;
	for (const FPRDirectorRoomWeightAdjustment& Adjustment : Registry->DirectorRoomWeightAdjustments)
	{
		if (Adjustment.RoomType != Room.TypeTag || !Adjustment.RuleId.IsValid()) continue;
		FPRDirectorRuleRuntimeState State;
		if (Director->GetRuleRuntimeState(Adjustment.RuleId, State) && (State.Status == EPRDirectorRuleRuntimeStatus::Active || State.Status == EPRDirectorRuleRuntimeStatus::Degraded)) Weight += Adjustment.WeightDelta * State.Level;
	}
	return FMath::Max(0, Weight);
}

bool UPRRoomSubsystem::DoesConditionPass(const FPRRoomCondition& Condition) const
{
	switch (Condition.Kind)
	{
	case EPRRoomConditionKind::Always: return true;
	case EPRRoomConditionKind::MinClearedRooms: return RuntimeState.CurrentStepIndex + 1 >= Condition.IntegerValue;
	case EPRRoomConditionKind::DirectorRule:
		if (const UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr) { FPRDirectorRuleRuntimeState State; return Director->GetRuleRuntimeState(Condition.Tag, State) && (State.Status == EPRDirectorRuleRuntimeStatus::Active || State.Status == EPRDirectorRuleRuntimeStatus::Degraded) && State.Level >= Condition.IntegerValue; } return false;
	case EPRRoomConditionKind::PrimaryCompanion:
		if (const UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr) return Companions->GetSyncState().PrimaryCompanionId == Condition.Tag; return false;
	default: return false;
	}
}

void UPRRoomSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || RuntimeState.FlowStatus != EPRRoomFlowStatus::Travelling) return;
	if (UPRQTESubsystem* QTE = LoadedWorld->GetSubsystem<UPRQTESubsystem>()) QTEResultHandle = QTE->OnQTEResult().AddUObject(this, &UPRRoomSubsystem::HandleQTEResult);
	if (!RebindSessionGameplayEffects()) { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	StartEncounter();
}

void UPRRoomSubsystem::StartEncounter()
{
	const UPRRoomDataAsset* Room = Registry ? Registry->FindRoom(RuntimeState.ActiveRoomId) : nullptr;
	if (!Room) { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	if (PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Event"))) { RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingEvent; BroadcastState(); return; }
	if (PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Safe"))) { RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingReward; CreateRewardOffer(); BroadcastState(); return; }
	const UPREncounterDataAsset* Encounter = Registry->FindEncounter(Room->EncounterId);
	if (!Encounter || !GetWorld()) { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>()) CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRRoomSubsystem::HandleCombatEvent);
	if (Encounter->Kind == EPRRoomEncounterKind::Boss)
	{
		if (UPRBossSubsystem* Boss = GetWorld()->GetSubsystem<UPRBossSubsystem>()) BossCompletedHandle = Boss->OnPrototypeRunCompleted().AddUObject(this, &UPRRoomSubsystem::HandleBossCompleted);
		RuntimeState.FlowStatus = EPRRoomFlowStatus::EncounterActive; BroadcastState(); return;
	}
	UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>();
	if (!Enemies || !Enemies->IsRegistryReady())
	{
		GetWorld()->GetTimerManager().SetTimer(EncounterRetryTimer, this, &UPRRoomSubsystem::StartEncounter, 0.1f, false);
		return;
	}
	EnemyStateChangedHandle = Enemies->OnEnemyStateChanged().AddUObject(this, &UPRRoomSubsystem::HandleEnemyStateChanged);
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Origin = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	for (const FPREncounterSpawnDefinition& Spawn : Encounter->SpawnDefinitions)
	{
		FGuid SpawnId; class APREnemyCharacter* Enemy = nullptr;
		if (Enemies->SpawnEnemyPrototype(Spawn.PrototypeTag, FTransform(FRotator::ZeroRotator, Origin + Spawn.RelativeLocation), SpawnId, Enemy) == EPREnemySpawnStatus::Spawned) ActiveEncounterSpawnIds.Add(SpawnId);
	}
	if (ActiveEncounterSpawnIds.IsEmpty()) { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	RuntimeState.FlowStatus = EPRRoomFlowStatus::EncounterActive;
	GetWorld()->GetTimerManager().SetTimer(EncounterCompletionTimer, this, &UPRRoomSubsystem::CheckEncounterCompletion, 0.1f, true);
	BroadcastState();
}

void UPRRoomSubsystem::CheckEncounterCompletion()
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::EncounterActive || !GetWorld()) return;
	UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>();
	if (!Enemies || ActiveEncounterSpawnIds.IsEmpty()) return;
	for (const FGuid& SpawnId : ActiveEncounterSpawnIds) { FPREnemyRuntimeState State; if (Enemies->GetEnemyRuntimeState(SpawnId, State) && State.bAlive) return; }
	CompleteEncounter();
}

void UPRRoomSubsystem::HandleEnemyStateChanged(const FPREnemyRuntimeState& State)
{
	if (!ActiveEncounterSpawnIds.Contains(State.SpawnId) || State.bAlive) return;
	for (const FGuid& SpawnId : ActiveEncounterSpawnIds)
	{
		FPREnemyRuntimeState Existing;
		if (GetWorld() && GetWorld()->GetSubsystem<UPREnemySubsystem>()->GetEnemyRuntimeState(SpawnId, Existing) && Existing.bAlive) return;
	}
	CompleteEncounter();
}

void UPRRoomSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Result) { if (RuntimeState.FlowStatus == EPRRoomFlowStatus::EncounterActive) CompleteEncounter(); }
void UPRRoomSubsystem::HandleCombatEvent(const FPRCombatEvent& Event) { if (RuntimeState.FlowStatus == EPRRoomFlowStatus::EncounterActive && Event.EventId.IsValid()) { LastCombatEventId = Event.EventId; LastCombatEventTag = Event.EventTag; } }
void UPRRoomSubsystem::HandleQTEResult(const FPRQTEResult& Result) { LastQTEResultTag = Result.ResultTag; }

void UPRRoomSubsystem::CompleteEncounter()
{
	RuntimeState.bEncounterComplete = true; ClearWorldBindings(); RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingReward; CreateRewardOffer(); BroadcastState();
}

void UPRRoomSubsystem::CreateRewardOffer()
{
	const UPRRoomDataAsset* Room = Registry ? Registry->FindRoom(RuntimeState.ActiveRoomId) : nullptr;
	const UPRRewardPolicyDataAsset* Policy = Room && Registry ? Registry->FindPolicy(Room->RewardPolicyId) : nullptr;
	if (!Policy) return;
	if (ActiveOffer.OfferId.IsValid() && !ActiveOffer.bResolved && ActiveOffer.Choices.Num() == 3) return;
	const bool bEpicWeightBoosted = bCurrentOfferEpicWeightBoosted;
	bCurrentOfferEpicWeightBoosted = false;
	ActiveOffer = FPRRewardOffer(); ActiveOffer.OfferId = FGuid::NewGuid(); RuntimeState.ActiveRewardOfferId = ActiveOffer.OfferId;
	TArray<const UPRRewardDataAsset*> Eligible;
	for (const FPrimaryAssetId& Id : Policy->RewardIds) if (const UPRRewardDataAsset* Reward = Registry->FindReward(Id)) if (IsRewardEligible(*Reward)) Eligible.Add(Reward);
	Eligible.Sort([](const UPRRewardDataAsset& A, const UPRRewardDataAsset& B) { return A.GetPrimaryAssetId().ToString() < B.GetPrimaryAssetId().ToString(); });
	uint32 Random = static_cast<uint32>(RuntimeState.Seed) ^ static_cast<uint32>(RuntimeState.CurrentStepIndex + 1) * 0x9E3779B9U;
	TSet<FName> OfferedFamilies;
	FGameplayTagContainer OfferedExclusions;
	while (Eligible.Num() && ActiveOffer.Choices.Num() < 3)
	{
		int32 TotalWeight = 0;
		for (const UPRRewardDataAsset* Candidate : Eligible) if (!OfferedFamilies.Contains(Candidate->FamilyId) && !Candidate->MutualExclusionTags.HasAny(OfferedExclusions)) TotalWeight += FPRRewardContract::GetRarityWeight(Candidate->RarityTag, Policy->CommonWeight, Policy->RareWeight, Policy->EpicWeight, bEpicWeightBoosted);
		if (TotalWeight <= 0) break;
		int32 Roll = static_cast<int32>(PRRoomRuntime::NextRandom(Random) % static_cast<uint32>(TotalWeight));
		int32 Index = INDEX_NONE;
		for (int32 CandidateIndex = 0; CandidateIndex < Eligible.Num(); ++CandidateIndex)
		{
			const UPRRewardDataAsset* Candidate = Eligible[CandidateIndex];
			if (OfferedFamilies.Contains(Candidate->FamilyId) || Candidate->MutualExclusionTags.HasAny(OfferedExclusions)) continue;
			const int32 Weight = FPRRewardContract::GetRarityWeight(Candidate->RarityTag, Policy->CommonWeight, Policy->RareWeight, Policy->EpicWeight, bEpicWeightBoosted);
			if (Roll < Weight) { Index = CandidateIndex; break; }
			Roll -= Weight;
		}
		if (Index == INDEX_NONE) break;
		const UPRRewardDataAsset* Reward = Eligible[Index]; Eligible.RemoveAt(Index); OfferedFamilies.Add(Reward->FamilyId); OfferedExclusions.AppendTags(Reward->MutualExclusionTags);
		FPRRewardOfferChoice& Choice = ActiveOffer.Choices.AddDefaulted_GetRef(); Choice.RewardId = Reward->GetPrimaryAssetId(); Choice.RarityTag = Reward->RarityTag; Choice.FamilyId = Reward->FamilyId; Choice.DisplayName = Reward->DisplayName; Choice.EffectText = Reward->EffectText; Choice.CostText = Reward->CostText;
	}
	if (ActiveOffer.Choices.Num() == 3) RewardOfferChanged.Broadcast(ActiveOffer);
	else { ActiveOffer = FPRRewardOffer(); RuntimeState.ActiveRewardOfferId.Invalidate(); RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; }
}

void UPRRoomSubsystem::CompleteSequence()
{
	if (RuntimeState.FlowStatus == EPRRoomFlowStatus::Completed) return;
	FPRRoomSequenceCompleted Completion; Completion.CompletionId = FGuid::NewGuid(); Completion.SessionId = RuntimeState.SessionId; Completion.Seed = RuntimeState.Seed; Completion.CompletedPath = RuntimeState.Path;
	for (const FPRRewardApplicationHandle& Handle : AppliedRewards) Completion.RewardIds.Add(Handle.RewardId);
	if (const UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr) { TArray<FPRAppliedDirectorRuleHandle> Rules; Director->GetAppliedRules(Rules); for (const FPRAppliedDirectorRuleHandle& Rule : Rules) Completion.DirectorRuleIds.Add(Rule.RuleId); }
	Completion.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	RuntimeState.FlowStatus = EPRRoomFlowStatus::Completed;
	SequenceCompleted.Broadcast(Completion);
	ClearWorldBindings();
	ClearSessionGameplayEffects();
	AppliedRewards.Reset();
	ActiveOffer = FPRRewardOffer();
	RuntimeState.ActiveRewardOfferId.Invalidate();
	BroadcastState();
}

UAbilitySystemComponent* UPRRoomSubsystem::ResolvePlayerAbilitySystem() const
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0); IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Pawn); return Interface ? Interface->GetAbilitySystemComponent() : nullptr;
}

bool UPRRoomSubsystem::RebindSessionGameplayEffects()
{
	if (AppliedRewards.IsEmpty()) return true;
	UAbilitySystemComponent* ASC = ResolvePlayerAbilitySystem();
	if (!ASC || !Registry) return false;
	TMap<FGuid, FActiveGameplayEffectHandle> ReboundHandles;
	for (const FPRRewardApplicationHandle& RewardHandle : AppliedRewards)
	{
		if (!RewardHandle.bPersistent) continue;
		const UPRRewardDataAsset* Reward = Registry->FindReward(RewardHandle.RewardId);
		if (!Reward) return false;
		FActiveGameplayEffectHandle EffectHandle;
		if (FPRGASRewardApplication::Apply(*ASC, Reward->EffectSpec, EffectHandle) != EPRRewardApplyResult::Applied) return false;
		ReboundHandles.Add(RewardHandle.HandleId, EffectHandle);
	}
	GameplayEffectHandles = MoveTemp(ReboundHandles);
	return true;
}

void UPRRoomSubsystem::ClearSessionGameplayEffects()
{
	if (UAbilitySystemComponent* ASC = ResolvePlayerAbilitySystem()) for (const TPair<FGuid, FActiveGameplayEffectHandle>& Pair : GameplayEffectHandles) FPRGASRewardApplication::Remove(*ASC, Pair.Value);
	GameplayEffectHandles.Reset();
}

void UPRRoomSubsystem::ClearWorldBindings()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(EncounterRetryTimer);
	GetWorld()->GetTimerManager().ClearTimer(EncounterCompletionTimer);
	if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>()) Combat->OnCombatEvent().Remove(CombatEventHandle);
	if (UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>()) { Enemies->OnEnemyStateChanged().Remove(EnemyStateChangedHandle); for (const FGuid& SpawnId : ActiveEncounterSpawnIds) Enemies->DespawnEnemy(SpawnId); }
	if (UPRBossSubsystem* Boss = GetWorld()->GetSubsystem<UPRBossSubsystem>()) Boss->OnPrototypeRunCompleted().Remove(BossCompletedHandle);
	if (UPRQTESubsystem* QTE = GetWorld()->GetSubsystem<UPRQTESubsystem>()) QTE->OnQTEResult().Remove(QTEResultHandle);
	EnemyStateChangedHandle.Reset(); BossCompletedHandle.Reset(); CombatEventHandle.Reset(); QTEResultHandle.Reset(); ActiveEncounterSpawnIds.Reset();
}

void UPRRoomSubsystem::BroadcastState() { StateChanged.Broadcast(RuntimeState); }
void UPRRoomSubsystem::ResetSession()
{
	ClearWorldBindings(); ClearSessionGameplayEffects(); RuntimeState = FPRRoomRuntimeState(); ActiveOffer = FPRRewardOffer(); AppliedRewards.Reset(); LastCombatEventId.Invalidate(); LastCombatEventTag = FGameplayTag(); LastQTEResultTag = FGameplayTag(); bCurrentOfferEpicWeightBoosted = false;
}
