// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRoomSubsystem.h"

#include "AbilitySystemInterface.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Auditor/PRAuditorChapterBoss.h"
#include "Chapters/Auditor/PRAuditorChapterBossComponent.h"
#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Chapters/Headmind/PRHeadmindProjectionBossComponent.h"
#include "Chapters/Allocator/PRAllocatorBoss.h"
#include "Chapters/Allocator/PRAllocatorBossComponent.h"
#include "Chapters/Pacifier/PRPacifierBoss.h"
#include "Chapters/Pacifier/PRPacifierBossComponent.h"
#include "Chapters/Warden/PRWardenBoss.h"
#include "Chapters/Warden/PRWardenBossComponent.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Core/PRPlayerState.h"
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
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "QTE/PRQTESubsystem.h"
#include "ProjectR.h"
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

int32 GetRewardResourceIndex(const UPRRewardDataAsset& Reward)
{
	switch (Reward.EffectSpec.Attribute)
	{
	case EPRRewardAttribute::Health:
	case EPRRewardAttribute::MaxHealth: return 0;
	case EPRRewardAttribute::Shield:
	case EPRRewardAttribute::MaxShield: return 1;
	case EPRRewardAttribute::Energy:
	case EPRRewardAttribute::MaxEnergy: return 2;
	default: return INDEX_NONE;
	}
}

void GetPlayerResourceRatios(const UObject* Context, float (&OutRatios)[3])
{
	OutRatios[0] = OutRatios[1] = OutRatios[2] = 1.0f;
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(Context, 0);
	const APRPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<APRPlayerState>() : nullptr;
	const UPRAttributeSet* Attributes = PlayerState ? PlayerState->GetAttributeSet() : nullptr;
	if (!Attributes) return;
	OutRatios[0] = Attributes->GetMaxHealth() > UE_SMALL_NUMBER ? FMath::Clamp(Attributes->GetHealth() / Attributes->GetMaxHealth(), 0.0f, 1.0f) : 1.0f;
	OutRatios[1] = Attributes->GetMaxShield() > UE_SMALL_NUMBER ? FMath::Clamp(Attributes->GetShield() / Attributes->GetMaxShield(), 0.0f, 1.0f) : 1.0f;
	OutRatios[2] = Attributes->GetMaxEnergy() > UE_SMALL_NUMBER ? FMath::Clamp(Attributes->GetEnergy() / Attributes->GetMaxEnergy(), 0.0f, 1.0f) : 1.0f;
}

FGameplayTag GetHeadmindSecondaryRuleId(const FName DirectiveId)
{
	if (DirectiveId == TEXT("Headmind.ObediencePrediction")) return FixedTag(TEXT("Rule.PredictionLock"));
	if (DirectiveId == TEXT("Headmind.RepetitionOptimality")) return FixedTag(TEXT("Rule.OptimalPath"));
	if (DirectiveId == TEXT("Headmind.IsolationCooperation")) return FixedTag(TEXT("Rule.CooperationAudit"));
	if (DirectiveId == TEXT("Headmind.SurvivalRisk")) return FixedTag(TEXT("Rule.RiskReward"));
	if (DirectiveId == TEXT("Headmind.ResourceDistance")) return FixedTag(TEXT("Rule.DistanceCorrection"));
	return FGameplayTag();
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
	ConfiguredRegistryId = FPrimaryAssetId();
	ConfiguredContentId = FName();
	ActiveChapterDirectiveId = FName();
	ActiveChapterPressure = 0;
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
	if (const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry))
	{
		ActiveChapterDirectiveId = UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(ChapterRegistry->ContentId, Seed);
		if (!ChapterRegistry->IsKnownDirective(ActiveChapterDirectiveId)) { ResetSession(); return EPRRoomOperationResult::NotReady; }
		ActiveChapterPressure = 0;
		bChapterDirectiveValidated = IsValidatedChapterDirective(ChapterDirectiveLevel);
		if ((ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
				|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
				|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
				|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId())
			&& !bChapterDirectiveValidated)
		{
			RuntimeState.ChapterOfferFallbackReason =
				ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
					? TEXT("Pacifier.DirectorNeutralFallback")
					: ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
						? TEXT("Auditor.DirectorNeutralFallback")
						: ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId()
							? TEXT("Headmind.DirectiveFusionUnavailable")
							: TEXT("Warden.DirectorNeutralFallback");
		}
	}
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
	const UPRChapterRoguelikeContentRegistryDataAsset* ActiveChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry);
	if (ActiveChapterRegistry
		&& ActiveChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId()
		&& !IsRelationshipDeltaEmpty(Choice->RelationshipDelta))
	{
		// Headmind events are transient synthesis-pressure choices.  A malformed
		// DataAsset must not turn them into a relationship or Save write seam.
		return EPRRoomOperationResult::Rejected;
	}
	if (Choice->bRequiresQTESuccess && LastQTEResultTag != UPRTagLibrary::GetQTEResultSuccessTag()) return EPRRoomOperationResult::Rejected;
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	if (!IsRelationshipDeltaEmpty(Choice->RelationshipDelta) && (!Save || !Save->GetSaveRuntimeState().bHasLoadedProfile)) return EPRRoomOperationResult::Rejected;
	FPRRoomEventResult Result;
	Result.ResolutionId = FGuid::NewGuid(); Result.RoomId = RuntimeState.ActiveRoomId; Result.EventId = EventId; Result.ChoiceId = ChoiceId; Result.bEpicWeightBoosted = Choice->bBoostEpicWeight;
	if (const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = ActiveChapterRegistry)
	{
		FPRChapterEventPressureBinding Binding;
		if (!ChapterRegistry->FindEventPressureBinding(EventId, ChoiceId, Binding)) return EPRRoomOperationResult::Rejected;
		ActiveChapterPressure = FMath::Clamp(ActiveChapterPressure + Binding.PressureDelta, 0, 4);
		if (ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
			|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
			|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
			|| ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId())
		{
			ApplyChapterRouteConstraint(Binding);
			// Warden events are chapter-local choices: accepting one is observable, but never writes relationship state.
			Result.bChoiceApplied = true;
		}
	}
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

EPRRoomContentResult UPRRoomSubsystem::ConfigureContentRegistry(const FPrimaryAssetId RegistryId)
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::Idle && RuntimeState.FlowStatus != EPRRoomFlowStatus::Completed && RuntimeState.FlowStatus != EPRRoomFlowStatus::Cancelled) return EPRRoomContentResult::Busy;
	if (!RegistryId.IsValid() || RegistryId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRChapterRoguelikeRegistry"))) return EPRRoomContentResult::InvalidRegistry;
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(RegistryId);
	UPRChapterRoguelikeContentRegistryDataAsset* Candidate = Path.IsValid() ? Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Path.TryLoad()) : nullptr;
	if (!Candidate && RegistryId == UPRChapterContentRegistryDataAsset::GetWardenRoomRegistryId())
	{
		// Exact v0.6.1 compatibility fallback; no caller-controlled object path.
		Candidate = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_RoguelikeContentRegistry_Warden.DA_RoguelikeContentRegistry_Warden"));
	}
	if (!Candidate && RegistryId == UPRChapterContentRegistryDataAsset::GetPacifierRoomRegistryId())
	{
		Candidate = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Pacifier/DA_RoguelikeContentRegistry_Pacifier.DA_RoguelikeContentRegistry_Pacifier"));
	}
	if (!Candidate && RegistryId == UPRChapterContentRegistryDataAsset::GetAuditorRoomRegistryId())
	{
		Candidate = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Auditor/DA_RoguelikeContentRegistry_Auditor.DA_RoguelikeContentRegistry_Auditor"));
	}
	if (!Candidate && RegistryId == UPRChapterContentRegistryDataAsset::GetHeadmindRoomRegistryId())
	{
		Candidate = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Headmind/DA_RoguelikeContentRegistry_Headmind.DA_RoguelikeContentRegistry_Headmind"));
	}
	if (!Candidate) return EPRRoomContentResult::NotFound;
	if (!Candidate->IsRegistryReady()) return EPRRoomContentResult::InvalidRegistry;
	RegistryAsset = Candidate;
	Registry = Candidate;
	ConfiguredRegistryId = RegistryId;
	ConfiguredContentId = Candidate->ContentId;
	ActiveChapterDirectiveId = FName();
	ActiveChapterPressure = 0;
	bChapterDirectiveValidated = false;
	ChapterDirectiveLevel = 1;
	return EPRRoomContentResult::Succeeded;
}

FPrimaryAssetId UPRRoomSubsystem::GetConfiguredContentRegistryId() const
{
	return ConfiguredRegistryId;
}

EPRRoomContentResult UPRRoomSubsystem::ConfigureContentContext(const FName ContentId, const FName ChapterDirectiveId, const int32 AllocationPressure)
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::Idle && RuntimeState.FlowStatus != EPRRoomFlowStatus::Completed && RuntimeState.FlowStatus != EPRRoomFlowStatus::Cancelled) return EPRRoomContentResult::Busy;
	const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry);
	if (!ChapterRegistry || ConfiguredRegistryId != ChapterRegistry->GetPrimaryAssetId() || ContentId != ChapterRegistry->ContentId || !ChapterRegistry->IsKnownDirective(ChapterDirectiveId)) return EPRRoomContentResult::RejectedContext;
	ConfiguredContentId = ContentId;
	ActiveChapterDirectiveId = ChapterDirectiveId;
	ActiveChapterPressure = FMath::Clamp(AllocationPressure, 0, 4);
	bChapterDirectiveValidated = false;
	ChapterDirectiveLevel = 1;
	return EPRRoomContentResult::Succeeded;
}

bool UPRRoomSubsystem::GetRoomRuntimeState(FPRRoomRuntimeState& OutState) const { OutState = RuntimeState; return true; }
bool UPRRoomSubsystem::GetExpectedBossSpawnId(FGuid& OutSpawnId) const { OutSpawnId = ExpectedBossSpawnId; return OutSpawnId.IsValid(); }
FName UPRRoomSubsystem::GetChapterRouteFallbackReason() const { return RuntimeState.ChapterOfferFallbackReason; }
void UPRRoomSubsystem::GetActiveEncounterSpawnIds(TArray<FGuid>& OutSpawnIds) const { OutSpawnIds = ActiveEncounterSpawnIds; }
void UPRRoomSubsystem::GetAppliedRewards(TArray<FPRRewardApplicationHandle>& OutHandles) const { OutHandles = AppliedRewards; }
void UPRRoomSubsystem::GetAppliedRewardSnapshots(TArray<FPRAppliedRewardSnapshot>& OutRewards) const
{
	OutRewards.Reset();
	for (const FPRRewardApplicationHandle& Handle : AppliedRewards)
	{
		if (const UPRRewardDataAsset* Reward = Registry ? Registry->FindReward(Handle.RewardId) : nullptr)
		{
			FPRAppliedRewardSnapshot& Snapshot = OutRewards.AddDefaulted_GetRef();
			Snapshot.RewardId = Handle.RewardId;
			Snapshot.FamilyId = Handle.FamilyId;
			Snapshot.Tier = Handle.Tier;
			Snapshot.EffectSpec = Reward->EffectSpec;
		}
	}
	OutRewards.Sort([](const FPRAppliedRewardSnapshot& A, const FPRAppliedRewardSnapshot& B) { return A.RewardId.ToString() < B.RewardId.ToString(); });
}
FPRRoomStateChangedNative& UPRRoomSubsystem::OnRoomStateChanged() { return StateChanged; }
FPRRewardOfferChangedNative& UPRRoomSubsystem::OnRewardOfferChanged() { return RewardOfferChanged; }
FPRRoomEventResolvedNative& UPRRoomSubsystem::OnRoomEventResolved() { return EventResolved; }
FPRRoomSequenceCompletedNative& UPRRoomSubsystem::OnRoomSequenceCompleted() { return SequenceCompleted; }

bool UPRRoomSubsystem::BuildPath()
{
	TArray<const UPRRoomDataAsset*> Rooms;
	const bool bChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry) != nullptr;
	for (const TSoftObjectPtr<UPRRoomDataAsset>& Reference : Registry->Rooms)
	{
		if (const UPRRoomDataAsset* Room = Reference.LoadSynchronous())
		{
			if (IsRoomEligible(*Room) && (bChapterRegistry || !PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Shop")))) Rooms.Add(Room);
		}
	}
	Rooms.Sort([](const UPRRoomDataAsset& A, const UPRRoomDataAsset& B) { return A.GetPrimaryAssetId().ToString() < B.GetPrimaryAssetId().ToString(); });
	if (Rooms.Num() < 5) return false;
	const TCHAR* RequiredTypes[] = { TEXT("Room.Type.Combat"), TEXT("Room.Type.Event"), bChapterRegistry ? TEXT("Room.Type.Shop") : TEXT("Room.Type.Safe"), TEXT("Room.Type.Safe") };
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
	if ((ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
			|| ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
			|| ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
			|| ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId())
		&& bChapterDirectiveValidated)
	{
		const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry);
		const UPRChapterRuleDataAsset* Rule = ChapterRegistry ? ChapterRegistry->FindChapterRule(ActiveChapterDirectiveId) : nullptr;
		if (Rule && Rule->PreferredRoomIds.Contains(Room.GetPrimaryAssetId())) Weight *= 1 + FMath::Clamp(ChapterDirectiveLevel, 1, 5);
	}
	return FMath::Max(0, Weight);
}

bool UPRRoomSubsystem::ApplyChapterRouteConstraint(const FPRChapterEventPressureBinding& Binding)
{
	if (Binding.ExcludedFutureRoomIds.IsEmpty()) return true;
	TArray<FPRRoomPathStep> CandidatePath = RuntimeState.Path;
	for (int32 Index = RuntimeState.CurrentStepIndex + 1; Index < CandidatePath.Num(); ++Index)
	{
		FPRRoomPathStep& Step = CandidatePath[Index];
		if (Step.SelectedRoomId.IsValid()) continue;
		Step.CandidateRoomIds.RemoveAll([&Binding](const FPrimaryAssetId& Id) { return Binding.ExcludedFutureRoomIds.Contains(Id); });
		if (Step.CandidateRoomIds.IsEmpty())
		{
			RuntimeState.ChapterOfferFallbackReason =
				ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
					? TEXT("Pacifier.ConstraintNoEligibleAlternative")
					: ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
						? TEXT("Auditor.ConstraintNoEligibleAlternative")
						: ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId()
							? TEXT("Headmind.ConstraintNoEligibleAlternative")
							: TEXT("Warden.RouteConstraintNoEligibleAlternative");
			return false;
		}
	}
	RuntimeState.Path = MoveTemp(CandidatePath);
	return true;
}

bool UPRRoomSubsystem::IsValidatedChapterDirective(int32& OutLevel) const
{
	OutLevel = 1;
	if (ConfiguredContentId != UPRChapterContentRegistryDataAsset::GetWardenContentId()
		&& ConfiguredContentId != UPRChapterContentRegistryDataAsset::GetPacifierContentId()
		&& ConfiguredContentId != UPRChapterContentRegistryDataAsset::GetAuditorContentId()
		&& ConfiguredContentId != UPRChapterContentRegistryDataAsset::GetHeadmindContentId()) return false;
	const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry);
	const UPRChapterRuleDataAsset* Rule = ChapterRegistry ? ChapterRegistry->FindChapterRule(ActiveChapterDirectiveId) : nullptr;
	const UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
	if (!Rule || !Director || !Rule->RequiredDirectorRuleId.IsValid()) return false;
	TArray<FPRAppliedDirectorRuleHandle> AppliedRules;
	if (!Director->GetAppliedRules(AppliedRules)) return false;
	const FPRAppliedDirectorRuleHandle* Match = nullptr;
	for (const FPRAppliedDirectorRuleHandle& Handle : AppliedRules)
	{
		if (Handle.RuleId != Rule->RequiredDirectorRuleId) continue;
		if (!Handle.HandleId.IsValid() || Handle.Level < 1 || Handle.Level > 5 || Match) return false;
		Match = &Handle;
	}
	if (!Match) return false;
	FPRDirectorRuleRuntimeState Runtime;
	if (!Director->GetRuleRuntimeState(Match->RuleId, Runtime)
		|| Runtime.Level != Match->Level
		|| (Runtime.Status != EPRDirectorRuleRuntimeStatus::Active && Runtime.Status != EPRDirectorRuleRuntimeStatus::Degraded)) return false;
	OutLevel = Match->Level;
	if (ConfiguredContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId())
	{
		const FGameplayTag SecondaryRuleId = PRRoomRuntime::GetHeadmindSecondaryRuleId(ActiveChapterDirectiveId);
		if (!SecondaryRuleId.IsValid()) return false;
		const FPRAppliedDirectorRuleHandle* SecondaryMatch = nullptr;
		for (const FPRAppliedDirectorRuleHandle& Handle : AppliedRules)
		{
			if (Handle.RuleId != SecondaryRuleId) continue;
			if (!Handle.HandleId.IsValid() || Handle.Level < 1 || Handle.Level > 5 || SecondaryMatch) return false;
			SecondaryMatch = &Handle;
		}
		FPRDirectorRuleRuntimeState SecondaryRuntime;
		if (!SecondaryMatch || !Director->GetRuleRuntimeState(SecondaryRuleId, SecondaryRuntime)
			|| SecondaryRuntime.Level != SecondaryMatch->Level
			|| (SecondaryRuntime.Status != EPRDirectorRuleRuntimeStatus::Active && SecondaryRuntime.Status != EPRDirectorRuleRuntimeStatus::Degraded)) return false;
		OutLevel = FMath::Min(OutLevel, SecondaryMatch->Level);
	}
	return true;
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
	if (PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Safe")) || PRRoomRuntime::HasType(*Room, TEXT("Room.Type.Shop"))) { RuntimeState.FlowStatus = EPRRoomFlowStatus::SelectingReward; CreateRewardOffer(); BroadcastState(); return; }
	const UPREncounterDataAsset* Encounter = Registry->FindEncounter(Room->EncounterId);
	if (!Encounter || !GetWorld()) { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>()) CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRRoomSubsystem::HandleCombatEvent);
	UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>();
	if (!Enemies || !Enemies->IsRegistryReady())
	{
		GetWorld()->GetTimerManager().SetTimer(EncounterRetryTimer, this, &UPRRoomSubsystem::StartEncounter, 0.1f, false);
		return;
	}
	EnemyStateChangedHandle = Enemies->OnEnemyStateChanged().AddUObject(this, &UPRRoomSubsystem::HandleEnemyStateChanged);
	if (Encounter->Kind == EPRRoomEncounterKind::Boss)
	{
		if (UPRBossSubsystem* Boss = GetWorld()->GetSubsystem<UPRBossSubsystem>()) BossCompletedHandle = Boss->OnPrototypeRunCompleted().AddUObject(this, &UPRRoomSubsystem::HandleBossCompleted);
		else { RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled; BroadcastState(); return; }
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Origin = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	for (const FPREncounterSpawnDefinition& Spawn : Encounter->SpawnDefinitions)
	{
		FGuid SpawnId; class APREnemyCharacter* Enemy = nullptr;
		const FTransform SpawnTransform(FRotator::ZeroRotator, Origin + Spawn.RelativeLocation);
		const EPREnemySpawnStatus SpawnResult = Spawn.PrototypeId.IsValid()
			? Enemies->SpawnEnemyPrototype(Spawn.PrototypeId, SpawnTransform, SpawnId, Enemy)
			: Enemies->SpawnEnemyPrototype(Spawn.PrototypeTag, SpawnTransform, SpawnId, Enemy);
		if (SpawnResult == EPREnemySpawnStatus::Spawned)
		{
			ActiveEncounterSpawnIds.Add(SpawnId);
			if (Encounter->Kind == EPRRoomEncounterKind::Boss
				&& (Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetAllocatorBossPrototypeId()
					|| Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetWardenBossPrototypeId()
					|| Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetPacifierBossPrototypeId()
					|| Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetAuditorBossPrototypeId()
					|| Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetHeadmindBossPrototypeId()))
			{
				ExpectedBossSpawnId = SpawnId;
				if (Spawn.PrototypeId == UPRChapterContentRegistryDataAsset::GetAllocatorBossPrototypeId())
				{
					if (APRAllocatorBoss* Allocator = Cast<APRAllocatorBoss>(Enemy))
					{
						TArray<FPRAppliedRewardSnapshot> AppliedSnapshots;
						GetAppliedRewardSnapshots(AppliedSnapshots);
						Allocator->GetAllocatorBossComponent()->ConfigureChapterState(ActiveChapterPressure, AppliedSnapshots);
					}
				}
				else if (APRWardenBoss* Warden = Cast<APRWardenBoss>(Enemy))
				{
					Warden->GetWardenBossComponent()->ConfigureChapterState(ActiveChapterPressure);
				}
				else if (APRPacifierBoss* Pacifier = Cast<APRPacifierBoss>(Enemy))
				{
					Pacifier->GetPacifierBossComponent()->ConfigureChapterState(ActiveChapterPressure);
				}
				else if (APRAuditorChapterBoss* Auditor = Cast<APRAuditorChapterBoss>(Enemy))
				{
					Auditor->GetAuditorChapterBossComponent()->ConfigureChapterState(ActiveChapterPressure);
				}
				else if (APRHeadmindProjectionBoss* Headmind = Cast<APRHeadmindProjectionBoss>(Enemy))
				{
					const FGameplayTag PrimaryRule = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry) && Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry)->FindChapterRule(ActiveChapterDirectiveId)
						? Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry)->FindChapterRule(ActiveChapterDirectiveId)->RequiredDirectorRuleId : FGameplayTag();
					Headmind->GetHeadmindProjectionBossComponent()->ConfigureChapterState(ActiveChapterPressure, PrimaryRule, PRRoomRuntime::GetHeadmindSecondaryRuleId(ActiveChapterDirectiveId), bChapterDirectiveValidated);
				}
			}
		}
	}
	if (ActiveEncounterSpawnIds.IsEmpty() || (Encounter->Kind == EPRRoomEncounterKind::Boss && !ExpectedBossSpawnId.IsValid()))
	{
		UE_LOG(LogProjectR, Warning, TEXT("Room encounter rejected after closed-registry spawn (room=%s,kind=%d,spawnCount=%d,expectedBoss=%s)."), *RuntimeState.ActiveRoomId.ToString(), static_cast<int32>(Encounter->Kind), ActiveEncounterSpawnIds.Num(), ExpectedBossSpawnId.IsValid() ? TEXT("true") : TEXT("false"));
		RuntimeState.FlowStatus = EPRRoomFlowStatus::Cancelled;
		BroadcastState();
		return;
	}
	RuntimeState.FlowStatus = EPRRoomFlowStatus::EncounterActive;
	GetWorld()->GetTimerManager().SetTimer(EncounterCompletionTimer, this, &UPRRoomSubsystem::CheckEncounterCompletion, 0.1f, true);
	BroadcastState();
}

void UPRRoomSubsystem::CheckEncounterCompletion()
{
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::EncounterActive || !GetWorld()) return;
	UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>();
	if (!Enemies || ActiveEncounterSpawnIds.IsEmpty()) return;
	const UPRRoomDataAsset* Room = Registry ? Registry->FindRoom(RuntimeState.ActiveRoomId) : nullptr;
	const UPREncounterDataAsset* Encounter = Room ? Registry->FindEncounter(Room->EncounterId) : nullptr;
	if (Encounter && Encounter->Kind == EPRRoomEncounterKind::Boss && !bExpectedBossCompletionReceived) return;
	for (const FGuid& SpawnId : ActiveEncounterSpawnIds) { FPREnemyRuntimeState State; if (Enemies->GetEnemyRuntimeState(SpawnId, State) && State.bAlive) return; }
	CompleteEncounter();
}

void UPRRoomSubsystem::HandleEnemyStateChanged(const FPREnemyRuntimeState& State)
{
	if (!ActiveEncounterSpawnIds.Contains(State.SpawnId) || State.bAlive) return;
	const UPRRoomDataAsset* Room = Registry ? Registry->FindRoom(RuntimeState.ActiveRoomId) : nullptr;
	const UPREncounterDataAsset* Encounter = Room ? Registry->FindEncounter(Room->EncounterId) : nullptr;
	if (Encounter && Encounter->Kind == EPRRoomEncounterKind::Boss && !bExpectedBossCompletionReceived) return;
	for (const FGuid& SpawnId : ActiveEncounterSpawnIds)
	{
		FPREnemyRuntimeState Existing;
		if (GetWorld() && GetWorld()->GetSubsystem<UPREnemySubsystem>()->GetEnemyRuntimeState(SpawnId, Existing) && Existing.bAlive) return;
	}
	CompleteEncounter();
}

void UPRRoomSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Result)
{
	const UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Registry);
	const FName ExpectedBossId =
		ChapterRegistry && ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetWardenContentId()
			? UPRChapterContentRegistryDataAsset::GetWardenBossId()
			: ChapterRegistry && ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetPacifierContentId()
				? UPRChapterContentRegistryDataAsset::GetPacifierBossId()
				: ChapterRegistry && ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetAuditorContentId()
					? UPRChapterContentRegistryDataAsset::GetAuditorBossId()
					: ChapterRegistry && ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId()
						? UPRChapterContentRegistryDataAsset::GetHeadmindBossId()
						: UPRChapterContentRegistryDataAsset::GetAllocatorBossId();
	bool bMatchesExpectedBoss = Result.BossId == ExpectedBossId;
	if (!bMatchesExpectedBoss
		&& ChapterRegistry && ChapterRegistry->ContentId == UPRChapterContentRegistryDataAsset::GetHeadmindContentId()
		&& Result.BossId == UPRChapterContentRegistryDataAsset::GetAuditorBossId()
		&& GetWorld())
	{
		// APRBossAuditor's prototype completion identity is frozen as "Auditor".
		// Accept it only for the exact closed Headmind projection actor and its
		// currently expected spawn; no other legacy boss result crosses this seam.
		APREnemyCharacter* SpawnedEnemy = nullptr;
		if (UPREnemySubsystem* Enemies = GetWorld()->GetSubsystem<UPREnemySubsystem>())
		{
			bMatchesExpectedBoss = Enemies->ResolveSpawnedEnemy(Result.BossSpawnId, SpawnedEnemy)
				&& Cast<APRHeadmindProjectionBoss>(SpawnedEnemy) != nullptr;
		}
	}
	if (RuntimeState.FlowStatus != EPRRoomFlowStatus::EncounterActive || !bMatchesExpectedBoss || Result.BossSpawnId != ExpectedBossSpawnId || bExpectedBossCompletionReceived) return;
	bExpectedBossCompletionReceived = true;
	// Boss components publish their completion from the health-change delegate.
	// Defer despawn eligibility until Combat has finished the same fatal-damage
	// call and applied State.Dead to the boss ASC.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UPRRoomSubsystem::CheckEncounterCompletion);
	}
}
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
	RuntimeState.ChapterOfferFallbackReason = FName();
	TArray<const UPRRewardDataAsset*> Eligible;
	for (const FPrimaryAssetId& Id : Policy->RewardIds) if (const UPRRewardDataAsset* Reward = Registry->FindReward(Id)) if (IsRewardEligible(*Reward)) Eligible.Add(Reward);
	Eligible.Sort([](const UPRRewardDataAsset& A, const UPRRewardDataAsset& B) { return A.GetPrimaryAssetId().ToString() < B.GetPrimaryAssetId().ToString(); });
	uint32 Random = static_cast<uint32>(RuntimeState.Seed) ^ static_cast<uint32>(RuntimeState.CurrentStepIndex + 1) * 0x9E3779B9U;
	float ResourceRatios[3];
	PRRoomRuntime::GetPlayerResourceRatios(this, ResourceRatios);
	const UPRRoomDataAsset* ActiveRoom = Registry ? Registry->FindRoom(RuntimeState.ActiveRoomId) : nullptr;
	const bool bShop = ActiveRoom && PRRoomRuntime::HasType(*ActiveRoom, TEXT("Room.Type.Shop"));
	int32 HighestResource = 0;
	int32 LowestResource = 0;
	for (int32 Index = 1; Index < 3; ++Index)
	{
		if (ResourceRatios[Index] > ResourceRatios[HighestResource]) HighestResource = Index;
		if (ResourceRatios[Index] < ResourceRatios[LowestResource]) LowestResource = Index;
	}
	int32 ForcedResource = ActiveChapterDirectiveId == TEXT("Allocator.EqualizationQuota") ? LowestResource : INDEX_NONE;
	bool bUseDirective = true;
	TSet<FName> OfferedFamilies;
	FGameplayTagContainer OfferedExclusions;
	while (Eligible.Num() && ActiveOffer.Choices.Num() < 3)
	{
		int32 TotalWeight = 0;
		for (const UPRRewardDataAsset* Candidate : Eligible) if (!OfferedFamilies.Contains(Candidate->FamilyId) && !Candidate->MutualExclusionTags.HasAny(OfferedExclusions))
		{
			if (ForcedResource != INDEX_NONE && ActiveOffer.Choices.IsEmpty() && PRRoomRuntime::GetRewardResourceIndex(*Candidate) != ForcedResource) continue;
			int32 Weight = FPRRewardContract::GetRarityWeight(Candidate->RarityTag, Policy->CommonWeight, Policy->RareWeight, Policy->EpicWeight, bEpicWeightBoosted);
			const int32 ResourceIndex = PRRoomRuntime::GetRewardResourceIndex(*Candidate);
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.ResourceLock") && bShop && ResourceIndex == HighestResource) Weight = 0;
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.RewardWithholding") && Candidate->RarityTag.ToString() == TEXT("Reward.Rarity.Epic")) Weight = FMath::Max(1, Weight / 2);
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.ScarcityMarkup") && ResourceIndex != INDEX_NONE && ResourceRatios[ResourceIndex] < 0.35f) Weight = FMath::Max(1, Weight / 2);
			TotalWeight += Weight;
		}
		if (TotalWeight <= 0)
		{
			if (ForcedResource != INDEX_NONE) { ForcedResource = INDEX_NONE; RuntimeState.ChapterOfferFallbackReason = TEXT("EqualizationNoEligibleRecovery"); continue; }
			if (bUseDirective) { bUseDirective = false; RuntimeState.ChapterOfferFallbackReason = TEXT("ChapterOfferBaselineFallback"); continue; }
			break;
		}
		int32 Roll = static_cast<int32>(PRRoomRuntime::NextRandom(Random) % static_cast<uint32>(TotalWeight));
		int32 Index = INDEX_NONE;
		for (int32 CandidateIndex = 0; CandidateIndex < Eligible.Num(); ++CandidateIndex)
		{
			const UPRRewardDataAsset* Candidate = Eligible[CandidateIndex];
			if (OfferedFamilies.Contains(Candidate->FamilyId) || Candidate->MutualExclusionTags.HasAny(OfferedExclusions)) continue;
			if (ForcedResource != INDEX_NONE && ActiveOffer.Choices.IsEmpty() && PRRoomRuntime::GetRewardResourceIndex(*Candidate) != ForcedResource) continue;
			int32 Weight = FPRRewardContract::GetRarityWeight(Candidate->RarityTag, Policy->CommonWeight, Policy->RareWeight, Policy->EpicWeight, bEpicWeightBoosted);
			const int32 ResourceIndex = PRRoomRuntime::GetRewardResourceIndex(*Candidate);
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.ResourceLock") && bShop && ResourceIndex == HighestResource) Weight = 0;
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.RewardWithholding") && Candidate->RarityTag.ToString() == TEXT("Reward.Rarity.Epic")) Weight = FMath::Max(1, Weight / 2);
			if (bUseDirective && ActiveChapterDirectiveId == TEXT("Allocator.ScarcityMarkup") && ResourceIndex != INDEX_NONE && ResourceRatios[ResourceIndex] < 0.35f) Weight = FMath::Max(1, Weight / 2);
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
	ClearWorldBindings(); ClearSessionGameplayEffects(); RuntimeState = FPRRoomRuntimeState(); ActiveOffer = FPRRewardOffer(); AppliedRewards.Reset(); LastCombatEventId.Invalidate(); LastCombatEventTag = FGameplayTag(); LastQTEResultTag = FGameplayTag(); bCurrentOfferEpicWeightBoosted = false; bExpectedBossCompletionReceived = false; ExpectedBossSpawnId.Invalidate();
}
