// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeCharacterData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeCharacterData"

const FPrimaryAssetType UVestigeCharacterData::AssetType(TEXT("VestigeCharacter"));

#if WITH_EDITOR
EDataValidationResult UVestigeCharacterData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (const TPair<FGameplayTag, int32>& Entry : TrustFromMilestones)
	{
		if (!Entry.Key.IsValid() || Entry.Value < 1)
		{
			Context.AddError(
				LOCTEXT("BadTrustFromMilestone", "Trust From Milestones needs a valid milestone tag and at least 1 point for each entry."));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
