// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "QTE/PRQTETypes.h"

#include "PRQTEComponent.generated.h"

class APRPlayerCharacter;
class UPRQTESubsystem;
class UPRQTEPromptWidget;
struct FPRSemanticInputEvent;

/** Local input bridge only; QTE decision and effects remain owned by UPRQTESubsystem. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRQTEComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void InitializeForSubsystem(UPRQTESubsystem* InSubsystem);
	void RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn);
	void SetPromptWidgetClass(TSubclassOf<UPRQTEPromptWidget> InPromptWidgetClass);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleSemanticInput(const FPRSemanticInputEvent& Event);
	void HandleQTEStateChanged(const FPRQTERuntimeState& State);
	void ClearBinding();
	void ClearPrompt();

	TWeakObjectPtr<UPRQTESubsystem> QTESubsystem;
	TWeakObjectPtr<APRPlayerCharacter> BoundPlayerPawn;
	FDelegateHandle SemanticInputHandle;
	FDelegateHandle QTEStateHandle;
	TSubclassOf<UPRQTEPromptWidget> PromptWidgetClass;
	TWeakObjectPtr<UPRQTEPromptWidget> PromptWidget;
};
