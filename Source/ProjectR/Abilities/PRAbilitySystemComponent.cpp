// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/PRAbilitySystemComponent.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/PRGameplayAbility.h"
#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "ProjectR.h"

UPRAbilitySystemComponent::UPRAbilitySystemComponent()
{
	AbilityEndedHandle = OnAbilityEnded.AddUObject(this, &UPRAbilitySystemComponent::HandleAbilityEnded);
}

EPRAbilitySetOperationStatus UPRAbilitySystemComponent::GrantAbilitySet(
	const UPRAbilitySetDataAsset* AbilitySet,
	const EPRAbilitySetGrantMode GrantMode,
	FPRAbilitySetGrantHandle& OutHandle)
{
	OutHandle = FPRAbilitySetGrantHandle();
	if (!AbilitySet || !ValidateAbilitySetForGrant(AbilitySet))
	{
		return EPRAbilitySetOperationStatus::Invalid;
	}
	if (!IsOwnerActorAuthoritative())
	{
		return EPRAbilitySetOperationStatus::NotAuthoritative;
	}

	FGuid ExistingGrantId;
	FGrantRecord* ExistingRecord = nullptr;
	for (TPair<FGuid, FGrantRecord>& Pair : GrantRecords)
	{
		if (Pair.Value.AbilitySet.Get() == AbilitySet)
		{
			ExistingGrantId = Pair.Key;
			ExistingRecord = &Pair.Value;
			break;
		}
	}
	if (ExistingRecord
		&& (ExistingRecord->GrantMode == EPRAbilitySetGrantMode::AllEntries
			|| ExistingRecord->GrantMode == GrantMode))
	{
		OutHandle.GrantId = ExistingGrantId;
		OutHandle.AbilitySet = const_cast<UPRAbilitySetDataAsset*>(AbilitySet);
		OutHandle.AbilitySpecHandles = ExistingRecord->AbilitySpecHandles;
		return EPRAbilitySetOperationStatus::AlreadyApplied;
	}

	const TArray<FPRAbilitySetEntry>& Entries = AbilitySet->GetAbilityEntries();
	TArray<int32> EntryIndices;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (ExistingRecord && ExistingRecord->GrantedEntryIndices.Contains(Index))
		{
			continue;
		}
		if (GrantMode == EPRAbilitySetGrantMode::InitializationOnly
			&& !Entries[Index].bGrantOnInitialization)
		{
			continue;
		}
		if (DoesSpecConflict(Entries[Index]))
		{
			UE_LOG(LogProjectR, Error,
				TEXT("AbilitySet %s conflicts with an existing AbilityTag or InputTag."),
				*AbilitySet->GetPathName());
			return EPRAbilitySetOperationStatus::Invalid;
		}
		EntryIndices.Add(Index);
	}

	TArray<FGameplayAbilitySpecHandle> NewHandles;
	for (const int32 Index : EntryIndices)
	{
		const FPRAbilitySetEntry& Entry = Entries[Index];
		UObject* SourceObject = Entry.AbilityData
			? static_cast<UObject*>(Entry.AbilityData.Get())
			: const_cast<UPRAbilitySetDataAsset*>(AbilitySet);
		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.AbilityLevel, INDEX_NONE, SourceObject);
		const UPRGameplayAbility* AbilityCDO = Entry.AbilityClass->GetDefaultObject<UPRGameplayAbility>();
		Spec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->GetProjectRAbilityTag());
		if (Entry.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Entry.InputTag);
		}
		Spec.GetDynamicSpecSourceTags().AppendTags(Entry.GrantedSpecTags);
		const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);
		if (!Handle.IsValid() || !FindAbilitySpecFromHandle(Handle))
		{
			RollBackGrantedSpecs(NewHandles);
			UE_LOG(LogProjectR, Error,
				TEXT("AbilitySet %s failed to grant entry %d."), *AbilitySet->GetPathName(), Index);
			return EPRAbilitySetOperationStatus::Invalid;
		}
		NewHandles.Add(Handle);
	}

	const FGuid GrantId = ExistingRecord ? ExistingGrantId : FGuid::NewGuid();
	FGrantRecord& Record = ExistingRecord ? *ExistingRecord : GrantRecords.Add(GrantId);
	Record.AbilitySet = const_cast<UPRAbilitySetDataAsset*>(AbilitySet);
	Record.GrantMode = GrantMode;
	for (int32 ArrayIndex = 0; ArrayIndex < EntryIndices.Num(); ++ArrayIndex)
	{
		Record.GrantedEntryIndices.Add(EntryIndices[ArrayIndex]);
		Record.AbilitySpecHandles.Add(NewHandles[ArrayIndex]);
	}

	OutHandle.GrantId = GrantId;
	OutHandle.AbilitySet = const_cast<UPRAbilitySetDataAsset*>(AbilitySet);
	OutHandle.AbilitySpecHandles = Record.AbilitySpecHandles;
	for (const FGameplayAbilitySpecHandle Handle : NewHandles)
	{
		BroadcastLifecycle(EPRAbilityLifecycleEventType::Granted, Handle);
	}
	ActivatePassiveAbilities();
	return EPRAbilitySetOperationStatus::Applied;
}

EPRAbilitySetOperationStatus UPRAbilitySystemComponent::RemoveAbilitySet(
	const FPRAbilitySetGrantHandle& Handle)
{
	if (!IsOwnerActorAuthoritative())
	{
		return EPRAbilitySetOperationStatus::NotAuthoritative;
	}
	FGrantRecord* Record = GrantRecords.Find(Handle.GrantId);
	if (!Record || Record->AbilitySet != Handle.AbilitySet)
	{
		return EPRAbilitySetOperationStatus::NotFound;
	}

	const TArray<FGameplayAbilitySpecHandle> Handles = Record->AbilitySpecHandles;
	for (const FGameplayAbilitySpecHandle SpecHandle : Handles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (HeldAbilityHandles.Remove(SpecHandle) > 0)
			{
				Spec->InputPressed = false;
				AbilitySpecInputReleased(*Spec);
			}
			if (Spec->IsActive())
			{
				CancelAbilityHandle(SpecHandle);
			}
			FPRAbilityRuntimeState RemovedState;
			BuildRuntimeState(*Spec, RemovedState);
			ClearAbility(SpecHandle);
			FPRAbilityLifecycleEvent Event;
			Event.EventType = EPRAbilityLifecycleEventType::Removed;
			Event.AbilityState = RemovedState;
			Event.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			AbilityLifecycleEvent.Broadcast(Event);
		}
	}
	GrantRecords.Remove(Handle.GrantId);
	return EPRAbilitySetOperationStatus::Removed;
}

void UPRAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	if (!InputTag.IsValid() || !InputTag.ToString().StartsWith(TEXT("Input.")))
	{
		if (InputTag.IsValid())
		{
			UE_LOG(LogProjectR, Error, TEXT("ASC rejected non-Input tag %s."), *InputTag.ToString());
		}
		return;
	}
	const TArray<FGameplayAbilitySpecHandle> Matches = FindSpecsByExactDynamicTag(InputTag);
	if (Matches.Num() > 1)
	{
		UE_LOG(LogProjectR, Error, TEXT("InputTag %s matches multiple AbilitySpecs."), *InputTag.ToString());
		return;
	}
	if (Matches.IsEmpty() || HeldAbilityHandles.Contains(Matches[0]))
	{
		return;
	}

	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Matches[0]);
	const UPRGameplayAbility* AbilityCDO = Spec
		? Cast<UPRGameplayAbility>(Spec->Ability)
		: nullptr;
	if (!Spec || !AbilityCDO || AbilityCDO->GetActivationPolicy() == EPRAbilityActivationPolicy::Passive)
	{
		return;
	}

	Spec->InputPressed = true;
	HeldAbilityHandles.Add(Spec->Handle);
	AbilitySpecInputPressed(*Spec);
	if (!Spec->IsActive())
	{
		TryActivateAbility(Spec->Handle);
	}
}

void UPRAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	const TArray<FGameplayAbilitySpecHandle> Matches = FindSpecsByExactDynamicTag(InputTag);
	if (Matches.Num() > 1)
	{
		UE_LOG(LogProjectR, Error, TEXT("InputTag %s matches multiple AbilitySpecs."), *InputTag.ToString());
		return;
	}
	if (Matches.IsEmpty() || !HeldAbilityHandles.Contains(Matches[0]))
	{
		return;
	}

	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Matches[0]);
	const UPRGameplayAbility* AbilityCDO = Spec
		? Cast<UPRGameplayAbility>(Spec->Ability)
		: nullptr;
	if (!Spec || !AbilityCDO)
	{
		HeldAbilityHandles.Remove(Matches[0]);
		return;
	}
	AbilitySpecInputReleased(*Spec);
	Spec->InputPressed = false;
	HeldAbilityHandles.Remove(Spec->Handle);
	if (AbilityCDO->GetActivationPolicy() == EPRAbilityActivationPolicy::WhileInputActive
		&& Spec->IsActive())
	{
		CancelAbilityHandle(Spec->Handle);
	}
}

bool UPRAbilitySystemComponent::GetAbilityRuntimeStateByAbilityTag(
	const FGameplayTag AbilityTag,
	FPRAbilityRuntimeState& OutState) const
{
	const TArray<FGameplayAbilitySpecHandle> Matches = FindSpecsByExactDynamicTag(AbilityTag);
	if (Matches.Num() != 1)
	{
		return false;
	}
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Matches[0]);
	return Spec && BuildRuntimeState(*Spec, OutState);
}

bool UPRAbilitySystemComponent::GetAbilityRuntimeStateByInputTag(
	const FGameplayTag InputTag,
	FPRAbilityRuntimeState& OutState) const
{
	const TArray<FGameplayAbilitySpecHandle> Matches = FindSpecsByExactDynamicTag(InputTag);
	if (Matches.Num() != 1)
	{
		return false;
	}
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Matches[0]);
	return Spec && BuildRuntimeState(*Spec, OutState);
}

FPRAbilityLifecycleEventNative& UPRAbilitySystemComponent::OnAbilityLifecycleEvent()
{
	return AbilityLifecycleEvent;
}

void UPRAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	const TWeakObjectPtr<AActor> PreviousAvatar = GetAvatarActor();
	if (PreviousAvatar.IsValid() && PreviousAvatar.Get() != InAvatarActor)
	{
		ReleaseHeldAbilities();
		CancelProjectRAbilities(false);
	}

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	if (!DeadTagEventHandle.IsValid())
	{
		DeadTagEventHandle = RegisterGameplayTagEvent(
			UPRTagLibrary::GetStateDeadTag(), EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRAbilitySystemComponent::HandleProjectRLifeStateChanged);
	}
	if (!StunnedTagEventHandle.IsValid())
	{
		StunnedTagEventHandle = RegisterGameplayTagEvent(
			UPRTagLibrary::GetStateStunnedTag(), EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRAbilitySystemComponent::HandleProjectRStunStateChanged);
	}
	if (!HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		ActivatePassiveAbilities();
	}
}

void UPRAbilitySystemComponent::ClearActorInfo()
{
	ReleaseHeldAbilities();
	CancelProjectRAbilities(false);
	Super::ClearActorInfo();
}

void UPRAbilitySystemComponent::NotifyAbilityCommit(UGameplayAbility* Ability)
{
	Super::NotifyAbilityCommit(Ability);
	if (Ability)
	{
		BroadcastLifecycle(EPRAbilityLifecycleEventType::Committed,
			Ability->GetCurrentAbilitySpecHandle());
	}
}

void UPRAbilitySystemComponent::NotifyAbilityActivated(
	const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);
	if (Cast<UPRGameplayAbility>(Ability))
	{
		BroadcastLifecycle(EPRAbilityLifecycleEventType::Activated, Handle);
	}
}

void UPRAbilitySystemComponent::NotifyAbilityFailed(
	const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability,
	const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);
	if (Cast<UPRGameplayAbility>(Ability))
	{
		BroadcastLifecycle(EPRAbilityLifecycleEventType::ActivationFailed, Handle, FailureReason);
	}
}

bool UPRAbilitySystemComponent::ValidateAbilitySetForGrant(
	const UPRAbilitySetDataAsset* AbilitySet) const
{
	if (!AbilitySet)
	{
		return false;
	}
	TSet<FGameplayTag> AbilityTags;
	TSet<FGameplayTag> InputTags;
	for (const FPRAbilitySetEntry& Entry : AbilitySet->GetAbilityEntries())
	{
		const UPRGameplayAbility* AbilityCDO = Entry.AbilityClass
			? Entry.AbilityClass->GetDefaultObject<UPRGameplayAbility>()
			: nullptr;
		if (!AbilityCDO || Entry.AbilityLevel < 1 || !AbilityCDO->GetProjectRAbilityTag().IsValid())
		{
			return false;
		}
		const UPRPlayerSkillGameplayAbility* PlayerSkillAbility = Cast<UPRPlayerSkillGameplayAbility>(AbilityCDO);
		const UPRPlayerSkillDataAsset* PlayerSkillData = Cast<UPRPlayerSkillDataAsset>(Entry.AbilityData);
		if ((PlayerSkillAbility != nullptr) != (PlayerSkillData != nullptr)
			|| (PlayerSkillData
				&& (PlayerSkillData->AbilityClass != Entry.AbilityClass
					|| PlayerSkillData->AbilityTag != AbilityCDO->GetProjectRAbilityTag()
					|| PlayerSkillData->InputTag != Entry.InputTag)))
		{
			return false;
		}
		const bool bPassive = AbilityCDO->GetActivationPolicy() == EPRAbilityActivationPolicy::Passive;
		if (bPassive ? Entry.InputTag.IsValid()
			: (!Entry.InputTag.IsValid() || !Entry.InputTag.ToString().StartsWith(TEXT("Input."))))
		{
			return false;
		}
		if (AbilityTags.Contains(AbilityCDO->GetProjectRAbilityTag())
			|| (Entry.InputTag.IsValid() && InputTags.Contains(Entry.InputTag)))
		{
			return false;
		}
		AbilityTags.Add(AbilityCDO->GetProjectRAbilityTag());
		if (Entry.InputTag.IsValid())
		{
			InputTags.Add(Entry.InputTag);
		}
		for (const FGameplayTag& Tag : Entry.GrantedSpecTags)
		{
			if (Tag.ToString().StartsWith(TEXT("Input.")))
			{
				return false;
			}
		}
	}
	return true;
}

bool UPRAbilitySystemComponent::DoesSpecConflict(const FPRAbilitySetEntry& Entry) const
{
	const UPRGameplayAbility* NewAbility = Entry.AbilityClass->GetDefaultObject<UPRGameplayAbility>();
	for (const FGameplayAbilitySpec& ExistingSpec : GetActivatableAbilities())
	{
		const UPRGameplayAbility* ExistingAbility = Cast<UPRGameplayAbility>(ExistingSpec.Ability);
		if (ExistingAbility
			&& ExistingAbility->GetProjectRAbilityTag() == NewAbility->GetProjectRAbilityTag())
		{
			return true;
		}
		if (Entry.InputTag.IsValid()
			&& ExistingSpec.GetDynamicSpecSourceTags().HasTagExact(Entry.InputTag))
		{
			return true;
		}
	}
	return false;
}

void UPRAbilitySystemComponent::RollBackGrantedSpecs(
	const TArray<FGameplayAbilitySpecHandle>& Handles)
{
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		ClearAbility(Handle);
	}
}

void UPRAbilitySystemComponent::ActivatePassiveAbilities()
{
	if (!AbilityActorInfo.IsValid() || !AbilityActorInfo->AvatarActor.IsValid()
		|| HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		return;
	}
	TArray<FGameplayAbilitySpecHandle> PassiveHandles;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UPRGameplayAbility* Ability = Cast<UPRGameplayAbility>(Spec.Ability);
		if (Ability && Ability->GetActivationPolicy() == EPRAbilityActivationPolicy::Passive
			&& !Spec.IsActive())
		{
			PassiveHandles.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : PassiveHandles)
	{
		TryActivateAbility(Handle);
	}
}

void UPRAbilitySystemComponent::ReleaseHeldAbilities()
{
	const TArray<FGameplayAbilitySpecHandle> Handles = HeldAbilityHandles.Array();
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle))
		{
			AbilitySpecInputReleased(*Spec);
			Spec->InputPressed = false;
		}
	}
	HeldAbilityHandles.Reset();
}

void UPRAbilitySystemComponent::ReleaseHeldPlayerSkillAbilities()
{
	const TArray<FGameplayAbilitySpecHandle> Handles = HeldAbilityHandles.Array();
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec || !Cast<UPRPlayerSkillGameplayAbility>(Spec->Ability))
		{
			continue;
		}
		AbilitySpecInputReleased(*Spec);
		Spec->InputPressed = false;
		HeldAbilityHandles.Remove(Handle);
	}
}

void UPRAbilitySystemComponent::CancelProjectRAbilities(const bool bOnlyCancelOnDeath)
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UPRGameplayAbility* Ability = Cast<UPRGameplayAbility>(Spec.Ability);
		if (Ability && Spec.IsActive()
			&& (!bOnlyCancelOnDeath || Ability->ShouldCancelOnDeath()))
		{
			Handles.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		CancelAbilityHandle(Handle);
	}
}

void UPRAbilitySystemComponent::CancelPlayerSkillAbilities()
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive() && Cast<UPRPlayerSkillGameplayAbility>(Spec.Ability))
		{
			Handles.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		CancelAbilityHandle(Handle);
	}
}

void UPRAbilitySystemComponent::HandleProjectRLifeStateChanged(
	const FGameplayTag Tag,
	const int32 NewCount)
{
	if (Tag != UPRTagLibrary::GetStateDeadTag())
	{
		return;
	}
	if (NewCount > 0)
	{
		ReleaseHeldAbilities();
		CancelProjectRAbilities(true);
	}
	else
	{
		ActivatePassiveAbilities();
	}
}

void UPRAbilitySystemComponent::HandleProjectRStunStateChanged(
	const FGameplayTag Tag,
	const int32 NewCount)
{
	if (Tag == UPRTagLibrary::GetStateStunnedTag() && NewCount > 0)
	{
		ReleaseHeldPlayerSkillAbilities();
		CancelPlayerSkillAbilities();
	}
}

void UPRAbilitySystemComponent::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (Cast<UPRGameplayAbility>(EndedData.AbilityThatEnded))
	{
		BroadcastLifecycle(
			EndedData.bWasCancelled
				? EPRAbilityLifecycleEventType::Cancelled
				: EPRAbilityLifecycleEventType::Ended,
			EndedData.AbilitySpecHandle);
	}
}

void UPRAbilitySystemComponent::NotifyProjectRCommitFailed(
	UPRGameplayAbility* Ability,
	const FGameplayTagContainer& FailureTags)
{
	if (Ability)
	{
		BroadcastLifecycle(EPRAbilityLifecycleEventType::CommitFailed,
			Ability->GetCurrentAbilitySpecHandle(), FailureTags);
	}
}

bool UPRAbilitySystemComponent::BuildRuntimeState(
	const FGameplayAbilitySpec& Spec,
	FPRAbilityRuntimeState& OutState) const
{
	const UPRGameplayAbility* Ability = Cast<UPRGameplayAbility>(Spec.Ability);
	if (!Ability)
	{
		return false;
	}
	OutState = FPRAbilityRuntimeState();
	OutState.SpecHandle = Spec.Handle;
	OutState.AbilityTag = Ability->GetProjectRAbilityTag();
	for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
	{
		if (Tag.ToString().StartsWith(TEXT("Input.")))
		{
			OutState.InputTag = Tag;
			break;
		}
	}
	OutState.ActivationPolicy = Ability->GetActivationPolicy();
	OutState.AbilityLevel = Spec.Level;
	OutState.bActive = Spec.IsActive();
	OutState.bInputHeld = HeldAbilityHandles.Contains(Spec.Handle);
	if (AbilityActorInfo.IsValid())
	{
		OutState.bCanActivate = Ability->CanActivateAbility(
			Spec.Handle, AbilityActorInfo.Get(), nullptr, nullptr, &OutState.FailureTags);
	}

	if (const UGameplayEffect* Cost = Ability->GetCostGameplayEffect())
	{
		for (const FGameplayModifierInfo& Modifier : Cost->Modifiers)
		{
			if (Modifier.Attribute == UPRAttributeSet::GetEnergyAttribute())
			{
				float Magnitude = 0.0f;
				if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(Spec.Level, Magnitude))
				{
					OutState.EnergyCost = FMath::Max(0.0f, -Magnitude);
				}
				break;
			}
		}
	}
	if (const UGameplayEffect* Cooldown = Ability->GetCooldownGameplayEffect())
	{
		Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(Spec.Level, OutState.CooldownDuration);
		if (const FGameplayTagContainer* CooldownTags = Ability->GetCooldownTags();
			CooldownTags && !CooldownTags->IsEmpty())
		{
			const FGameplayEffectQuery Query =
				FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
			for (const TPair<float, float>& Time : GetActiveEffectsTimeRemainingAndDuration(Query))
			{
				OutState.CooldownRemaining = FMath::Max(OutState.CooldownRemaining, Time.Key);
			}
		}
	}
	return true;
}

void UPRAbilitySystemComponent::BroadcastLifecycle(
	const EPRAbilityLifecycleEventType EventType,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayTagContainer& FailureTags)
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		return;
	}
	FPRAbilityLifecycleEvent Event;
	Event.EventType = EventType;
	BuildRuntimeState(*Spec, Event.AbilityState);
	Event.FailureTags = FailureTags;
	Event.AbilityState.FailureTags = FailureTags;
	Event.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	AbilityLifecycleEvent.Broadcast(Event);
}

TArray<FGameplayAbilitySpecHandle> UPRAbilitySystemComponent::FindSpecsByExactDynamicTag(
	const FGameplayTag Tag) const
{
	TArray<FGameplayAbilitySpecHandle> Matches;
	if (!Tag.IsValid())
	{
		return Matches;
	}
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(Tag))
		{
			Matches.Add(Spec.Handle);
		}
	}
	return Matches;
}
