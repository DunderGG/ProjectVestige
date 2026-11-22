// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"

class UVestigeInventoryComponent;
class UVestigeWalletComponent;
struct FVestigeShopOffer;

/** How a purchase went: bought, or the first reason it wasn't. */
enum class EVestigeBuyResult : uint8
{
	Bought,
	/** The count or the offer's price is below 1. */
	InvalidOffer,
	/** The bag has no room for the items. */
	NoRoom,
	/** The purse holds less than the cost. */
	NotEnoughMoney,
};

/** The buying rule every shop uses, a dispenser now and a shop screen later (Systems.md, section 26). */
namespace VestigeShop
{
	/**
	 * Buys Count of Offer's item: checks the bag first, then the purse, and only then spends the cost (Price times Count) and
	 * adds the items. All or nothing: a refusal changes neither. Instant and free; needs no world.
	 */
	EVestigeBuyResult Buy(
		UVestigeInventoryComponent& Inventory, UVestigeWalletComponent& Wallet, const FVestigeShopOffer& Offer, int32 Count);
}
