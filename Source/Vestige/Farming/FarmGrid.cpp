// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGrid.h"

#include "Components/BoxComponent.h"
#include "Core/VestigeActorLookup.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldMeshesComponent.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmLog.h"

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

	Meshes = CreateDefaultSubobject<UFarmFieldMeshesComponent>(TEXT("Meshes"));
	Meshes->SetupAttachment(RootComponent);
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
		Meshes->ShowField(*Field, TileSize);
	}
	else
	{
		VESTIGE_LOG(this, LogFarm, Warning, "There is no game instance with farm fields, so the field is empty.");
	}

	bFieldReady = true;
	OnFieldReady.Broadcast();
}

void AFarmGrid::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The meshes component stops following the field in its own EndPlay.
	Field = nullptr;
	bFieldReady = false;
	Super::EndPlay(EndPlayReason);
}

void AFarmGrid::Resize(const FIntPoint& NewSize)
{
	Width = FMath::Max(NewSize.X, 1);
	Height = FMath::Max(NewSize.Y, 1);
	UpdateBounds();
	if (Field)
	{
		Field->Resize(FIntPoint(Width, Height));
	}
}

AFarmGrid* AFarmGrid::Find(const UWorld* World)
{
	return Cast<AFarmGrid>(VestigeActorLookup::FindSingle(World, StaticClass()));
}

const FFarmTileState* AFarmGrid::GetTile(const FIntPoint& Coordinates) const
{
	return Field ? Field->GetTile(Coordinates) : nullptr;
}

FVector AFarmGrid::GetLocalTileCenter(const FIntPoint& Coordinates, float InTileSize)
{
	return FVector((Coordinates.X + 0.5f) * InTileSize, (Coordinates.Y + 0.5f) * InTileSize, 0.f);
}

FTransform AFarmGrid::GetTileTransform(const FIntPoint& Coordinates) const
{
	const FVector WorldCenter = GetActorTransform().TransformPosition(GetLocalTileCenter(Coordinates, TileSize));
	return FTransform(GetActorQuat(), WorldCenter, FVector(TileSize / UnscaledTileSize));
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
