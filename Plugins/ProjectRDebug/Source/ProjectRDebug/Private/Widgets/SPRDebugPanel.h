// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PRDebugCommandTypes.h"
#include "Widgets/SCompoundWidget.h"

class UPRDebugSubsystem;

/** Data-driven native Slate surface for the fixed debug registry. */
class SPRDebugPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPRDebugPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UPRDebugSubsystem>, Owner)
		SLATE_ARGUMENT(TArray<FPRDebugCommandDescriptor>, Descriptors)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	static FString MakeArgumentKey(EPRDebugCommandId CommandId, FName ArgumentId);
	TSharedRef<SWidget> BuildCommandWidget(const FPRDebugCommandDescriptor& Descriptor);
	FReply HandleExecute(EPRDebugCommandId CommandId);
	FReply HandleClose();
	FPRDebugCommandRequest BuildRequest(const FPRDebugCommandDescriptor& Descriptor) const;
	FText FormatResult(const FPRDebugCommandResult& Result) const;
	const FPRDebugCommandDescriptor* FindDescriptor(EPRDebugCommandId CommandId) const;

	TWeakObjectPtr<UPRDebugSubsystem> Owner;
	TArray<FPRDebugCommandDescriptor> Descriptors;
	TMap<FString, double> FloatArguments;
	TMap<FString, FName> ChoiceArguments;
	TMap<FString, TSharedPtr<TArray<TSharedPtr<FName>>>> ChoiceOptions;
	TSharedPtr<STextBlock> ResultText;
};
