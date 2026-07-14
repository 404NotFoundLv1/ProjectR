// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDebugSubsystem.h"

#include "Core/PRDeveloperSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "PRDebugBuiltInCommands.h"
#include "PRDebugInputPreprocessor.h"
#include "PRDebugLog.h"
#include "Slate/SceneViewport.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SPRDebugPanel.h"

int32 UPRDebugSubsystem::OpenPanelCount = 0;

bool UPRDebugSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer)
		&& GetDefault<UPRDeveloperSettings>()->bEnableDebugFeatures;
#endif
}

void UPRDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CommandRegistry = MakeUnique<FPRDebugCommandRegistry>();
	BuiltInProviderHandle = CommandRegistry->RegisterProvider(
		TEXT("ProjectR.BuiltIn"),
		FPRDebugBuiltInCommands::BuildDescriptors(),
		FPRDebugCommandHandler::CreateUObject(this, &UPRDebugSubsystem::ExecuteBuiltInCommand));
	if (!BuiltInProviderHandle.IsValid())
	{
		UE_LOG(LogProjectRDebug, Error, TEXT("Failed to register built-in debug commands."));
	}

	if (FSlateApplication::IsInitialized())
	{
		InputPreprocessor = MakeShared<FPRDebugInputPreprocessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputPreprocessor);
	}

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
		this,
		&UPRDebugSubsystem::HandlePreLoadMap);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
		this,
		&UPRDebugSubsystem::HandleWorldCleanup);
	UE_LOG(LogProjectRDebug, Log, TEXT("ProjectR debug subsystem initialized."));
}

void UPRDebugSubsystem::Deinitialize()
{
	ClosePanel();
	if (FSlateApplication::IsInitialized() && InputPreprocessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputPreprocessor);
	}
	InputPreprocessor.Reset();

	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
		PreLoadMapHandle.Reset();
	}
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

	if (CommandRegistry.IsValid() && BuiltInProviderHandle.IsValid())
	{
		CommandRegistry->UnregisterProvider(BuiltInProviderHandle);
	}
	BuiltInProviderHandle = FPRDebugProviderHandle();
	CommandRegistry.Reset();
	UE_LOG(LogProjectRDebug, Log, TEXT("ProjectR debug subsystem deinitialized."));
	Super::Deinitialize();
}

FPRDebugCommandRegistry* UPRDebugSubsystem::GetCommandRegistry() const
{
	return CommandRegistry.Get();
}

#if WITH_DEV_AUTOMATION_TESTS
TArray<FPRDebugCommandDescriptor> UPRDebugSubsystem::BuildBuiltInDescriptorsForTests()
{
	return FPRDebugBuiltInCommands::BuildDescriptors();
}

FPRDebugCommandResult UPRDebugSubsystem::ExecuteBuiltInCommandForTests(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	return ExecuteBuiltInCommand(Request, GameInstance);
}

bool UPRDebugSubsystem::IsAnyDebugPanelOpenForTests()
{
	return OpenPanelCount > 0;
}
#endif

FPRDebugCommandResult UPRDebugSubsystem::ExecuteBuiltInCommand(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	return FPRDebugBuiltInCommands::Execute(Request, GameInstance);
}

FPRDebugCommandResult UPRDebugSubsystem::ExecutePanelCommand(const FPRDebugCommandRequest& Request)
{
	if (!CommandRegistry.IsValid())
	{
		FPRDebugCommandResult Result;
		Result.RequestId = Request.RequestId.IsValid() ? Request.RequestId : FGuid::NewGuid();
		Result.CommandId = Request.CommandId;
		Result.ResultCode = EPRDebugCommandResultCode::NotAvailable;
		Result.PlayerMessage = FText::FromString(TEXT("Command registry is not available."));
		return Result;
	}
	return CommandRegistry->ExecuteCommand(Request, GetGameInstance());
}

bool UPRDebugSubsystem::CanTogglePanelFromInput() const
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
	UGameViewportClient* ViewportClient = GameInstance != nullptr ? GameInstance->GetGameViewportClient() : nullptr;
	FSceneViewport* SceneViewport = ViewportClient != nullptr ? ViewportClient->GetGameViewport() : nullptr;
	if (World == nullptr || !World->IsGameWorld() || ViewportClient == nullptr || SceneViewport == nullptr)
	{
		return false;
	}

	const TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget();
	const bool bViewportFocused = ViewportWidget.IsValid()
		&& (ViewportWidget->HasKeyboardFocus() || ViewportWidget->HasFocusedDescendants());
	const bool bViewportCaptured = SceneViewport->HasMouseCapture();
	if (!PanelWidget.IsValid())
	{
		return bViewportFocused || bViewportCaptured;
	}

	const bool bPanelFocused = PanelWidget->HasKeyboardFocus() || PanelWidget->HasFocusedDescendants();
	return bViewportFocused || bViewportCaptured || bPanelFocused;
}

bool UPRDebugSubsystem::TogglePanel()
{
	return PanelWidget.IsValid() ? ClosePanel() : OpenPanel();
}

bool UPRDebugSubsystem::OpenPanel()
{
	if (PanelWidget.IsValid() || !CommandRegistry.IsValid())
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGameViewportClient* ViewportClient = GameInstance != nullptr ? GameInstance->GetGameViewportClient() : nullptr;
	APlayerController* Controller = GameInstance != nullptr
		? GameInstance->GetFirstLocalPlayerController()
		: nullptr;
	if (ViewportClient == nullptr || Controller == nullptr)
	{
		return false;
	}

	SAssignNew(PanelWidget, SPRDebugPanel)
		.Owner(this)
		.Descriptors(CommandRegistry->GetCommandDescriptors());
	ViewportClient->AddViewportWidgetContent(PanelWidget.ToSharedRef(), 10000);
	PanelViewport = ViewportClient;
	PanelPlayerController = Controller;
	bPreviousShowMouseCursor = Controller->bShowMouseCursor;
	Controller->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetWidgetToFocus(PanelWidget);
	Controller->SetInputMode(InputMode);
	++OpenPanelCount;
	UE_LOG(LogProjectRDebug, Log, TEXT("ProjectR debug panel opened."));
	return true;
}

bool UPRDebugSubsystem::ClosePanel()
{
	if (!PanelWidget.IsValid())
	{
		return false;
	}

	if (UGameViewportClient* ViewportClient = PanelViewport.Get())
	{
		ViewportClient->RemoveViewportWidgetContent(PanelWidget.ToSharedRef());
	}
	if (APlayerController* Controller = PanelPlayerController.Get())
	{
		Controller->bShowMouseCursor = bPreviousShowMouseCursor;
		Controller->SetInputMode(FInputModeGameOnly());
	}
	PanelWidget.Reset();
	PanelViewport.Reset();
	PanelPlayerController.Reset();
	OpenPanelCount = FMath::Max(0, OpenPanelCount - 1);
	UE_LOG(LogProjectRDebug, Log, TEXT("ProjectR debug panel closed."));
	return true;
}

void UPRDebugSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance == GetGameInstance())
	{
		ClosePanel();
	}
}

void UPRDebugSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World != nullptr && World->GetGameInstance() == GetGameInstance())
	{
		ClosePanel();
	}
}
