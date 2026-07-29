// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Pacifier/PRPacifierBoss.h"

#include "Chapters/Pacifier/PRPacifierBossComponent.h"
#include "Components/TextRenderComponent.h"

APRPacifierBoss::APRPacifierBoss()
{
	PacifierBossComponent = CreateDefaultSubobject<UPRPacifierBossComponent>(TEXT("PacifierBossComponent"));
	MechanicText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PacifierMechanicText"));
	MechanicText->SetupAttachment(GetRootComponent());
	MechanicText->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
	MechanicText->SetHorizontalAlignment(EHTA_Center);
	MechanicText->SetWorldSize(28.0f);
	MechanicText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MechanicText->SetHiddenInGame(true);
}

UPRPacifierBossComponent* APRPacifierBoss::GetPacifierBossComponent() const
{
	return PacifierBossComponent;
}

void APRPacifierBoss::SetMechanicPresentation(const FText& Text, const FColor& Color, const bool bVisible)
{
	if (!MechanicText) return;
	MechanicText->SetText(Text);
	MechanicText->SetTextRenderColor(Color);
	MechanicText->SetHiddenInGame(!bVisible);
}
