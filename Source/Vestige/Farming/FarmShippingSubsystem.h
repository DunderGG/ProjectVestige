// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveable.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FarmShippingSubsystem.generated.h"

class UVestigeContentCatalog;
class UVestigeItemData;
class UVestigeMilestoneSubsystem;
class UVestigeSaveRegistry;
class UVestigeWalletComponent;

/** What ShipFromSlot shipped: the item and how many of it, or no item and 0 when nothing was shipped. */
struct FFarmShipResult
{
	const UVestigeItemData* Item = nullptr;
	int32 Count = 0;
};

/**
 * What is in the shipping bin until the night: the shipment, one entry per item. A game-instance subsystem rather than the
 * bin's state, so it outlives the map, a second bin or a robot that ships adds to the same shipment, and the night can pass
 * in a map without a bin. It takes the stack in hand from an inventory, sells the shipment at each item's sell price when
 * the game mode says the night has come, and pays the wallet. It reports nothing to the player: the bin tells of a shipment,
 * the game mode of the sale. Saves the shipment under Base.Shipping through UVestigeSaveRegistry; loading sells nothing.
 * Systems.md, section 25.
 */
UCLASS()
class UFarmShippingSubsystem : public UGameInstanceSubsystem, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** The shipment of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UFarmShippingSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Moves the whole stack in Slot of Inventory into the shipment, merged with what is there of the same item. Another stack
	 * of the item elsewhere in the bag stays. An empty slot or an item that can't be sold ships nothing and changes nothing.
	 */
	FFarmShipResult ShipFromSlot(UVestigeInventoryComponent& Inventory, int32 Slot);

	/** What is in the bin, one entry per item. */
	const TArray<FVestigeItemStack>& GetShippedStacks() const { return Shipment; }

	/**
	 * Sells the shipment at each item's sell price now, pays Wallet and empties the bin. Returns the total paid: 0 for an empty
	 * bin, which changes nothing. An item that no longer exists or no longer sells is dropped with a warning and pays nothing.
	 */
	int32 SellShipment(UVestigeWalletComponent& Wallet);

	/** Empties the bin. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

private:
	/** Adds Count of the item to the shipment, merged into its entry. */
	void AddToShipment(const FPrimaryAssetId& ItemId, int32 Count);

	TArray<FVestigeItemStack> Shipment;

	UPROPERTY()
	TObjectPtr<UVestigeContentCatalog> Catalog;

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> SaveRegistry;

	/** The first sale is reached here. */
	UPROPERTY()
	TObjectPtr<UVestigeMilestoneSubsystem> Milestones;
};
