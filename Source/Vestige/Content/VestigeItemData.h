// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "Templates/SubclassOf.h"
#include "VestigeItemData.generated.h"

/**
 * One aspect that only some items have, such as the crop a seed plants. Added to an item asset's Fragments list,
 * so a new kind of item is a new fragment class, never a subclass of UVestigeItemData. Holds no runtime state.
 * BlueprintType, as every data asset already is, so editor Python scripts that make content can create fragments.
 */
UCLASS(Abstract, BlueprintType, DefaultToInstanced, EditInlineNew)
class UVestigeItemFragment : public UObject
{
	GENERATED_BODY()
};

/**
 * The definition of one item the player can carry: seeds, harvested crops, materials. Owns what every item has (stack size,
 * sell price) and the fragments for what only some items have, plus the ID and name every content asset has.
 * One asset per item, found through the Asset Manager by its ContentId. Holds no runtime state.
 */
UCLASS()
class UVestigeItemData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type items are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	int32 GetMaxStackSize() const { return MaxStackSize; }
	int32 GetSellPrice() const { return SellPrice; }

	/** The item's fragment of class FragmentClass or a subclass of it, or nullptr if it has none. */
	const UVestigeItemFragment* FindFragmentByClass(TSubclassOf<UVestigeItemFragment> FragmentClass) const;

	/** The item's fragment of type T or a subclass of it, or nullptr if it has none. */
	template <typename T> const T* FindFragment() const { return Cast<T>(FindFragmentByClass(T::StaticClass())); }

#if WITH_EDITOR
	/** Reports a stack size below one, empty fragment entries, two fragments of one class, and each fragment's own mistakes. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Items get their stack size and fragments in the editor. */
	void SetMaxStackSizeForTests(int32 InMaxStackSize) { MaxStackSize = InMaxStackSize; }
	void AddFragmentForTests(UVestigeItemFragment* Fragment) { Fragments.Add(Fragment); }
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** How many of this item fit in one inventory slot. 1 for items that don't stack. */
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 MaxStackSize = 99;

	/** What selling one of this item pays. Not used yet: there is no currency. */
	UPROPERTY(EditDefaultsOnly, Category = "Economy", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 SellPrice = 0;

	/** What only some items have, one fragment per aspect (Farm Seed Fragment makes the item a seed). At most one of each class. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Item", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UVestigeItemFragment>> Fragments;
};
