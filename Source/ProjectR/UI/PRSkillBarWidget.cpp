// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRSkillBarWidget.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRGameplayAbility.h"
#include "Blueprint/WidgetTree.h"
#include "Core/PRPlayerState.h"
#include "TimerManager.h"
#include "UI/PRSkillSlotWidget.h"

void UPRSkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	APRPlayerState* PlayerState = GetOwningPlayerState<APRPlayerState>();
	AbilitySystem = PlayerState ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
	if (AbilitySystem.IsValid())
	{
		AbilityLifecycleHandle = AbilitySystem->OnAbilityLifecycleEvent().AddUObject(
			this, &UPRSkillBarWidget::HandleAbilityLifecycleEvent);
	}

	Slots.Reset();
	if (DefaultAbilitySet && WidgetTree)
	{
		const TArray<FPRAbilitySetEntry>& Entries = DefaultAbilitySet->GetAbilityEntries();
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FName SlotName(*FString::Printf(TEXT("SkillSlot_%02d"), Index + 1));
			UPRSkillSlotWidget* SkillSlot = Cast<UPRSkillSlotWidget>(WidgetTree->FindWidget(SlotName));
			const UPRGameplayAbility* Ability = Entries[Index].AbilityClass
				? Entries[Index].AbilityClass->GetDefaultObject<UPRGameplayAbility>() : nullptr;
			if (SkillSlot && Ability)
			{
				SkillSlot->ConfigureSlot(Ability->GetProjectRAbilityTag(), Index);
				Slots.Add(SkillSlot);
			}
		}
	}
	RefreshAllSlots();
}

void UPRSkillBarWidget::NativeDestruct()
{
	if (AbilitySystem.IsValid())
	{
		AbilitySystem->OnAbilityLifecycleEvent().Remove(AbilityLifecycleHandle);
	}
	AbilityLifecycleHandle.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownTimer);
	}
	Slots.Reset();
	AbilitySystem.Reset();
	Super::NativeDestruct();
}

void UPRSkillBarWidget::HandleAbilityLifecycleEvent(const FPRAbilityLifecycleEvent& Event)
{
	RefreshAllSlots();
}

void UPRSkillBarWidget::RefreshAllSlots()
{
	if (!AbilitySystem.IsValid())
	{
		return;
	}
	for (UPRSkillSlotWidget* SkillSlot : Slots)
	{
		if (!IsValid(SkillSlot))
		{
			continue;
		}
		FPRAbilityRuntimeState State;
		if (AbilitySystem->GetAbilityRuntimeStateByAbilityTag(SkillSlot->GetAbilityTag(), State))
		{
			SkillSlot->SetRuntimeState(State);
		}
	}
	UpdateCooldownTimer();
}

void UPRSkillBarWidget::RefreshCooldowns()
{
	RefreshAllSlots();
}

void UPRSkillBarWidget::UpdateCooldownTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	bool bHasCooldown = false;
	for (const UPRSkillSlotWidget* SkillSlot : Slots)
	{
		if (SkillSlot && SkillSlot->GetCooldownPercent() > 0.0f)
		{
			bHasCooldown = true;
			break;
		}
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (bHasCooldown)
	{
		if (!Timers.IsTimerActive(CooldownTimer))
		{
			Timers.SetTimer(CooldownTimer, this, &UPRSkillBarWidget::RefreshCooldowns, 0.1f, true);
		}
	}
	else
	{
		Timers.ClearTimer(CooldownTimer);
	}
}
