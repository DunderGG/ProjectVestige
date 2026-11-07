// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Interaction/VestigeInteractionComponent.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
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

	// What the interaction did (a harvest) may have taken the option away, so the prompt follows at once.
	Scan();
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

	const FVector Facing = GetFacing();
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

		// Only the first option is weighed, as it is the one interacting would use.
		Options.Reset();
		IVestigeInteractable::Execute_GetInteractionOptions(Candidate, OwningActor, Options);
		double DistanceSquared = 0.0;
		if (!Options.IsEmpty() && IsFaced(Options[0], Overlap.GetComponent(), Origin, Facing, DistanceSquared)
			&& DistanceSquared < NearestDistanceSquared)
		{
			Nearest = Candidate;
			NearestOption = Options[0];
			NearestDistanceSquared = DistanceSquared;
		}
	}
	SetFocus(Nearest, NearestOption);
}

FVector UVestigeInteractionComponent::GetFacing() const
{
	FVector Facing = FacingDirection.IsBound() ? FacingDirection.Execute() : FVector::ZeroVector;
	if (Facing.IsNearlyZero() && GetOwner())
	{
		Facing = GetOwner()->GetActorForwardVector();
	}
	return Facing.GetSafeNormal2D();
}

bool UVestigeInteractionComponent::IsFaced(const FVestigeInteractionOption& Option, UPrimitiveComponent* Shape, const FVector& Origin,
	const FVector& Facing, double& OutDistanceSquared) const
{
	if (Option.bHasLocation)
	{
		// Within the cone around the facing direction, on the ground plane, or at the owner's own location.
		const FVector ToOption = (Option.Location - Origin) * FVector(1.0, 1.0, 0.0);
		OutDistanceSquared = FVector::DistSquared(Origin, Option.Location);
		if (ToOption.SizeSquared() < 1.0)
		{
			return true;
		}
		return FVector::DotProduct(ToOption.GetSafeNormal(), Facing) >= FMath::Cos(FMath::DegreesToRadians(FrontHalfAngleDegrees));
	}

	// One ray against the one shape the scan found, at the shape's centre height so any shape height works.
	if (!Shape || Facing.IsNearlyZero())
	{
		return false;
	}
	const FVector Start(Origin.X, Origin.Y, Shape->Bounds.Origin.Z);
	const FVector End = Start + Facing * (2.f * InteractionRadius);
	FHitResult Hit;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(VestigeInteractionFacing));
	if (!Shape->LineTraceComponent(Hit, Start, End, Params))
	{
		return false;
	}
	OutDistanceSquared = FVector::DistSquared2D(Origin, Hit.ImpactPoint);
	return true;
}
void UVestigeInteractionComponent::SetFocus(AActor* Actor, const FVestigeInteractionOption& Option)
{
	// The latest option is kept even when its prompt is the same (another ripe tile), so its location stays current.
	const bool bChanged = Actor != FocusedActor.Get() || !Option.IsSameAs(FocusedOption);
	FocusedActor = Actor;
	FocusedOption = Actor ? Option : FVestigeInteractionOption();
	if (bChanged)
	{
		OnFocusChanged.Broadcast();
	}
}
