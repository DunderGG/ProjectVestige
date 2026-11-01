// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTargetHighlightComponent.h"

#include "Engine/StaticMesh.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTile.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace FarmTargetHighlight
{
	/** The engine cube is 100 units on each side and centered on its origin. */
	constexpr float CubeSize = 100.f;

	/** How tall each bar is, in cm. Thin, so it reads as a line on the ground. */
	constexpr float BarHeight = 1.f;

	/** Vector parameter that sets the frame's color in HighlightMaterial. */
	const FName ColorParameter(TEXT("Color"));
}

UFarmTargetHighlightComponent::UFarmTargetHighlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnlitMaterial(
		TEXT("/Game/Vestige/Materials/M_TileHighlight.M_TileHighlight"));
	SetStaticMesh(CubeMesh.Object);
	HighlightMaterial = UnlitMaterial.Object;

	// The frame sits on a tile in the world, not on the farmer, and moves only when the target changes.
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
	SetMobility(EComponentMobility::Movable);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	SetHiddenInGame(true);
}

void UFarmTargetHighlightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(HighlightMaterial, this))
	{
		Material->SetVectorParameterValue(FarmTargetHighlight::ColorParameter, HighlightColor);
		SetMaterial(0, Material);
	}
	BuildFrame();

	Farming = GetOwner() ? GetOwner()->FindComponentByClass<UFarmingComponent>() : nullptr;
	if (UFarmingComponent* OwnerFarming = Farming.Get())
	{
		OwnerFarming->OnTargetTileChanged.AddUObject(this, &UFarmTargetHighlightComponent::ShowOn);
		ShowOn(OwnerFarming->GetTargetTile());
	}
}

void UFarmTargetHighlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFarmingComponent* OwnerFarming = Farming.Get())
	{
		OwnerFarming->OnTargetTileChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UFarmTargetHighlightComponent::BuildFrame()
{
	using namespace FarmTargetHighlight;
	const float Half = AFarmTile::UnscaledSize * 0.5f;
	const float BarCenter = Half - BarWidth * 0.5f;
	const FVector AlongX(AFarmTile::UnscaledSize / CubeSize, BarWidth / CubeSize, BarHeight / CubeSize);
	const FVector AlongY(BarWidth / CubeSize, (AFarmTile::UnscaledSize - 2.f * BarWidth) / CubeSize, BarHeight / CubeSize);

	ClearInstances();
	AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, BarCenter, FrameHeight), AlongX));
	AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, -BarCenter, FrameHeight), AlongX));
	AddInstance(FTransform(FRotator::ZeroRotator, FVector(BarCenter, 0.f, FrameHeight), AlongY));
	AddInstance(FTransform(FRotator::ZeroRotator, FVector(-BarCenter, 0.f, FrameHeight), AlongY));
}

void UFarmTargetHighlightComponent::ShowOn(const AFarmTile* Tile)
{
	SetHiddenInGame(!Tile);
	if (Tile)
	{
		SetWorldTransform(Tile->GetActorTransform());
	}
}
