// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "PRPacifierIllusionProjection.generated.h"

/** Presentation-only, transient projection. It has no collision, ASC, combat, target, completion, or save identity. */
UCLASS(NotPlaceable, Transient)
class PROJECTR_API APRPacifierIllusionProjection final : public AActor
{
	GENERATED_BODY()

public:
	APRPacifierIllusionProjection();

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Pacifier") TObjectPtr<class UTextRenderComponent> ProjectionText;
};
