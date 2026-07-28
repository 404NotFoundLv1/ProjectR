// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Blueprint/UserWidget.h"
#include "Quests/PRCompanionQuestTypes.h"
#include "PRCompanionQuestWidget.generated.h"
class UPRCompanionQuestSubsystem;
class UTextBlock;
UCLASS(Abstract)
class PROJECTR_API UPRCompanionQuestWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|Quest") void PresentQuestSnapshot(const FPRCompanionQuestSnapshot& Snapshot);
private:
	UFUNCTION() void ActivateAxiomLowProbabilitySample();
	UFUNCTION() void ActivateAxiomImperfectOptimum();
	UFUNCTION() void ActivateKindleNoRetreatLine();
	UFUNCTION() void ActivateKindleLearnToRetreat();
	UFUNCTION() void ActivateNullGarbageCollection();
	UFUNCTION() void ActivateNullRememberMe();
	UFUNCTION() void RetryQuestPersistence();
	UFUNCTION() void ConfirmRememberMeAfterDisplayedGraveyard();
	void HandleStateChanged(const FPRCompanionQuestSnapshot& NewSnapshot);
	void HandleOperation(const FPRCompanionQuestOperationEvent& Event);
	void RequestFixedQuest(FName QuestId);
	void PresentReadOnlyText(const FPRCompanionQuestSnapshot& NewSnapshot);
	void PresentReadOnlyGraveyardProjection();
	UPRCompanionQuestSubsystem* GetQuestSubsystem() const;
	FDelegateHandle StateChangedHandle;
	FDelegateHandle OperationHandle;
};
