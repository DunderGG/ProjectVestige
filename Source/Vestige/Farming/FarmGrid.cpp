// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGrid.h"

#include "Components/BoxComponent.h"
#include "Core/Vestige.h"
#include "Core/VestigeActorLookup.h"
#include "Engine/World.h"
#include "Farming/FarmTile.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Time/VestigeDayCycle.h"

namespace FarmGridEditor
{
	/** Half the height of the box that outlines the grid in the editor, in cm. Only needs to be visible. */
	constexpr float BoundsHalfHeight = 10.f;
}

AFarmGrid::AFarmGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetupAttachment(RootComponent);
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetHiddenInGame(true);

	TileClass = AFarmTile::StaticClass();
}

void AFarmGrid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FVector HalfSize(Width * TileSize * 0.5f, Height * TileSize * 0.5f, FarmGridEditor::BoundsHalfHeight);
	Bounds->SetBoxExtent(HalfSize);
	Bounds->SetRelativeLocation(FVector(HalfSize.X, HalfSize.Y, 0.f));
}

void AFarmGrid::BeginPlay()
{
	Super::BeginPlay();

	DayCycle = AVestigeDayCycle::Get(GetWorld());
	if (AVestigeDayCycle* Cycle = DayCycle.Get())
	{
		Cycle->OnNewDay.AddUObject(this, &AFarmGrid::AdvanceTiles);
	}

	SpawnTiles();
	bTilesReady = true;
	OnTilesReady.Broadcast();
}

void AFarmGrid::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AVestigeDayCycle* Cycle = DayCycle.Get())
	{
		Cycle->OnNewDay.RemoveAll(this);
	}

	// When the grid alone goes (destroyed, or its sublevel streamed out), its tiles go with it. When the whole world
	// ends, the world destroys them itself.
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		for (AFarmTile* Tile : Tiles)
		{
			if (IsValid(Tile))
			{
				Tile->Destroy();
			}
		}
	}
	Tiles.Reset();
	bTilesReady = false;
	Super::EndPlay(EndPlayReason);
}

AFarmGrid* AFarmGrid::Find(const UWorld* World)
{
	return Cast<AFarmGrid>(VestigeActorLookup::FindSingle(World, StaticClass()));
}

void AFarmGrid::SpawnTiles()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::SpawnTiles);

	if (!TileClass)
	{
		UE_LOG(LogVestige, Warning, TEXT("%s has no Tile Class, so the field is empty."), *GetName());
		return;
	}

	const FVector TileScale(TileSize / AFarmTile::UnscaledSize);
	const FTransform GridTransform = GetActorTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Tiles.Reserve(Width * Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FVector LocalCenter((X + 0.5f) * TileSize, (Y + 0.5f) * TileSize, 0.f);
			const FTransform TileTransform(GetActorRotation(), GridTransform.TransformPosition(LocalCenter), TileScale);
			Tiles.Add(GetWorld()->SpawnActor<AFarmTile>(TileClass, TileTransform, SpawnParams));
		}
	}
}

AFarmTile* AFarmGrid::GetTileAtLocation(const FVector& WorldLocation) const
{
	return GetTileAt(GetCoordinatesAt(WorldLocation));
}

FIntPoint AFarmGrid::GetCoordinatesAt(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return FIntPoint(FMath::FloorToInt(Local.X / TileSize), FMath::FloorToInt(Local.Y / TileSize));
}

FVector2D AFarmGrid::ToGridDirection(const FVector& WorldDirection) const
{
	const FVector Local = GetActorTransform().InverseTransformVectorNoScale(WorldDirection);
	return FVector2D(Local.X, Local.Y);
}

FVector AFarmGrid::ToWorldDirection(const FIntPoint& Step) const
{
	return GetActorTransform().TransformVectorNoScale(FVector(Step.X, Step.Y, 0.f)).GetSafeNormal();
}

AFarmTile* AFarmGrid::GetTileAt(const FIntPoint& Coordinates) const
{
	if (Coordinates.X < 0 || Coordinates.X >= Width || Coordinates.Y < 0 || Coordinates.Y >= Height)
	{
		return nullptr;
	}

	const int32 Index = Coordinates.Y * Width + Coordinates.X;
	return Tiles.IsValidIndex(Index) ? Tiles[Index].Get() : nullptr;
}

TArray<FFarmTileSaveData> AFarmGrid::GetTilesSaveData() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::GetTilesSaveData);

	TArray<FFarmTileSaveData> SavedTiles;
	SavedTiles.Reserve(Tiles.Num());
	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		if (const AFarmTile* Tile = Tiles[Index])
		{
			FFarmTileSaveData& Data = SavedTiles.Add_GetRef(Tile->GetSaveData());
			Data.Coordinates = FIntPoint(Index % Width, Index / Width);
		}
	}
	return SavedTiles;
}

int32 AFarmGrid::RestoreTilesFromSaveData(const TArray<FFarmTileSaveData>& SavedTiles, const TMap<FName, const UFarmCropData*>& Crops)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::RestoreTilesFromSaveData);

	int32 SkippedCount = 0;
	for (const FFarmTileSaveData& Data : SavedTiles)
	{
		AFarmTile* Tile = GetTileAt(Data.Coordinates);
		if (!Tile)
		{
			UE_LOG(LogVestige, Warning, TEXT("The save has a tile at %s, outside %s, so it was skipped."), *Data.Coordinates.ToString(),
				*GetName());
			++SkippedCount;
			continue;
		}

		const UFarmCropData* SavedCrop = Crops.FindRef(Data.CropId);
		if (!Data.CropId.IsNone() && !SavedCrop)
		{
			UE_LOG(LogVestige, Warning, TEXT("The save has a crop %s that no longer exists, so tile %s is empty."), *Data.CropId.ToString(),
				*Data.Coordinates.ToString());
			++SkippedCount;
		}
		Tile->RestoreFromSaveData(Data, SavedCrop);
	}
	return SkippedCount;
}

void AFarmGrid::AdvanceTiles(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::AdvanceTiles);

	for (AFarmTile* Tile : Tiles)
	{
		if (Tile)
		{
			Tile->AdvanceDay();
		}
	}
}
