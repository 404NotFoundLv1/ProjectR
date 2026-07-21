// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRCombatFeedbackWidget.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/PRCombatSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"
#include "TimerManager.h"

namespace PRCombatFeedback
{
constexpr double MergeWindowSeconds = 0.2;
constexpr int32 MaxEntries = 3;
constexpr float LifetimeSeconds = 2.5f;
}

void UPRCombatFeedbackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr)
	{
		CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRCombatFeedbackWidget::HandleCombatEvent);
	}
	if (APRPlayerState* PlayerState = GetOwningPlayerState<APRPlayerState>())
	{
		AbilitySystem = PlayerState->GetProjectRAbilitySystemComponent();
		if (AbilitySystem.IsValid())
		{
			AbilityLifecycleHandle = AbilitySystem->OnAbilityLifecycleEvent().AddUObject(
				this, &UPRCombatFeedbackWidget::HandleAbilityLifecycleEvent);
		}
	}
}

void UPRCombatFeedbackWidget::NativeDestruct()
{
	if (UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr)
	{
		Combat->OnCombatEvent().Remove(CombatEventHandle);
	}
	if (AbilitySystem.IsValid())
	{
		AbilitySystem->OnAbilityLifecycleEvent().Remove(AbilityLifecycleHandle);
	}
	CombatEventHandle.Reset();
	AbilityLifecycleHandle.Reset();
	AbilitySystem.Reset();
	ClearFeedback();
	ConsumedCombatEventIds.Reset();
	Super::NativeDestruct();
}

void UPRCombatFeedbackWidget::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (!IsPlayerRelevant(Event) || !Event.EventId.IsValid() || ConsumedCombatEventIds.Contains(Event.EventId))
	{
		return;
	}
	ConsumedCombatEventIds.Add(Event.EventId);
	FName Semantic = TEXT("Combat");
	FString Text;
	const float Value = Event.HealthDamage + Event.ShieldAbsorbed;
	bool bAllowMerge = !Event.bFatal;
	if (Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePredictionBlockedTag()))
	{
		Semantic = TEXT("PredictionBlocked");
		Text = TEXT("Prediction blocked");
	}
	else if (Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseGuardBlockedTag()))
	{
		Semantic = TEXT("GuardBlocked");
		Text = TEXT("Guard blocked");
	}
	else if (Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()))
	{
		Semantic = TEXT("ShieldBroken");
		Text = TEXT("Shield broken");
	}
	else if (Event.bFatal)
	{
		Semantic = Event.TargetId == TEXT("Player") ? TEXT("PlayerDefeated") : TEXT("EnemyDefeated");
		Text = Event.TargetId == TEXT("Player") ? TEXT("Player defeated") : TEXT("Enemy defeated");
		bAllowMerge = false;
	}
	else if (Event.bCritical)
	{
		Semantic = TEXT("Critical");
		Text = FString::Printf(TEXT("Critical %.0f"), Event.HealthDamage + Event.ShieldAbsorbed);
	}
	else if (Event.TargetId == TEXT("Player") && Event.ShieldAbsorbed > 0.0f && Event.HealthDamage <= 0.0f)
	{
		Semantic = TEXT("PlayerShieldHit");
		Text = FString::Printf(TEXT("Shield hit %.0f"), Value);
	}
	else if (Event.TargetId == TEXT("Player") && Event.HealthDamage > 0.0f)
	{
		Semantic = TEXT("PlayerHit");
		Text = FString::Printf(TEXT("Player hit %.0f"), Value);
	}
	else if (Event.HealthDamage > 0.0f || Event.ShieldAbsorbed > 0.0f)
	{
		Semantic = TEXT("Damage");
		Text = FString::Printf(TEXT("Damage %.0f"), Event.HealthDamage + Event.ShieldAbsorbed);
	}
	else
	{
		Text = Event.AbilityTag.ToString();
	}
	AddFeedback(Event.SourceId, Event.TargetId, Event.AbilityTag, Semantic, Text, Value, bAllowMerge);
}

void UPRCombatFeedbackWidget::HandleAbilityLifecycleEvent(const FPRAbilityLifecycleEvent& Event)
{
	if (Event.EventType != EPRAbilityLifecycleEventType::ActivationFailed
		&& Event.EventType != EPRAbilityLifecycleEventType::CommitFailed)
	{
		return;
	}
	const FString Failure = Event.FailureTags.IsEmpty()
		? TEXT("Ability unavailable")
		: Event.FailureTags.ToStringSimple();
	AddFeedback(TEXT("Player"), NAME_None, Event.AbilityState.AbilityTag, TEXT("AbilityFailure"), Failure, 0.0f, false);
}

void UPRCombatFeedbackWidget::AddFeedback(
	const FName SourceId,
	const FName TargetId,
	const FGameplayTag AbilityTag,
	const FName Semantic,
	const FString& Text,
	const float Value,
	const bool bAllowMerge)
{
	if (!FeedbackEntries || !GetWorld())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (bAllowMerge && !Entries.IsEmpty())
	{
		FFeedbackEntry& Previous = Entries.Last();
		if (Previous.SourceId == SourceId && Previous.TargetId == TargetId
			&& Previous.AbilityTag == AbilityTag && Previous.Semantic == Semantic
			&& Now - Previous.CreatedAt <= PRCombatFeedback::MergeWindowSeconds)
		{
			++Previous.Count;
			Previous.TotalValue += Value;
			Previous.CreatedAt = Now;
			if (Previous.Text)
			{
				Previous.Text->SetText(FText::FromString(FString::Printf(TEXT("%s x%d (%.0f)"), *Text, Previous.Count, Previous.TotalValue)));
			}
			GetWorld()->GetTimerManager().ClearTimer(Previous.ExpireTimer);
			GetWorld()->GetTimerManager().SetTimer(Previous.ExpireTimer, FTimerDelegate::CreateUObject(
				this, &UPRCombatFeedbackWidget::RemoveFeedback, Previous.Id), PRCombatFeedback::LifetimeSeconds, false);
			return;
		}
	}
	while (Entries.Num() >= PRCombatFeedback::MaxEntries)
	{
		RemoveFeedback(Entries[0].Id);
	}
	FFeedbackEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Id = FGuid::NewGuid();
	Entry.SourceId = SourceId;
	Entry.TargetId = TargetId;
	Entry.AbilityTag = AbilityTag;
	Entry.Semantic = Semantic;
	Entry.CreatedAt = Now;
	Entry.TotalValue = Value;
	Entry.Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Entry.Text->SetText(FText::FromString(Text));
	FeedbackEntries->AddChildToVerticalBox(Entry.Text);
	GetWorld()->GetTimerManager().SetTimer(Entry.ExpireTimer, FTimerDelegate::CreateUObject(
		this, &UPRCombatFeedbackWidget::RemoveFeedback, Entry.Id), PRCombatFeedback::LifetimeSeconds, false);
}

void UPRCombatFeedbackWidget::RemoveFeedback(const FGuid Id)
{
	const int32 Index = Entries.IndexOfByPredicate([&Id](const FFeedbackEntry& Entry) { return Entry.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	FFeedbackEntry& Entry = Entries[Index];
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Entry.ExpireTimer);
	}
	if (Entry.Text)
	{
		Entry.Text->RemoveFromParent();
	}
	Entries.RemoveAt(Index);
}

void UPRCombatFeedbackWidget::ClearFeedback()
{
	while (!Entries.IsEmpty())
	{
		RemoveFeedback(Entries[0].Id);
	}
}

bool UPRCombatFeedbackWidget::IsPlayerRelevant(const FPRCombatEvent& Event) const
{
	return Event.SourceId == TEXT("Player") || Event.TargetId == TEXT("Player");
}
