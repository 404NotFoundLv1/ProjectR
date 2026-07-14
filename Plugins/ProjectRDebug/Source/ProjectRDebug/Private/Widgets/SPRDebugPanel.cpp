// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widgets/SPRDebugPanel.h"

#include "PRDebugSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPRDebugPanel::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	Descriptors = InArgs._Descriptors;
	Descriptors.Sort([](const FPRDebugCommandDescriptor& Left, const FPRDebugCommandDescriptor& Right)
	{
		return static_cast<uint8>(Left.CommandId) < static_cast<uint8>(Right.CommandId);
	});

	TSharedRef<SVerticalBox> CommandList = SNew(SVerticalBox);
	for (const FPRDebugCommandDescriptor& Descriptor : Descriptors)
	{
		CommandList->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			BuildCommandWidget(Descriptor)
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SBox)
			.WidthOverride(520.0f)
			.MaxDesiredHeight(680.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ProjectR Debug")))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.AccessibleText(FText::FromString(TEXT("Close ProjectR Debug")))
						.Text(FText::FromString(TEXT("Close")))
						.OnClicked(this, &SPRDebugPanel::HandleClose)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 8.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						CommandList
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f)
				[
					SAssignNew(ResultText, STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(TEXT("Ready.")))
				]
			]
		]
	];
}

FString SPRDebugPanel::MakeArgumentKey(EPRDebugCommandId CommandId, FName ArgumentId)
{
	return FString::Printf(TEXT("%d:%s"), static_cast<int32>(CommandId), *ArgumentId.ToString());
}

TSharedRef<SWidget> SPRDebugPanel::BuildCommandWidget(const FPRDebugCommandDescriptor& Descriptor)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(Descriptor.DisplayName)
	];
	Box->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Text(Descriptor.Description)
	];

	for (const FPRDebugArgumentSpec& Argument : Descriptor.Arguments)
	{
		const FString Key = MakeArgumentKey(Descriptor.CommandId, Argument.ArgumentId);
		const FText AccessibleText = FText::FromString(FString::Printf(
			TEXT("Argument %s.%s"),
			*Descriptor.StableName.ToString(),
			*Argument.ArgumentId.ToString()));
		if (Argument.Type == EPRDebugArgumentType::Float)
		{
			FloatArguments.Add(Key, Argument.DefaultValue);
			Box->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(SNumericEntryBox<double>)
				.AccessibleText(AccessibleText)
				.MinValue(Argument.MinValue)
				.MaxValue(Argument.MaxValue)
				.Value_Lambda([this, Key]() -> TOptional<double>
				{
					return FloatArguments.FindRef(Key);
				})
				.OnValueChanged_Lambda([this, Key](double Value)
				{
					FloatArguments.Add(Key, Value);
				})
			];
		}
		else if (Argument.Type == EPRDebugArgumentType::EnumChoice)
		{
			TSharedPtr<TArray<TSharedPtr<FName>>> Options = MakeShared<TArray<TSharedPtr<FName>>>();
			for (const FName Choice : Argument.AllowedChoices)
			{
				Options->Add(MakeShared<FName>(Choice));
			}
			ChoiceOptions.Add(Key, Options);
			const TSharedPtr<FName> Initial = Options->IsEmpty() ? nullptr : (*Options)[0];
			ChoiceArguments.Add(Key, Initial.IsValid() ? *Initial : NAME_None);
			Box->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(SComboBox<TSharedPtr<FName>>)
				.AccessibleText(AccessibleText)
				.OptionsSource(Options.Get())
				.InitiallySelectedItem(Initial)
				.OnGenerateWidget_Lambda([](TSharedPtr<FName> Item)
				{
					return SNew(STextBlock).Text(FText::FromName(Item.IsValid() ? *Item : NAME_None));
				})
				.OnSelectionChanged_Lambda([this, Key](TSharedPtr<FName> Item, ESelectInfo::Type)
				{
					ChoiceArguments.Add(Key, Item.IsValid() ? *Item : NAME_None);
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this, Key]()
					{
						return FText::FromName(ChoiceArguments.FindRef(Key));
					})
				]
			];
		}
	}

	Box->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 4.0f)
	[
		SNew(SButton)
		.AccessibleText(FText::FromString(FString::Printf(TEXT("Execute %s"), *Descriptor.StableName.ToString())))
		.Text(FText::FromString(FString::Printf(TEXT("Execute %s"), *Descriptor.StableName.ToString())))
		.OnClicked(this, &SPRDebugPanel::HandleExecute, Descriptor.CommandId)
	];
	return SNew(SBorder).Padding(6.0f)[Box];
}

FReply SPRDebugPanel::HandleExecute(EPRDebugCommandId CommandId)
{
	UPRDebugSubsystem* Subsystem = Owner.Get();
	const FPRDebugCommandDescriptor* Descriptor = FindDescriptor(CommandId);
	if (Subsystem == nullptr || Descriptor == nullptr)
	{
		return FReply::Handled();
	}

	const FPRDebugCommandResult Result = Subsystem->ExecutePanelCommand(BuildRequest(*Descriptor));
	FText DisplayResult = FormatResult(Result);
	if (Descriptor->bChangesRuntimeState && CommandId != EPRDebugCommandId::ReturnToCombatGym)
	{
		FPRDebugCommandRequest StatusRequest;
		StatusRequest.CommandId = EPRDebugCommandId::StatusSnapshot;
		const FPRDebugCommandResult Status = Subsystem->ExecutePanelCommand(StatusRequest);
		DisplayResult = FText::FromString(FString::Printf(
			TEXT("%s\n%s"),
			*DisplayResult.ToString(),
			*FormatResult(Status).ToString()));
	}
	if (ResultText.IsValid())
	{
		ResultText->SetText(DisplayResult);
	}
	return FReply::Handled();
}

FReply SPRDebugPanel::HandleClose()
{
	if (UPRDebugSubsystem* Subsystem = Owner.Get())
	{
		Subsystem->ClosePanel();
	}
	return FReply::Handled();
}

FPRDebugCommandRequest SPRDebugPanel::BuildRequest(const FPRDebugCommandDescriptor& Descriptor) const
{
	FPRDebugCommandRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.CommandId = Descriptor.CommandId;
	Request.SchemaVersion = Descriptor.SchemaVersion;
	for (const FPRDebugArgumentSpec& Argument : Descriptor.Arguments)
	{
		FPRDebugArgumentValue Value;
		Value.ArgumentId = Argument.ArgumentId;
		Value.Type = Argument.Type;
		const FString Key = MakeArgumentKey(Descriptor.CommandId, Argument.ArgumentId);
		if (Argument.Type == EPRDebugArgumentType::Float)
		{
			Value.FloatValue = FloatArguments.FindRef(Key);
		}
		else if (Argument.Type == EPRDebugArgumentType::EnumChoice)
		{
			Value.ChoiceValue = ChoiceArguments.FindRef(Key);
		}
		Request.Arguments.Add(Value);
	}
	return Request;
}

FText SPRDebugPanel::FormatResult(const FPRDebugCommandResult& Result) const
{
	FString Text = FString::Printf(
		TEXT("Result %d: %s"),
		static_cast<int32>(Result.ResultCode),
		*Result.PlayerMessage.ToString());
	if (Result.bHasStatusSnapshot)
	{
		const FPRDebugStatusSnapshot& Snapshot = Result.StatusSnapshot;
		Text += FString::Printf(
			TEXT("\nMap=%s LocalPlayer=%s Pawn=%s ASC=%s Health=%.1f Shield=%.1f Energy=%.1f Abilities=%d/%d Effects=%d Save=%d Schema=%d Revision=%lld"),
			*Snapshot.MapShortName.ToString(),
			Snapshot.bHasLocalPlayer ? TEXT("true") : TEXT("false"),
			Snapshot.bHasPawn ? TEXT("true") : TEXT("false"),
			Snapshot.bHasAbilitySystem ? TEXT("true") : TEXT("false"),
			Snapshot.Health,
			Snapshot.Shield,
			Snapshot.Energy,
			Snapshot.ActiveAbilityCount,
			Snapshot.GrantedAbilityCount,
			Snapshot.ActiveEffectCount,
			static_cast<int32>(Snapshot.SaveState),
			Snapshot.SaveSchemaVersion,
			Snapshot.SaveRevision);
	}
	return FText::FromString(Text);
}

const FPRDebugCommandDescriptor* SPRDebugPanel::FindDescriptor(EPRDebugCommandId CommandId) const
{
	return Descriptors.FindByPredicate([CommandId](const FPRDebugCommandDescriptor& Descriptor)
	{
		return Descriptor.CommandId == CommandId;
	});
}
