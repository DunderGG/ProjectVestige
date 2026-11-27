// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeBed.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Farming/VestigeRestComponent.h"

#define LOCTEXT_NAMESPACE "VestigeBed"

namespace VestigeBedInteraction
{
	/** Half the size of the box the bed is used from: 20 cm around the bed, and up to the farmer's waist. */
	const FVector InteractionExtent(120.f, 70.f, 60.f);
	const FVector InteractionLocation(0.f, 0.f, 60.f);
}

const FName AVestigeBed::SleepAction(TEXT("Sleep"));

AVestigeBed::AVestigeBed()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Keeps its default blocking collision, so the farmer walks around the bed rather than through it.
	BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
	BedMesh->SetupAttachment(RootComponent);
	BedMesh->SetGenerateOverlapEvents(false);

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(VestigeBedInteraction::InteractionExtent);
	InteractionVolume->SetRelativeLocation(VestigeBedInteraction::InteractionLocation);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
}

void AVestigeBed::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	if (UVestigeRestComponent::Find(Interactor))
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
	if (UVestigeRestComponent* Rest = UVestigeRestComponent::Find(Interactor))
	{
		Rest->Sleep();
	}
}

#undef LOCTEXT_NAMESPACE
