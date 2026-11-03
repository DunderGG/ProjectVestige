// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeCharacterMovementComponent.h"

FRotator UVestigeCharacterMovementComponent::GetTurnTarget(
	const FRotator& CurrentRotation, const FVector& MoveDirection, const FVector& IdleDirection)
{
	const FVector GroundMove = MoveDirection.GetSafeNormal2D();
	if (!GroundMove.IsZero())
	{
		return GroundMove.Rotation();
	}
	const FVector GroundIdle = IdleDirection.GetSafeNormal2D();
	return GroundIdle.IsZero() ? CurrentRotation : GroundIdle.Rotation();
}

float UVestigeCharacterMovementComponent::GetMaxSpeed() const
{
	const float Speed = Super::GetMaxSpeed();
	return bToolUseHeld && IsWalking() ? Speed * ToolUseSpeedScale : Speed;
}

FRotator UVestigeCharacterMovementComponent::ComputeOrientToMovementRotation(
	const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	// As the engine does: input steers the turn, and without input an AI path following request does.
	FVector MoveDirection = Acceleration;
	if (MoveDirection.IsNearlyZero() && bHasRequestedVelocity)
	{
		MoveDirection = RequestedVelocity;
	}
	return GetTurnTarget(CurrentRotation, MoveDirection, IdleFacing);
}
