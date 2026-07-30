// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Headmind/PRHeadmindTypes.h"

struct FPRRunSummary;
struct FPRProgressionSnapshot;

/** Pure, bounded final-chapter ending derivation.  It never reads Save internals. */
class PROJECTR_API FPRHeadmindEndingEvaluator
{
public:
	static bool BuildInput(const FPRRunSummary& Summary, const FPRProgressionSnapshot& Progression, FPRHeadmindEndingInputSnapshot& OutInput);
};
