// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"

#include "Chapters/Headmind/PRHeadmindProjectionBossComponent.h"
#include "Components/TextRenderComponent.h"

APRHeadmindProjectionBoss::APRHeadmindProjectionBoss()
{
	HeadmindComponent = CreateDefaultSubobject<UPRHeadmindProjectionBossComponent>(TEXT("HeadmindProjectionBossComponent"));
	HeadmindText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HeadmindMechanicText"));
	HeadmindText->SetupAttachment(GetRootComponent());
	HeadmindText->SetRelativeLocation(FVector(0.0f, 0.0f, 260.0f));
	HeadmindText->SetHorizontalAlignment(EHTA_Center);
	HeadmindText->SetWorldSize(28.0f);
	HeadmindText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadmindText->SetHiddenInGame(true);
}

UPRHeadmindProjectionBossComponent* APRHeadmindProjectionBoss::GetHeadmindProjectionBossComponent() const { return HeadmindComponent; }

void APRHeadmindProjectionBoss::SetHeadmindPresentation(const FText& Text, const FColor& Color, const bool bVisible)
{
	if (!HeadmindText) return;
	HeadmindText->SetText(Text);
	HeadmindText->SetTextRenderColor(Color);
	HeadmindText->SetHiddenInGame(!bVisible);
}
