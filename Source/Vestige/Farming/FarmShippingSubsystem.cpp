// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmShippingSubsystem.h"

#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmShippingSaveData.h"
#include "Inventory/VestigeWalletComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UFarmShippingSubsystem* UFarmShippingSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFarmShippingSubsystem>() : nullptr;
}

void UFarmShippingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The catalogue gives the items' prices at the sale; the registry holds the save part.
	Catalog = Collection.InitializeDependency<UVestigeContentCatalog>();
	SaveRegistry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	if (SaveRegistry)
	{
		SaveRegistry->RegisterGameOwner(*this);
	}
}

void UFarmShippingSubsystem::Deinitialize()
{
	if (SaveRegistry)
	{
		SaveRegistry->UnregisterGameOwner(*this);
	}
	Super::Deinitialize();
}

FFarmShipResult UFarmShippingSubsystem::ShipFromSlot(UVestigeInventoryComponent& Inventory, int32 Slot)
{
	const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
	if (!Slots.IsValidIndex(Slot))
	{
		return {};
	}

	const UVestigeItemData* Item = Slots[Slot].Item;
	const int32 Count = Slots[Slot].Count;
	if (!Item || Count < 1 || !Item->CanBeSold())
	{
		return {};
	}

	// Removed first: a refused removal ships nothing, and nothing is counted twice.
	if (!Inventory.TryRemoveItemFromSlot(Slot, Count))
	{
		return {};
	}
	AddToShipment(Item->GetPrimaryAssetId(), Count);
	VESTIGE_LOG(this, LogFarm, Verbose, "Shipped {Count} of {Item}.", Count, Item->GetContentId().ToString());
	return {Item, Count};
}

void UFarmShippingSubsystem::AddToShipment(const FPrimaryAssetId& ItemId, int32 Count)
{
	for (FVestigeItemStack& Stack : Shipment)
	{
		if (Stack.ItemId == ItemId)
		{
			Stack.Count = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Stack.Count) + Count, MAX_int32));
			return;
		}
	}
	Shipment.Add({ItemId, Count});
}

int32 UFarmShippingSubsystem::SellShipment(UVestigeWalletComponent& Wallet)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmShippingSubsystem::SellShipment);

	if (Shipment.IsEmpty())
	{
		return 0;
	}

	// The price on the night, whatever it was when the stack went in. The sum is in int64, which a few stacks can't overflow.
	int64 Sum = 0;
	TArray<FString> Sold;
	for (const FVestigeItemStack& Stack : Shipment)
	{
		const UVestigeItemData* Item = Catalog ? Catalog->Find<UVestigeItemData>(Stack.ItemId) : nullptr;
		if (!Item || !Item->CanBeSold())
		{
			VESTIGE_LOG(this, LogFarm, Warning, "{Count} of {Item} in the bin no longer sell, so they pay nothing and are dropped.", Stack.Count,
				Stack.ItemId.PrimaryAssetName.ToString());
			continue;
		}
		Sum += static_cast<int64>(Stack.Count) * Item->GetSellPrice();
		Sold.Add(FString::Printf(TEXT("%s x%d"), *Stack.ItemId.PrimaryAssetName.ToString(), Stack.Count));
	}

	if (Sum > VestigeWallet::MaxMoney)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The sale of {Sum} is above the limit of {Max}, so it pays the limit.", Sum, VestigeWallet::MaxMoney);
	}
	const int32 Total = static_cast<int32>(FMath::Min<int64>(Sum, VestigeWallet::MaxMoney));
	if (Total > 0)
	{
		Wallet.AddMoney(Total);
	}
	Shipment.Reset();
	VESTIGE_LOG(this, LogFarm, Log, "Sold {Stacks} for {Total}.", FString::Join(Sold, TEXT(", ")), Total);
	return Total;
}

void UFarmShippingSubsystem::StartNewGame()
{
	Shipment.Reset();
}

FName UFarmShippingSubsystem::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Shipping"));
	return Key;
}

int32 UFarmShippingSubsystem::GetSaveVersion() const
{
	return FFarmShippingSaveData::Version;
}

FVestigeSaveRecord UFarmShippingSubsystem::WriteSaveRecord() const
{
	FFarmShippingSaveData Data;
	Data.Stacks.Reserve(Shipment.Num());
	for (const FVestigeItemStack& Stack : Shipment)
	{
		Data.Stacks.Add({Stack.ItemId.PrimaryAssetName, Stack.Count});
	}
	return FVestigeSaveRecord::Write(Data);
}

bool UFarmShippingSubsystem::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FFarmShippingSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}

	Shipment.Reset();
	for (const FFarmShippedStackSaveData& Saved : Data.Stacks)
	{
		const FPrimaryAssetId ItemId(UVestigeItemData::AssetType, Saved.ItemId);
		if (Saved.Count < 1 || !Catalog || !Catalog->Find<UVestigeItemData>(ItemId))
		{
			VESTIGE_LOG(this, LogFarm, Warning, "The saved {Count} of {Item} in the bin can't be restored, so it is left out.", Saved.Count,
				Saved.ItemId.ToString());
			++OutLeftOutCount;
			continue;
		}
		AddToShipment(ItemId, Saved.Count);
	}
	return true;
}
