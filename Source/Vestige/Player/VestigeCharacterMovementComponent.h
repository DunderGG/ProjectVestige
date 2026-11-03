// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VestigeCharacterMovementComponent.generated.h"

namespace VestigeMovement
{
	/** How fast the farmer turns, in degrees per second: a half turn takes a third of a second. */
	constexpr float TurnDegreesPerSecond = 540.f;
}

/**
 * The farmer's movement. Walks and turns as the engine's character movement does, and keeps turning toward the idle
 * facing (the aimed tile) when there is no movement input, so a short tap still finishes its turn. Every turn goes
 * through RotationRate.
 */
UCLASS()
class UVestigeCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** The direction to turn toward while there is no movement input. A zero vector leaves the farmer facing as he is. */
	void SetIdleFacing(const FVector& Direction) { IdleFacing = Direction; }

	/** Whether the farmer is holding the use key, so he walks at ToolUseSpeedScale of his speed. */
	void SetToolUseHeld(bool bHeld) { bToolUseHeld = bHeld; }

	/**
	 * The speed on the ground and in the air (falling moves at the walking speed), scaled down while the use key is held,
	 * so jumping can't get round the slower walk. Swimming and flying keep the engine's speed.
	 */
	virtual float GetMaxSpeed() const override;

	/**
	 * The rotation to turn toward: along MoveDirection while there is one, else toward IdleDirection, else
	 * CurrentRotation. Only the ground direction counts.
	 */
	static FRotator GetTurnTarget(const FRotator& CurrentRotation, const FVector& MoveDirection, const FVector& IdleDirection);

protected:
	virtual FRotator ComputeOrientToMovementRotation(
		const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

private:
	/** Share of the walking speed left while the use key is held, so walking along a row with a tool is steady. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = 0.1, ClampMax = 1))
	float ToolUseSpeedScale = 0.5f;

	FVector IdleFacing = FVector::ZeroVector;
	bool bToolUseHeld = false;
};
