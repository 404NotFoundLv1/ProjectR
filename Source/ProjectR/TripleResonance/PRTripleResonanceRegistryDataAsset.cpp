// Copyright ProjectR. All Rights Reserved.

#include "TripleResonance/PRTripleResonanceRegistryDataAsset.h"

#include "Misc/DataValidation.h"
#include "QTE/PRQTEDataAsset.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

#if WITH_EDITOR
EDataValidationResult UPRTripleResonanceRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	if (Definition.IsNull() || ExternalQTEs.Num() != FPRTripleResonanceContract::GetExternalQTEIds().Num())
	{
		Context.AddError(FText::FromString(TEXT("Triple Resonance registry must contain one definition and exactly three external QTEs."))); return EDataValidationResult::Invalid;
	}
	for (int32 Index = 0; Index < ExternalQTEs.Num(); ++Index)
	{
		const UPRQTEDataAsset* Asset = ExternalQTEs[Index].LoadSynchronous();
		if (!Asset || !Asset->bResultOnly || Asset->QTEId != FPRTripleResonanceContract::GetExternalQTEIds()[Index])
		{
			Context.AddError(FText::FromString(TEXT("Triple Resonance QTE registry entries must be fixed, ordered ResultOnly assets."))); return EDataValidationResult::Invalid;
		}
	}
	return EDataValidationResult::Valid;
}
#endif
