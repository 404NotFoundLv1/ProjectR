// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Auditor/PRAuditorChapterBoss.h"

#include "PRHeadmindProjectionBoss.generated.h"

class UPRHeadmindProjectionBossComponent;
class UTextRenderComponent;

/** Fifth chapter actor: preserves the frozen Demo Auditor and adds value-only presentation. */
UCLASS()
class PROJECTR_API APRHeadmindProjectionBoss final : public APRBossAuditor
{
	GENERATED_BODY()
public:
	APRHeadmindProjectionBoss();
	UPRHeadmindProjectionBossComponent* GetHeadmindProjectionBossComponent() const;
private:
	friend class UPRHeadmindProjectionBossComponent;
	void SetHeadmindPresentation(const FText& Text, const FColor& Color, bool bVisible);
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Headmind") TObjectPtr<UPRHeadmindProjectionBossComponent> HeadmindComponent;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Headmind") TObjectPtr<UTextRenderComponent> HeadmindText;
};
