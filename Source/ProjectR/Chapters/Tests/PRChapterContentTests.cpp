// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "UObject/UObjectGlobals.h"

namespace PRChapterAutomation
{
constexpr EAutomationTestFlags ContentTestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRChapterContentLocalDirectiveWhitelistTest,
	"ProjectR.Chapter.Content.LocalDirectiveWhitelist",
	PRChapterAutomation::ContentTestFlags)

bool FPRChapterContentLocalDirectiveWhitelistTest::RunTest(const FString& Parameters)
{
	UPRChapterRuleDataAsset* ValidRule = NewObject<UPRChapterRuleDataAsset>();
	ValidRule->DirectiveId = TEXT("Allocator.ResourceLock");
	ValidRule->ContentId = TEXT("Allocator");
	TestTrue(TEXT("Allocator ResourceLock is an approved local directive"), ValidRule->IsRuleDefinitionValid());

	UPRChapterRuleDataAsset* UnknownRule = NewObject<UPRChapterRuleDataAsset>();
	UnknownRule->DirectiveId = TEXT("Rule.NotAllocator");
	UnknownRule->ContentId = TEXT("Allocator");
	TestFalse(TEXT("Unknown directives are rejected before room or reward execution"), UnknownRule->IsRuleDefinitionValid());

	UPRChapterRuleDataAsset* WrongContentRule = NewObject<UPRChapterRuleDataAsset>();
	WrongContentRule->DirectiveId = TEXT("Allocator.PriceAudit");
	WrongContentRule->ContentId = TEXT("OtherChapter");
	TestFalse(TEXT("A valid directive cannot be reused by another content pack"), WrongContentRule->IsRuleDefinitionValid());

	UPRChapterRoguelikeContentRegistryDataAsset* ChapterRegistry = NewObject<UPRChapterRoguelikeContentRegistryDataAsset>();
	ChapterRegistry->ContentId = TEXT("Allocator");
	TestFalse(TEXT("An incomplete Allocator Registry is rejected before travel"), ChapterRegistry->IsRegistryReady());
	TestTrue(TEXT("Only the derived Chapter Registry permits the allocation-terminal Shop type"), ChapterRegistry->SupportsChapterShopRooms());

	const UPRChapterRoguelikeContentRegistryDataAsset* AuthoredRegistry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Allocator/DA_RoguelikeContentRegistry_Allocator.DA_RoguelikeContentRegistry_Allocator"));
	TestNotNull(TEXT("The fixed Allocator Registry package is available to runtime validation"), AuthoredRegistry);
	if (AuthoredRegistry) TestTrue(TEXT("The authored Registry closes every Room/Event/Reward/Rule reference"), AuthoredRegistry->IsRegistryReady());
	return true;
}

#endif
