// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "VestigeShopData.generated.h"

/**
 * One thing a shop sells: an item and the price of one. BlueprintType so editor scripts can make it; Blueprints don't read it.
 * The price is the shop's, not the item's: two shops may price one item differently (Systems.md, section 26).
 */
USTRUCT(BlueprintType)
struct FVestigeShopOffer
{
	GENERATED_BODY()

	/** The item sold. */
	UPROPERTY(EditAnywhere, Category = "Shop", meta = (AllowedTypes = "VestigeItem"))
	FPrimaryAssetId ItemId;

	/** What one of the item costs, in coins. */
	UPROPERTY(EditAnywhere, Category = "Shop", meta = (ClampMin = 1))
	int32 Price = 1;
};

/**
 * The definition of one shop: its stock, a list of offers, plus the ID and name every content asset has. One asset per shop
 * (DA_Shop_Farm is Base.FarmShop), found through the Asset Manager by its ContentId. Holds no runtime state: the stock is
 * unlimited and the prices fixed. Systems.md, section 26.
 */
UCLASS()
class UVestigeShopData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type shops are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	/** Every offer, in the order a shop screen shows them. */
	const TArray<FVestigeShopOffer>& GetOffers() const { return Offers; }

	/** The offer for ItemId, or nullptr when the shop doesn't sell it. */
	const FVestigeShopOffer* FindOffer(const FPrimaryAssetId& ItemId) const;

#if WITH_EDITOR
	/** Reports an offer whose item isn't a known item, a price below 1, and an item offered twice. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Shops get their offers in the editor. */
	void AddOfferForTests(const FVestigeShopOffer& Offer) { Offers.Add(Offer); }
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** What the shop sells, one offer per item, in the order a shop screen shows them. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop", meta = (AllowPrivateAccess = "true", TitleProperty = "ItemId"))
	TArray<FVestigeShopOffer> Offers;
};
