// Copyright Epic Games, Inc. All Rights Reserved.
#include "Quests/PRCompanionQuestDialogueProvider.h"
namespace { bool Build(const FName Id, FPRCompanionQuestDialogueLine& Out) { static const TMap<FName, TPair<FGameplayTag, FString>> Lines = { {TEXT("Quest.Axiom.LowProbabilitySample"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false), TEXT("Low probability is still evidence.")}}, {TEXT("Quest.Axiom.ImperfectOptimum"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false), TEXT("An imperfect choice can still be ours.")}}, {TEXT("Quest.Kindle.NoRetreatLine"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false), TEXT("I held the line.")}}, {TEXT("Quest.Kindle.LearnToRetreat"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false), TEXT("Retreat can protect the next fire.")}}, {TEXT("Quest.Null.GarbageCollection"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false), TEXT("Collected. Nothing wasted.")}}, {TEXT("Quest.Null.RememberMe"), {FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false), TEXT("You remembered." )}} }; if (const auto* Found = Lines.Find(Id)) { Out.LineId=Id; Out.CompanionId=Found->Key; Out.Text=FText::FromString(Found->Value); return true;} return false; } }
bool FPRCompanionQuestDialogueProvider::GetCompletionLine(FName QuestId, FPRCompanionQuestDialogueLine& OutLine) { return Build(QuestId, OutLine); }
bool FPRCompanionQuestDialogueProvider::GetEntitlementLine(FName EntitlementId, FPRCompanionQuestDialogueLine& OutLine)
{
	static const TMap<FName, FName> LineEntitlements = {
		{ TEXT("Line:Axiom_ImperfectOptimum"), TEXT("Quest.Axiom.ImperfectOptimum") },
		{ TEXT("Line:Kindle_LearnToRetreat"), TEXT("Quest.Kindle.LearnToRetreat") },
		{ TEXT("Line:Null_RememberMe"), TEXT("Quest.Null.RememberMe") }
	};
	const FName* QuestId = LineEntitlements.Find(EntitlementId);
	if (!QuestId || !Build(*QuestId, OutLine)) return false;
	OutLine.LineId = EntitlementId;
	return true;
}
