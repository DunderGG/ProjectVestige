// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTile.h"

#include "Components/StaticMeshComponent.h"
#include "Content/FarmCropData.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmField.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace FarmTileColors
{
	// Placeholder colors until the Blender assets replace these shapes.
	const FLinearColor Grass(0.15f, 0.45f, 0.10f);
	const FLinearColor Tilled(0.35f, 0.20f, 0.08f);
	const FLinearColor Watered(0.12f, 0.07f, 0.03f);
	const FLinearColor Growing(0.20f, 0.70f, 0.15f);
	const FLinearColor Ripe(1.00f, 0.60f, 0.05f);
}

namespace FarmTilePlaceholder
{
	// The engine cube is AFarmTile::UnscaledSize wide, so this makes a thin 95x95 cm slab with a small gap between tiles.
	const FVector SoilScale(0.95f, 0.95f, 0.04f);
	const FVector SoilLocation(0.f, 0.f, 2.f);
	constexpr float SoilTop = 4.f;

	// The engine cylinder is 100 units tall and centered on its origin. It grows taller each stage.
	constexpr float CropWidthScale = 0.3f;
	constexpr float CropSeedlingHeightScale = 0.15f;
	constexpr float CropHeightScalePerDay = 0.25f;
	constexpr float CylinderHalfHeight = 50.f;

	/** Vector parameter of the engine's basic shape material that sets its color. */
	const FName ColorParameter(TEXT("Color"));
}

AFarmTile::AFarmTile()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	PlaceholderSoilMesh = CubeMesh.Object;
	PlaceholderCropMesh = CylinderMesh.Object;

	// Both meshes are swapped and moved during play, which static components aren't allowed to do.
	SoilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoilMesh"));
	SoilMesh->SetupAttachment(RootComponent);
	SoilMesh->SetMobility(EComponentMobility::Movable);
	SoilMesh->SetStaticMesh(PlaceholderSoilMesh);
	SoilMesh->SetMaterial(0, ShapeMaterial.Object);
	SoilMesh->SetRelativeScale3D(FarmTilePlaceholder::SoilScale);
	SoilMesh->SetRelativeLocation(FarmTilePlaceholder::SoilLocation);
	SoilMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SoilMesh->SetGenerateOverlapEvents(false);

	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	CropMesh->SetupAttachment(RootComponent);
	CropMesh->SetMobility(EComponentMobility::Movable);
	CropMesh->SetStaticMesh(PlaceholderCropMesh);
	CropMesh->SetMaterial(0, ShapeMaterial.Object);
	CropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CropMesh->SetGenerateOverlapEvents(false);
	CropMesh->SetVisibility(false);
}

void AFarmTile::BeginPlay()
{
	Super::BeginPlay();

	SoilMaterial = SoilMesh->CreateDynamicMaterialInstance(0);
	CropMaterial = CropMesh->CreateDynamicMaterialInstance(0);
	Refresh();
}

void AFarmTile::ShowFieldTile(UFarmField& InField, const FIntPoint& InCoordinates)
{
	Field = &InField;
	Coordinates = InCoordinates;
	Refresh();
}

const FFarmTileState& AFarmTile::GetTileState() const
{
	static const FFarmTileState Grass;
	const UFarmField* CurrentField = Field.Get();
	const FFarmTileState* Tile = CurrentField ? CurrentField->GetTile(Coordinates) : nullptr;
	return Tile ? *Tile : Grass;
}

bool AFarmTile::Till()
{
	UFarmField* CurrentField = Field.Get();
	return CurrentField && CurrentField->Till(Coordinates);
}

bool AFarmTile::Plant(const UFarmCropData* InCrop)
{
	UFarmField* CurrentField = Field.Get();
	return CurrentField && CurrentField->Plant(Coordinates, InCrop);
}

bool AFarmTile::Water()
{
	UFarmField* CurrentField = Field.Get();
	return CurrentField && CurrentField->Water(Coordinates);
}

bool AFarmTile::Harvest()
{
	UFarmField* CurrentField = Field.Get();
	return CurrentField && CurrentField->Harvest(Coordinates);
}

void AFarmTile::Refresh()
{
	RefreshSoil();
	RefreshCrop();
}

void AFarmTile::RefreshSoil()
{
	if (GetState() != EFarmTileState::Grass && TilledSoilMesh)
	{
		SoilMesh->SetStaticMesh(TilledSoilMesh);
		SoilMesh->SetRelativeScale3D(FVector::OneVector);
		SoilMesh->SetRelativeLocation(FVector::ZeroVector);

		// Use the mesh's own materials, or the watered material on every slot.
		SoilMesh->EmptyOverrideMaterials();
		if (IsWatered() && WateredSoilMaterial)
		{
			for (int32 Index = 0; Index < SoilMesh->GetNumMaterials(); ++Index)
			{
				SoilMesh->SetMaterial(Index, WateredSoilMaterial);
			}
		}
		return;
	}

	SoilMesh->SetStaticMesh(PlaceholderSoilMesh);
	SoilMesh->SetRelativeScale3D(FarmTilePlaceholder::SoilScale);
	SoilMesh->SetRelativeLocation(FarmTilePlaceholder::SoilLocation);
	if (SoilMaterial)
	{
		SoilMaterial->SetVectorParameterValue(FarmTilePlaceholder::ColorParameter, GetPlaceholderSoilColor());
		SoilMesh->SetMaterial(0, SoilMaterial);
	}
}

FLinearColor AFarmTile::GetPlaceholderSoilColor() const
{
	if (GetState() == EFarmTileState::Grass)
	{
		return FarmTileColors::Grass;
	}
	return IsWatered() ? FarmTileColors::Watered : FarmTileColors::Tilled;
}

void AFarmTile::RefreshCrop()
{
	const bool bHasCrop = GetState() == EFarmTileState::Planted;
	CropMesh->SetVisibility(bHasCrop);
	if (!bHasCrop)
	{
		return;
	}

	const float SurfaceZ = TilledSoilMesh ? SoilSurfaceHeight : FarmTilePlaceholder::SoilTop;
	const UFarmCropData* Crop = GetCrop();
	UStaticMesh* StageMesh = Crop ? Crop->GetGrowthStageMesh(GetGrowthDays()) : nullptr;
	if (StageMesh)
	{
		CropMesh->SetStaticMesh(StageMesh);
		CropMesh->EmptyOverrideMaterials();
		CropMesh->SetRelativeScale3D(FVector::OneVector);
		CropMesh->SetRelativeLocation(FVector(0.f, 0.f, SurfaceZ));
		return;
	}

	using namespace FarmTilePlaceholder;
	const float HeightScale = CropSeedlingHeightScale + CropHeightScalePerDay * GetGrowthDays();
	CropMesh->SetStaticMesh(PlaceholderCropMesh);
	CropMesh->SetRelativeScale3D(FVector(CropWidthScale, CropWidthScale, HeightScale));
	CropMesh->SetRelativeLocation(FVector(0.f, 0.f, SurfaceZ + HeightScale * CylinderHalfHeight));
	if (CropMaterial)
	{
		CropMaterial->SetVectorParameterValue(ColorParameter, IsRipe() ? FarmTileColors::Ripe : FarmTileColors::Growing);
		CropMesh->SetMaterial(0, CropMaterial);
	}
}
