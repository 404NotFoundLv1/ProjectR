// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyCharacter.h"

#include "PRWardenBoss.generated.h"

class UPRWardenBossComponent;

UCLASS()
class PROJECTR_API APRWardenBoss : public APREnemyCharacter
{
	GENERATED_BODY()

public:
	APRWardenBoss();
	UPRWardenBossComponent* GetWardenBossComponent() const;

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Warden") TObjectPtr<UPRWardenBossComponent> WardenBossComponent;
};
