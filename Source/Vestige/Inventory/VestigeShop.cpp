// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Inventory/VestigeShop.h"

#include "Content/VestigeShopData.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventoryLog.h"
#include "Inventory/VestigeWalletComponent.h"

namespace VestigeShop
{
	EVestigeBuyResult Buy(
		UVestigeInventoryComponent& Inventory, UVestigeWalletComponent& Wallet, const FVestigeShopOffer& Offer, int32 Count)
	{
		const FString ItemName = Offer.ItemId.ToString();
		if (Count < 1 || Offer.Price < 1)
		{
			VESTIGE_LOG(&Inventory, LogVestigeInventory, Verbose, "Not buying {Count} {ItemId} at {Price}: an invalid offer.", Count,
				ItemName, Offer.Price);
			return EVestigeBuyResult::InvalidOffer;
		}

		// The bag before the purse, so a full bag never costs coins (GameDesign.md, section 9).
		if (!Inventory.CanAddItem(Offer.ItemId, Count))
		{
			VESTIGE_LOG(&Inventory, LogVestigeInventory, Verbose, "Not buying {Count} {ItemId}: no room in the bag.", Count, ItemName);
			return EVestigeBuyResult::NoRoom;
		}

		// Never clamped: a cost above the purse limit must stay above the purse, not be sold for the limit.
		const int64 Cost = static_cast<int64>(Offer.Price) * Count;
		if (Cost > Wallet.GetMoney())
		{
			VESTIGE_LOG(&Inventory, LogVestigeInventory, Verbose,
				"Not buying {Count} {ItemId}: it costs {Cost} and the wallet holds {Money}.", Count, ItemName, Cost, Wallet.GetMoney());
			return EVestigeBuyResult::NotEnoughMoney;
		}

		// The cost is at most the purse here, so it fits an int32.
		const int32 Spent = static_cast<int32>(Cost);
		if (!Wallet.SpendMoney(Spent))
		{
			return EVestigeBuyResult::NotEnoughMoney;
		}
		if (!Inventory.AddItem(Offer.ItemId, Count))
		{
			// Both were checked, so this is broken state; give the coins back so the player never pays for nothing.
			Wallet.AddMoney(Spent);
			VESTIGE_LOG(&Inventory, LogVestigeInventory, Error, "Couldn't add {Count} {ItemId} after the check, so {Cost} was given back.",
				Count, ItemName, Spent);
			return EVestigeBuyResult::NoRoom;
		}

		VESTIGE_LOG(&Inventory, LogVestigeInventory, Log, "Bought {Count} {ItemId} for {Cost}, so the wallet holds {Money}.", Count,
			ItemName, Spent, Wallet.GetMoney());
		return EVestigeBuyResult::Bought;
	}
}
