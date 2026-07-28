// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Quests/PRCompanionQuestDialogueProvider.h"
#include "Quests/PRCompanionQuestDataAsset.h"
#include "Quests/PRCompanionQuestRegistryDataAsset.h"
#include "UI/PRCompanionQuestHubWidget.h"
#include "UI/PRCompanionQuestWidget.h"

#include "Blueprint/UserWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionQuestFoundationTest,
	"ProjectR.CompanionQuest.Foundation.FixedDialogueWhitelist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRCompanionQuestFoundationTest::RunTest(const FString&)
{
	FPRCompanionQuestDialogueLine Line;
	const FName CompletionQuestIds[] = {
		TEXT("Quest.Axiom.LowProbabilitySample"),
		TEXT("Quest.Axiom.ImperfectOptimum"),
		TEXT("Quest.Kindle.NoRetreatLine"),
		TEXT("Quest.Kindle.LearnToRetreat"),
		TEXT("Quest.Null.GarbageCollection"),
		TEXT("Quest.Null.RememberMe")};
	for (const FName QuestId : CompletionQuestIds)
	{
		TestTrue(TEXT("Every fixed quest has one fixed local completion line"), FPRCompanionQuestDialogueProvider::GetCompletionLine(QuestId, Line));
	}
	FPRCompanionQuestDialogueProvider::GetCompletionLine(TEXT("Quest.Axiom.LowProbabilitySample"), Line);
	TestEqual(TEXT("Axiom completion line remains Axiom"), Line.CompanionId, FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false));
	TestTrue(TEXT("Line entitlement maps only to its fixed local dialogue"), FPRCompanionQuestDialogueProvider::GetEntitlementLine(TEXT("Line:Axiom_ImperfectOptimum"), Line));
	TestEqual(TEXT("Entitlement keeps its fixed Axiom speaker"), Line.CompanionId, FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false));
	TestFalse(TEXT("Unknown quest line is refused"), FPRCompanionQuestDialogueProvider::GetCompletionLine(TEXT("Quest.Unknown"), Line));
	TestFalse(TEXT("Unknown entitlement is refused"), FPRCompanionQuestDialogueProvider::GetEntitlementLine(TEXT("Line:Unknown"), Line));

	const UPRCompanionQuestRegistryDataAsset* Registry = LoadObject<UPRCompanionQuestRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Quests/DA_CompanionQuestRegistry.DA_CompanionQuestRegistry"));
	TestNotNull(TEXT("Fixed six-quest Registry reloads"), Registry);
	if (Registry)
	{
		TestTrue(TEXT("Registry is canonical after asset reload"), Registry->IsRegistryReady());
		TestEqual(TEXT("Registry contains exactly six fixed quest assets"), Registry->Quests.Num(), 6);
		const FName ExpectedQuestIds[] = {
			TEXT("Quest.Axiom.ImperfectOptimum"),
			TEXT("Quest.Axiom.LowProbabilitySample"),
			TEXT("Quest.Kindle.LearnToRetreat"),
			TEXT("Quest.Kindle.NoRetreatLine"),
			TEXT("Quest.Null.GarbageCollection"),
			TEXT("Quest.Null.RememberMe")};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedQuestIds); ++Index)
		{
			TestNotNull(TEXT("Registry fixed quest reference reloads"), Registry->Quests[Index].LoadSynchronous());
			if (const UPRCompanionQuestDataAsset* Quest = Registry->Quests[Index].Get())
			{
				TestEqual(TEXT("Registry preserves fixed lexical QuestId order"), Quest->QuestId, ExpectedQuestIds[Index]);
			}
		}
	}

	const UClass* RootClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot.WBP_RealityHubRoot_C"));
	const UClass* TerminalClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubCompanionTerminal.WBP_RealityHubCompanionTerminal_C"));
	TestNotNull(TEXT("Reality Hub Root Blueprint reloads"), RootClass);
	TestNotNull(TEXT("Companion Terminal Blueprint reloads"), TerminalClass);
	if (RootClass)
	{
		TestTrue(TEXT("Root remains the fixed native Hub quest presentation seam"), RootClass->IsChildOf(UPRCompanionQuestHubWidget::StaticClass()));
	}
	if (TerminalClass)
	{
		TestTrue(TEXT("Terminal remains the fixed native quest presentation seam"), TerminalClass->IsChildOf(UPRCompanionQuestWidget::StaticClass()));
	}
	return true;
}

#endif
