// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/FarmTileState.h"
#include "GameFramework/Actor.h"
#include "FarmTile.generated.h"

class UFarmCropData;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
struct FFarmTileSaveData;

/**
 * One square of farmland, in the world: it can be tilled, planted, watered and harvested.
 * Its rules and state are an FFarmTileState; the actor applies them and shows the result with its meshes.
 */
UCLASS()
class AFarmTile : public AActor
{
	GENERATED_BODY()

public:
	/** Width of a tile at scale 1, in cm. The grid scales tiles from this to its own tile size. */
	static constexpr float UnscaledSize = 100.f;

	AFarmTile();

	// The farming actions follow FFarmTileState's rules, documented there, and return false when they change nothing.
	bool Till();
	bool Plant(const UFarmCropData* InCrop);
	bool Water();
	bool Harvest();

	/** Called once per in-game day. Watered crops grow one stage, then the soil dries out. */
	void AdvanceDay();

	EFarmTileState GetState() const { return TileState.GetState(); }
	bool IsWatered() const { return TileState.IsWatered(); }
	bool IsRipe() const { return TileState.IsRipe(); }
	bool CanTill() const { return TileState.CanTill(); }
	bool CanPlant() const { return TileState.CanPlant(); }
	bool CanWater() const { return TileState.CanWater(); }
	int32 GetGrowthDays() const { return TileState.GetGrowthDays(); }

	/** The crop growing here, or nullptr if nothing is planted. */
	const UFarmCropData* GetCrop() const { return TileState.GetCrop(); }

	/** The tile's state and its crop's ContentId, for saving. The grid adds the tile's coordinates. */
	FFarmTileSaveData GetSaveData() const;

	/**
	 * Puts the tile straight into a saved state, as FFarmTileState::Restore does, and shows it. SavedCrop is the crop
	 * Data.CropId names, already loaded, or nullptr if there is none or it no longer exists.
	 */
	void RestoreFromSaveData(const FFarmTileSaveData& Data, const UFarmCropData* SavedCrop);

protected:
	virtual void BeginPlay() override;

private:
	/** Shows the state again if bChanged, and returns bChanged, so each action stays one line. */
	bool RefreshIfChanged(bool bChanged);

	void RefreshVisuals();
	void RefreshSoil();
	void RefreshCrop();

	/** The placeholder soil's color for the current state: grass, dry soil or watered soil. */
	FLinearColor GetPlaceholderSoilColor() const;

	// Optional art. Anything left empty falls back to the colored placeholder shapes.

	/** Mesh for tilled, planted and watered soil. Origin at the bottom center, UnscaledSize wide. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> TilledSoilMesh;

	/** Material swapped onto TilledSoilMesh while the soil is watered. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> WateredSoilMaterial;

	/** Height of the soil surface that custom crop meshes stand on, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float SoilSurfaceHeight = 6.f;

	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UStaticMeshComponent> SoilMesh;

	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UStaticMeshComponent> CropMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaceholderSoilMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaceholderCropMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SoilMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CropMaterial;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FFarmTileState TileState;
};
