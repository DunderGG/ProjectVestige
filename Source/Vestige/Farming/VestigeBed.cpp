// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeBed.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Farming/VestigeRestComponent.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "VestigeBed"

namespace VestigeBedPlaceholder
{
	/** The engine cube is 100 cm on each side and centred on its pivot; scaled to a bed of 200 x 100 x 50 cm on the floor. */
	const FVector MeshScale(2.f, 1.f, 0.5f);
	const FVector MeshLocation(0.f, 0.f, 25.f);

	/** Half the size of the box the bed is used from: 20 cm around the bed, and up to the farmer's waist. */
	const FVector InteractionExtent(120.f, 70.f, 60.f);
	const FVector InteractionLocation(0.f, 0.f, 60.f);
}

const FName AVestigeBed::SleepAction(TEXT("Sleep"));

AVestigeBed::AVestigeBed()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Keeps its default blocking collision, so the farmer walks around the bed rather than through it.
	BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
	BedMesh->SetupAttachment(RootComponent);
	BedMesh->SetStaticMesh(CubeMesh.Object);
	BedMesh->SetRelativeScale3D(VestigeBedPlaceholder::MeshScale);
	BedMesh->SetRelativeLocation(VestigeBedPlaceholder::MeshLocation);
	BedMesh->SetGenerateOverlapEvents(false);

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(VestigeBedPlaceholder::InteractionExtent);
	InteractionVolume->SetRelativeLocation(VestigeBedPlaceholder::InteractionLocation);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
}

void AVestigeBed::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	if (Interactor && Interactor->FindComponentByClass<UVestigeRestComponent>())
	{
		FVestigeInteractionOption& SleepOption = OutOptions.AddDefaulted_GetRef();
		SleepOption.Text = LOCTEXT("Sleep", "Sleep");
		SleepOption.Action = SleepAction;
	}
}

void AVestigeBed::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	if (Option.Action != SleepAction)
	{
		return;
	}
	if (UVestigeRestComponent* Rest = Interactor ? Interactor->FindComponentByClass<UVestigeRestComponent>() : nullptr)
	{
		Rest->Sleep();
	}
}

#undef LOCTEXT_NAMESPACE
