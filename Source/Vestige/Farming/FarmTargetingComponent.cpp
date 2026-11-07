// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTargetingComponent.h"

#include "Components/SceneComponent.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmTileTargeting.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FarmTargetingComponent"

UFarmTargetingComponent::UFarmTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFarmTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	Grid = AFarmGrid::Find(GetWorld());
	if (Grid)
	{
		// The grid finds its field in its own BeginPlay, which may run after this one.
		Grid->OnFieldReady.AddUObject(this, &UFarmTargetingComponent::UpdateTargetTile);

		// Until the first movement input, aim where the owner faces.
		const FIntPoint FacingStep = FarmTileTargeting::SnapToStep(Grid->ToGridDirection(GetOwner()->GetActorForwardVector()));
		if (FacingStep != FIntPoint::ZeroValue)
		{
			AimStep = FacingStep;
		}
		OwnerCoordinates = Grid->GetCoordinatesAt(GetOwner()->GetActorLocation());
	}
	else
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The level {Level} has no FarmGrid, so there is nothing to farm.", GetWorld()->GetMapName());

		// The HUD may not be listening for feedback yet during BeginPlay.
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			[WeakThis = TWeakObjectPtr<UFarmTargetingComponent>(this)]()
			{
				if (const UFarmTargetingComponent* Targeting = WeakThis.Get())
				{
					Targeting->Notify(LOCTEXT("NoGrid", "No FarmGrid in the level. Drag one in from the Place Actors panel."));
				}
			});
	}

	if (USceneComponent* Root = GetOwner() ? GetOwner()->GetRootComponent() : nullptr)
	{
		Root->TransformUpdated.AddUObject(this, &UFarmTargetingComponent::FollowOwnerTransform);
	}
	UpdateTargetTile();
}

void UFarmTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Grid)
	{
		Grid->OnFieldReady.RemoveAll(this);
	}
	if (USceneComponent* Root = GetOwner() ? GetOwner()->GetRootComponent() : nullptr)
	{
		Root->TransformUpdated.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UFarmTargetingComponent::AimAt(const FVector& WorldDirection)
{
	if (!Grid)
	{
		return;
	}

	// Called every frame while a movement input is held, so the usual case stops here: no input, or the same direction.
	const FIntPoint Step = FarmTileTargeting::SnapToStep(Grid->ToGridDirection(WorldDirection));
	if (Step == FIntPoint::ZeroValue || Step == AimStep)
	{
		return;
	}
	AimStep = Step;
	UpdateTargetTile();
}

FVector UFarmTargetingComponent::GetAimDirection() const
{
	return Grid ? Grid->ToWorldDirection(AimStep) : FVector::ZeroVector;
}

TOptional<FIntPoint> UFarmTargetingComponent::FindTargetTile() const
{
	const AActor* Owner = GetOwner();
	if (!Grid || !Owner)
	{
		return {};
	}
	const FIntPoint Neighbour = FarmTileTargeting::GetNeighbour(Grid->GetCoordinatesAt(Owner->GetActorLocation()), AimStep);
	return Grid->GetTile(Neighbour) ? TOptional<FIntPoint>(Neighbour) : TOptional<FIntPoint>();
}

void UFarmTargetingComponent::UpdateTargetTile()
{
	const TOptional<FIntPoint> NewTarget = FindTargetTile();
	if (NewTarget == TargetTile)
	{
		return;
	}
	TargetTile = NewTarget;

	// A tool ability waiting with the key held listens, so walking along a row works each tile it reaches.
	OnTargetTileChanged.Broadcast(NewTarget);
}

void UFarmTargetingComponent::FollowOwnerTransform(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	// Fires on every move and turn; the target only changes when the owner reaches another tile.
	const AActor* Owner = GetOwner();
	const FIntPoint Coordinates = Grid && Owner ? Grid->GetCoordinatesAt(Owner->GetActorLocation()) : OwnerCoordinates;
	if (Coordinates == OwnerCoordinates)
	{
		return;
	}
	OwnerCoordinates = Coordinates;
	UpdateTargetTile();
}

void UFarmTargetingComponent::Notify(const FText& Message) const
{
	UVestigeFeedbackSubsystem::Notify(GetWorld(), Message);
}

#undef LOCTEXT_NAMESPACE
