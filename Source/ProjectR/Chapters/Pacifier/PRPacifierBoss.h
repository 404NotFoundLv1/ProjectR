// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyCharacter.h"

#include "PRPacifierBoss.generated.h"

class UPRPacifierBossComponent;
class UTextRenderComponent;

UCLASS()
class PROJECTR_API APRPacifierBoss : public APREnemyCharacter
{
	GENERATED_BODY()

public:
	APRPacifierBoss();
	UPRPacifierBossComponent* GetPacifierBossComponent() const;

private:
	friend class UPRPacifierBossComponent;
	void SetMechanicPresentation(const FText& Text, const FColor& Color, bool bVisible);

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Pacifier") TObjectPtr<UPRPacifierBossComponent> PacifierBossComponent;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Pacifier") TObjectPtr<UTextRenderComponent> MechanicText;
};
