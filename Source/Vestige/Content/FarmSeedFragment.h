// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeItemData.h"
#include "FarmSeedFragment.generated.h"

/** Makes an item a seed: planting it grows the crop CropId names. */
UCLASS()
class UFarmSeedFragment : public UVestigeItemFragment
{
	GENERATED_BODY()

public:
	/** The crop planting this seed grows (FarmCrop:Base.Turnip). */
	const FPrimaryAssetId& GetCropId() const { return CropId; }

#if WITH_EDITOR
	/** Reports a CropId that is empty, of another content type, or names no crop. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Seeds get their crop in the editor. */
	void SetCropIdForTests(const FPrimaryAssetId& InCropId) { CropId = InCropId; }
#endif

private:
	/** The crop planting this seed grows, by ID so the seed doesn't load the crop's meshes. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "FarmCrop"))
	FPrimaryAssetId CropId;
};
