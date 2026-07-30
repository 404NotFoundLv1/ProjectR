// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Auditor/PRAuditorChapterBoss.h"

#include "Chapters/Auditor/PRAuditorChapterBossComponent.h"
#include "Components/TextRenderComponent.h"

APRAuditorChapterBoss::APRAuditorChapterBoss()
{
	ChapterBossComponent = CreateDefaultSubobject<UPRAuditorChapterBossComponent>(TEXT("AuditorChapterBossComponent"));
	MechanicText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AuditorChapterMechanicText"));
	MechanicText->SetupAttachment(GetRootComponent());
	MechanicText->SetRelativeLocation(FVector(0.0f, 0.0f, 210.0f));
	MechanicText->SetHorizontalAlignment(EHTA_Center);
	MechanicText->SetWorldSize(28.0f);
	MechanicText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MechanicText->SetHiddenInGame(true);
}

UPRAuditorChapterBossComponent* APRAuditorChapterBoss::GetAuditorChapterBossComponent() const { return ChapterBossComponent; }

void APRAuditorChapterBoss::SetChapterMechanicPresentation(const FText& Text, const FColor& Color, const bool bVisible)
{
	if (!MechanicText) return;
	MechanicText->SetText(Text);
	MechanicText->SetTextRenderColor(Color);
	MechanicText->SetHiddenInGame(!bVisible);
}
