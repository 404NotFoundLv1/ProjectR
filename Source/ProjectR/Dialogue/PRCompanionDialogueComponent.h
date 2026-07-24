// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "PRCompanionDialogueComponent.generated.h"

class APRPlayerCharacter;
class UPRCompanionDialogueSubsystem;
class UPRCompanionDialogueWidget;
struct FPRDialogueRuntimeState;
struct FPRSemanticInputEvent;

/** Local semantic-input and presentation bridge; no relationship or dialogue arbitration authority. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRCompanionDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void InitializeForSubsystem(UPRCompanionDialogueSubsystem* InSubsystem);
	void RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn);
	void SetDialogueWidgetClass(TSubclassOf<UPRCompanionDialogueWidget> InWidgetClass);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleSemanticInput(const FPRSemanticInputEvent& Event);
	void HandleDialogueStateChanged(const FPRDialogueRuntimeState& State);
	void ClearBinding();
	void ClearWidget();

	TWeakObjectPtr<UPRCompanionDialogueSubsystem> DialogueSubsystem;
	TWeakObjectPtr<APRPlayerCharacter> BoundPlayerPawn;
	FDelegateHandle SemanticInputHandle;
	FDelegateHandle StateChangedHandle;
	TSubclassOf<UPRCompanionDialogueWidget> DialogueWidgetClass;
	TWeakObjectPtr<UPRCompanionDialogueWidget> DialogueWidget;
};
