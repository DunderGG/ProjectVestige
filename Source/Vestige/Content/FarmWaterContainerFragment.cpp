// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/FarmWaterContainerFragment.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "FarmWaterContainerFragment"

#if WITH_EDITOR
EDataValidationResult UFarmWaterContainerFragment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (Capacity < 1)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BadCapacity", "The water container fragment's Capacity is {0}. It must be at least 1."), FText::AsNumber(Capacity)));
		Result = EDataValidationResult::Invalid;
	}

	const UVestigeItemData* Item = GetTypedOuter<UVestigeItemData>();
	if (Item && Item->GetMaxStackSize() != 1)
	{
		Context.AddError(LOCTEXT("StackingContainer",
			"An item with a water container fragment must have a Max Stack Size of 1: its water is kept with its slot, which only an item "
			"that doesn't stack has."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
