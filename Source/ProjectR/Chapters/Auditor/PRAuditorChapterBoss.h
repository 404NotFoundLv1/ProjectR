// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Enemies/Bosses/PRBossAuditor.h"

#include "PRAuditorChapterBoss.generated.h"

class UPRAuditorChapterBossComponent;
class UTextRenderComponent;

/** Additive fourth-chapter actor: preserves APRBossAuditor mitigation, phases and completion. */
UCLASS()
class PROJECTR_API APRAuditorChapterBoss final : public APRBossAuditor
{
	GENERATED_BODY()

public:
	APRAuditorChapterBoss();
	UPRAuditorChapterBossComponent* GetAuditorChapterBossComponent() const;

private:
	friend class UPRAuditorChapterBossComponent;
	void SetChapterMechanicPresentation(const FText& Text, const FColor& Color, bool bVisible);

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Auditor") TObjectPtr<UPRAuditorChapterBossComponent> ChapterBossComponent;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Auditor") TObjectPtr<UTextRenderComponent> MechanicText;
};
