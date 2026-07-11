// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"

#include "PRPlayerController.generated.h"

class UInputMappingContext;
class UPRInputConfigDataAsset;

/** Formal ProjectR player controller extension point. */
UCLASS(Abstract)
class PROJECTR_API APRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Returns the immutable input configuration assigned by this controller's Blueprint CDO. */
	const UPRInputConfigDataAsset* GetInputConfig() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Input")
	TObjectPtr<UPRInputConfigDataAsset> DefaultInputConfig = nullptr;

private:
	bool EnsureDefaultMappingContext();
	void RemoveOwnedMappingContext();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> AppliedMappingContext = nullptr;

	bool bOwnsAppliedMappingContext = false;
};
