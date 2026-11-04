// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGrid.h"

#include "Components/BoxComponent.h"
#include "Core/VestigeActorLookup.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmTile.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

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
	UpdateBounds();
}

void AFarmGrid::UpdateBounds()
{
	const FVector HalfSize(Width * TileSize * 0.5f, Height * TileSize * 0.5f, FarmGridEditor::BoundsHalfHeight);
	Bounds->SetBoxExtent(HalfSize);
	Bounds->SetRelativeLocation(FVector(HalfSize.X, HalfSize.Y, 0.f));
}

void AFarmGrid::BeginPlay()
{
	Super::BeginPlay();

	if (UFarmFieldSubsystem* FarmFields = UFarmFieldSubsystem::Get(GetWorld()))
	{
		Field = &FarmFields->FindOrAddField(FieldId, FIntPoint(Width, Height));
		Field->OnTileChanged.AddUObject(this, &AFarmGrid::RefreshTile);
		Field->OnTilesChanged.AddUObject(this, &AFarmGrid::RefreshTiles);
	}
	else
	{
		VESTIGE_LOG(this, LogFarm, Warning, "There is no game instance with farm fields, so the field is empty.");
	}

	SpawnTiles();
	bTilesReady = true;
	OnTilesReady.Broadcast();
}

void AFarmGrid::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Field)
	{
		Field->OnTileChanged.RemoveAll(this);
		Field->OnTilesChanged.RemoveAll(this);
		Field = nullptr;
	}

	// When the grid alone goes (destroyed, or its sublevel streamed out), its tiles go with it. When the whole world
	// ends, the world destroys them itself.
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		DestroyTiles();
	}
	Tiles.Reset();
	bTilesReady = false;
	Super::EndPlay(EndPlayReason);
}

void AFarmGrid::DestroyTiles()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::DestroyTiles);

	for (AFarmTile* Tile : Tiles)
	{
		if (IsValid(Tile))
		{
			Tile->Destroy();
		}
	}
	Tiles.Reset();
}

void AFarmGrid::Resize(const FIntPoint& NewSize)
{
	DestroyTiles();
	Width = FMath::Max(NewSize.X, 1);
	Height = FMath::Max(NewSize.Y, 1);
	if (Field)
	{
		Field->Resize(FIntPoint(Width, Height));
	}
	UpdateBounds();
	SpawnTiles();
}

AFarmGrid* AFarmGrid::Find(const UWorld* World)
{
	return Cast<AFarmGrid>(VestigeActorLookup::FindSingle(World, StaticClass()));
}

void AFarmGrid::SpawnTiles()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::SpawnTiles);

	if (!Field)
	{
		return;
	}
	if (!TileClass)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "No Tile Class is set, so the field isn't shown.");
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
			AFarmTile* Tile = GetWorld()->SpawnActor<AFarmTile>(TileClass, TileTransform, SpawnParams);
			if (Tile)
			{
				Tile->ShowFieldTile(*Field, FIntPoint(X, Y));
			}
			Tiles.Add(Tile);
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

void AFarmGrid::RefreshTile(const FIntPoint& Coordinates)
{
	if (AFarmTile* Tile = GetTileAt(Coordinates))
	{
		Tile->Refresh();
	}
}

void AFarmGrid::RefreshTiles()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AFarmGrid::RefreshTiles);

	for (AFarmTile* Tile : Tiles)
	{
		if (Tile)
		{
			Tile->Refresh();
		}
	}
}
