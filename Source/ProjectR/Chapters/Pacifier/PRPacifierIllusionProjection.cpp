// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Pacifier/PRPacifierIllusionProjection.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

APRPacifierIllusionProjection::APRPacifierIllusionProjection()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectionRoot"));
	SetRootComponent(SceneRoot);
	ProjectionText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ProjectionText"));
	ProjectionText->SetupAttachment(SceneRoot);
	ProjectionText->SetText(FText::FromString(TEXT("ILLUSION")));
	ProjectionText->SetTextRenderColor(FColor(80, 220, 255));
	ProjectionText->SetHorizontalAlignment(EHTA_Center);
	ProjectionText->SetWorldSize(34.0f);
	ProjectionText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
