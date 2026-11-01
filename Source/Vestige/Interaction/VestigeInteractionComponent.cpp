// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Interaction/VestigeInteractionComponent.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UVestigeInteractionComponent::UVestigeInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVestigeInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	constexpr bool bLoop = true;
	GetWorld()->GetTimerManager().SetTimer(ScanTimer, this, &UVestigeInteractionComponent::Scan, ScanSeconds, bLoop);
}

void UVestigeInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UVestigeInteractionComponent::Interact()
{
	// Through the ability system's activation, so its rules apply, as they do to using a tool.
	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		AbilitySystem->TryActivateAbilitiesByTag(FGameplayTagContainer(VestigeGameplayTags::Ability_Interact));
	}
}

bool UVestigeInteractionComponent::InteractWithFocus()
{
	// The last scan can be up to ScanSeconds old, and the player may have stepped away since.
	Scan();

	AActor* Focus = FocusedActor.Get();
	if (!Focus)
	{
		return false;
	}

	// A copy, so the option stays as it was offered even if what the interactable does changes the focus.
	const FVestigeInteractionOption Option = FocusedOption;
	IVestigeInteractable::Execute_Interact(Focus, GetOwner(), Option);
	return true;
}

void UVestigeInteractionComponent::Scan()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeInteractionComponent::Scan);

	AActor* OwningActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwningActor || !World)
	{
		return;
	}

	const FVector Origin = OwningActor->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VestigeInteractionScan));
	Params.AddIgnoredActor(OwningActor);
	Overlaps.Reset();
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(VestigeInteraction::ObjectChannel),
		FCollisionShape::MakeSphere(InteractionRadius), Params);

	AActor* Nearest = nullptr;
	FVestigeInteractionOption NearestOption;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<UVestigeInteractable>())
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistanceSquared >= NearestDistanceSquared)
		{
			continue;
		}

		Options.Reset();
		IVestigeInteractable::Execute_GetInteractionOptions(Candidate, OwningActor, Options);
		if (!Options.IsEmpty())
		{
			Nearest = Candidate;
			NearestOption = Options[0];
			NearestDistanceSquared = DistanceSquared;
		}
	}
	SetFocus(Nearest, NearestOption);
}

void UVestigeInteractionComponent::SetFocus(AActor* Actor, const FVestigeInteractionOption& Option)
{
	if (Actor == FocusedActor.Get() && Option.IsSameAs(FocusedOption))
	{
		return;
	}

	FocusedActor = Actor;
	FocusedOption = Actor ? Option : FVestigeInteractionOption();
	OnFocusChanged.Broadcast();
}
