// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRSkillSlotWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPRSkillSlotWidget::ConfigureSlot(const FGameplayTag& InAbilityTag, const int32 InSlotIndex)
{
	AbilityTag = InAbilityTag;
	SlotIndex = InSlotIndex;
	RefreshPresentation();
}

void UPRSkillSlotWidget::SetRuntimeState(const FPRAbilityRuntimeState& InState)
{
	RuntimeState = InState;
	RefreshPresentation();
}

FGameplayTag UPRSkillSlotWidget::GetAbilityTag() const { return AbilityTag; }

float UPRSkillSlotWidget::GetCooldownPercent() const
{
	return RuntimeState.CooldownDuration > 0.0f
		? FMath::Clamp(RuntimeState.CooldownRemaining / RuntimeState.CooldownDuration, 0.0f, 1.0f)
		: 0.0f;
}

FText UPRSkillSlotWidget::GetDisplayNameForAbilityTag(const FGameplayTag& InAbilityTag)
{
	const FName TagName = InAbilityTag.GetTagName();
	if (TagName == TEXT("Skill.ShadowThrust")) return FText::FromString(TEXT("Shadow Thrust"));
	if (TagName == TEXT("Skill.FireSlash")) return FText::FromString(TEXT("Fire Slash"));
	if (TagName == TEXT("Skill.ThunderDrop")) return FText::FromString(TEXT("Thunder Drop"));
	if (TagName == TEXT("Skill.AfterimageDodge")) return FText::FromString(TEXT("Afterimage Dodge"));
	if (TagName == TEXT("Skill.VectorHook")) return FText::FromString(TEXT("Vector Hook"));
	if (TagName == TEXT("Skill.CounterProofWall")) return FText::FromString(TEXT("Counterproof Wall"));
	return FText::FromName(TagName);
}

void UPRSkillSlotWidget::RefreshPresentation()
{
	if (SkillNameText)
	{
		SkillNameText->SetText(FText::FromString(FString::Printf(
			TEXT("%d. %s"), SlotIndex + 1, *GetDisplayNameForAbilityTag(AbilityTag).ToString())));
	}
	if (SkillStateText)
	{
		const TCHAR* State = RuntimeState.bActive ? TEXT("Active")
			: RuntimeState.CooldownRemaining > 0.0f ? TEXT("Cooldown")
			: RuntimeState.bCanActivate ? TEXT("Ready") : TEXT("Unavailable");
		SkillStateText->SetText(FText::FromString(FString::Printf(
			TEXT("%s  Held:%s  Cost:%.0f  CD:%.1f/%.1f"), State,
			RuntimeState.bInputHeld ? TEXT("Yes") : TEXT("No"), RuntimeState.EnergyCost,
			RuntimeState.CooldownRemaining, RuntimeState.CooldownDuration)));
	}
	if (FailureText)
	{
		FailureText->SetText(FText::FromString(RuntimeState.FailureTags.ToStringSimple()));
	}
	if (CooldownBar)
	{
		CooldownBar->SetPercent(GetCooldownPercent());
	}
}
