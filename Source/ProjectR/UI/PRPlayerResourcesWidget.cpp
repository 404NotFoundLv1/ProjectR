// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRPlayerResourcesWidget.h"

#include "Abilities/PRAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/PRPlayerState.h"

float UPRPlayerResourcesWidget::GetHealthPercent() const { return HealthPercent; }
float UPRPlayerResourcesWidget::GetShieldPercent() const { return ShieldPercent; }
float UPRPlayerResourcesWidget::GetEnergyPercent() const { return EnergyPercent; }

void UPRPlayerResourcesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PlayerState = GetOwningPlayerState<APRPlayerState>();
	if (PlayerState.IsValid())
	{
		AttributeChangedHandle = PlayerState->OnAttributeChanged().AddUObject(
			this, &UPRPlayerResourcesWidget::HandleAttributeChanged);
	}
	RefreshPresentation();
}

void UPRPlayerResourcesWidget::NativeDestruct()
{
	if (PlayerState.IsValid())
	{
		PlayerState->OnAttributeChanged().Remove(AttributeChangedHandle);
	}
	AttributeChangedHandle.Reset();
	PlayerState.Reset();
	Super::NativeDestruct();
}

void UPRPlayerResourcesWidget::HandleAttributeChanged(const FPRAttributeChange& Change)
{
	RefreshPresentation();
}

void UPRPlayerResourcesWidget::RefreshPresentation()
{
	const UPRAttributeSet* Attributes = PlayerState.IsValid() ? PlayerState->GetAttributeSet() : nullptr;
	const float Health = Attributes ? Attributes->GetHealth() : 0.0f;
	const float MaxHealth = Attributes ? Attributes->GetMaxHealth() : 0.0f;
	const float Shield = Attributes ? Attributes->GetShield() : 0.0f;
	const float MaxShield = Attributes ? Attributes->GetMaxShield() : 0.0f;
	const float Energy = Attributes ? Attributes->GetEnergy() : 0.0f;
	const float MaxEnergy = Attributes ? Attributes->GetMaxEnergy() : 0.0f;
	const float Permission = Attributes ? Attributes->GetPermission() : 0.0f;
	const float Resonance = Attributes ? Attributes->GetResonance() : 0.0f;
	HealthPercent = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	ShieldPercent = MaxShield > 0.0f ? FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f) : 0.0f;
	EnergyPercent = MaxEnergy > 0.0f ? FMath::Clamp(Energy / MaxEnergy, 0.0f, 1.0f) : 0.0f;
	if (HealthBar) HealthBar->SetPercent(HealthPercent);
	if (ShieldBar) ShieldBar->SetPercent(ShieldPercent);
	if (EnergyBar) EnergyBar->SetPercent(EnergyPercent);
	if (HealthText) HealthText->SetText(FText::FromString(FString::Printf(TEXT("Health %.0f / %.0f"), Health, MaxHealth)));
	if (ShieldText) ShieldText->SetText(FText::FromString(FString::Printf(TEXT("Shield %.0f / %.0f"), Shield, MaxShield)));
	if (EnergyText) EnergyText->SetText(FText::FromString(FString::Printf(TEXT("Energy %.0f / %.0f"), Energy, MaxEnergy)));
	if (PermissionText) PermissionText->SetText(FText::FromString(FString::Printf(TEXT("Permission %.0f"), Permission)));
	if (ResonanceText) ResonanceText->SetText(FText::FromString(FString::Printf(TEXT("Resonance %.0f"), Resonance)));
}
