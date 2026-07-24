// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "PRDivergenceComponent.generated.h"

class APRPlayerCharacter;
class UPRDivergenceCacheWidget;
class UPRDivergenceSubsystem;
struct FPRDivergenceRuntimeState;
struct FPRSemanticInputEvent;

/** Local input/presentation bridge; gameplay ownership remains in the GI subsystem. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRDivergenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void InitializeForSubsystem(UPRDivergenceSubsystem* InSubsystem);
	void RebindPlayerPawn(APRPlayerCharacter* InPlayerPawn);
	void SetWidgetClass(TSubclassOf<UPRDivergenceCacheWidget> InWidgetClass);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleSemanticInput(const FPRSemanticInputEvent& Event);
	void HandleStateChanged(const FPRDivergenceRuntimeState& State);
	void ClearBinding();
	void ClearWidget();

	TWeakObjectPtr<UPRDivergenceSubsystem> DivergenceSubsystem;
	TWeakObjectPtr<APRPlayerCharacter> BoundPlayerPawn;
	TWeakObjectPtr<UPRDivergenceCacheWidget> Widget;
	TSubclassOf<UPRDivergenceCacheWidget> WidgetClass;
	FDelegateHandle SemanticInputHandle;
	FDelegateHandle StateChangedHandle;
};
