// Copyright ProjectR. All Rights Reserved.

#include "TripleResonance/PRTripleResonanceDataAsset.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UPRTripleResonanceDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	const bool bValid = !AbilityClass.IsNull() && !ChargeEffect.IsNull() && !CostEffect.IsNull() && !CooldownEffect.IsNull()
		&& FMath::IsNearlyEqual(Damage, 120.0f) && FMath::IsNearlyEqual(ExecutionHealthFraction, 0.20f);
	if (!bValid) { Context.AddError(FText::FromString(TEXT("Triple Resonance must use its four fixed assets, 120 damage, and the 20% execution threshold."))); return EDataValidationResult::Invalid; }
	return EDataValidationResult::Valid;
}
#endif
