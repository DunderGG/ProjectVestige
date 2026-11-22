// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeShopData.h"

#include "Content/VestigeItemData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeShopData"

const FPrimaryAssetType UVestigeShopData::AssetType(TEXT("VestigeShop"));

const FVestigeShopOffer* UVestigeShopData::FindOffer(const FPrimaryAssetId& ItemId) const
{
	return Offers.FindByPredicate(
		[&ItemId](const FVestigeShopOffer& Offer)
		{
			return Offer.ItemId == ItemId;
		});
}

#if WITH_EDITOR
EDataValidationResult UVestigeShopData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<FPrimaryAssetId> OfferedItems;
	for (int32 Index = 0; Index < Offers.Num(); ++Index)
	{
		const FVestigeShopOffer& Offer = Offers[Index];
		const FText OfferNumber = FText::AsNumber(Index);
		if (const TOptional<FText> Error = UVestigeContentData::FindContentReferenceError(Offer.ItemId, UVestigeItemData::AssetType))
		{
			Context.AddError(FText::Format(LOCTEXT("BadItemId", "Offer {0}'s Item Id {1}"), OfferNumber, *Error));
			Result = EDataValidationResult::Invalid;
		}
		if (Offer.Price < 1)
		{
			Context.AddError(FText::Format(
				LOCTEXT("BadPrice", "Offer {0}'s Price is {1}. It must be at least 1 coin."), OfferNumber, FText::AsNumber(Offer.Price)));
			Result = EDataValidationResult::Invalid;
		}

		bool bAlreadyOffered = false;
		OfferedItems.Add(Offer.ItemId, &bAlreadyOffered);
		if (bAlreadyOffered && Offer.ItemId.IsValid())
		{
			// Only the first offer of an item would ever be found, so the second one's price would silently do nothing.
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateOffer", "Offers has {0} more than once. Keep one."), FText::FromString(Offer.ItemId.ToString())));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
