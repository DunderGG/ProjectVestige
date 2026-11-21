// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeItemData.h"
#include "FarmWaterContainerFragment.generated.h"

/**
 * Makes an item a water container, as the watering can is: it holds up to Capacity waterings. How much it holds now is a stat
 * of its inventory slot (Vestige.ItemStat.Water.Clean), so this fragment holds no runtime state. Systems.md, section 24.
 */
UCLASS()
class UFarmWaterContainerFragment : public UVestigeItemFragment
{
	GENERATED_BODY()

public:
	/** How many waterings the container holds when full. */
	int32 GetCapacity() const { return Capacity; }

#if WITH_EDITOR
	/** Reports a capacity below 1, and an item that stacks (the water is a stat of a slot, which only one item has). */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Items get their capacity in the editor. */
	void SetCapacityForTests(int32 InCapacity) { Capacity = InCapacity; }
#endif

private:
	/** How many waterings the container holds when full. */
	UPROPERTY(EditDefaultsOnly, Category = "Water", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Capacity = 8;
};
