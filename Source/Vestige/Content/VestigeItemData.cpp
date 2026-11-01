// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeItemData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeItemData"

const FPrimaryAssetType UVestigeItemData::AssetType(TEXT("VestigeItem"));

const UVestigeItemFragment* UVestigeItemData::FindFragmentByClass(TSubclassOf<UVestigeItemFragment> FragmentClass) const
{
	if (!FragmentClass)
	{
		return nullptr;
	}
	for (const UVestigeItemFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UVestigeItemData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (MaxStackSize < 1)
	{
		Context.AddError(
			FText::Format(LOCTEXT("BadMaxStackSize", "Max Stack Size is {0}. It must be at least 1; use 1 for an item that doesn't stack."),
				FText::AsNumber(MaxStackSize)));
		Result = EDataValidationResult::Invalid;
	}

	TSet<const UClass*> FragmentClasses;
	for (const UVestigeItemFragment* Fragment : Fragments)
	{
		if (!Fragment)
		{
			Context.AddError(LOCTEXT("EmptyFragment", "Fragments has an empty entry. Pick a fragment class for it or remove it."));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		bool bAlreadyHasClass = false;
		FragmentClasses.Add(Fragment->GetClass(), &bAlreadyHasClass);
		if (bAlreadyHasClass)
		{
			// Only the first fragment of a class would ever be found, so the second one's settings would silently do nothing.
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateFragment", "Fragments has more than one {0}. Keep one."), Fragment->GetClass()->GetDisplayNameText()));
			Result = EDataValidationResult::Invalid;
		}
		Result = CombineDataValidationResults(Result, Fragment->IsDataValid(Context));
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
