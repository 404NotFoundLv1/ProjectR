// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRDebugCommandRegistry.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRDebugSubsystem.generated.h"

class FPRDebugInputPreprocessor;
class SPRDebugPanel;
class UGameViewportClient;
class UWorld;
struct FWorldContext;

/** Owns the Development-only registry, Slate panel, and F1 lifecycle. */
UCLASS()
class PROJECTRDEBUG_API UPRDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FPRDebugCommandRegistry* GetCommandRegistry() const;

#if WITH_DEV_AUTOMATION_TESTS
	static TArray<FPRDebugCommandDescriptor> BuildBuiltInDescriptorsForTests();
	FPRDebugCommandResult ExecuteBuiltInCommandForTests(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	static bool IsAnyDebugPanelOpenForTests();
#endif

private:
	FPRDebugCommandResult ExecuteBuiltInCommand(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	FPRDebugCommandResult ExecutePanelCommand(const FPRDebugCommandRequest& Request);
	bool CanTogglePanelFromInput() const;
	bool TogglePanel();
	bool OpenPanel();
	bool ClosePanel();
	void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	TUniquePtr<FPRDebugCommandRegistry> CommandRegistry;
	FPRDebugProviderHandle BuiltInProviderHandle;
	TSharedPtr<FPRDebugInputPreprocessor> InputPreprocessor;
	TSharedPtr<SPRDebugPanel> PanelWidget;
	TWeakObjectPtr<UGameViewportClient> PanelViewport;
	TWeakObjectPtr<APlayerController> PanelPlayerController;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle WorldCleanupHandle;
	bool bPreviousShowMouseCursor = false;
	static int32 OpenPanelCount;

	friend class FPRDebugInputPreprocessor;
	friend class SPRDebugPanel;
	friend class FPRDebugRegistryLifecycleTest;
};
