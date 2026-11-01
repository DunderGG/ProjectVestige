// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "Player/VestigeCharacterMovementComponent.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeCharacterMovementTests
{
	const FRotator FacingX(0.f, 0.f, 0.f);

	bool FacesYaw(const FRotator& Rotation, float Yaw)
	{
		return FMath::IsNearlyEqual(FRotator::NormalizeAxis(Rotation.Yaw), Yaw, 0.01f);
	}

	TEST_CLASS(CharacterMovement, "Vestige")
	{
		TEST_METHOD(MovementInputSteersTheTurn)
		{
			const FRotator Target =
				UVestigeCharacterMovementComponent::GetTurnTarget(FacingX, FVector(0.0, 50.0, 0.0), FVector(-1.0, 0.0, 0.0));

			ASSERT_THAT(IsTrue(FacesYaw(Target, 90.f)));
		}

		TEST_METHOD(WithoutInputTheFarmerTurnsToTheIdleFacing)
		{
			const FRotator Target =
				UVestigeCharacterMovementComponent::GetTurnTarget(FacingX, FVector::ZeroVector, FVector(-1.0, -1.0, 0.0));

			ASSERT_THAT(IsTrue(FacesYaw(Target, -135.f)));
		}

		TEST_METHOD(WithoutInputOrIdleFacingTheFarmerKeepsFacing)
		{
			const FRotator Current(0.f, 30.f, 0.f);

			ASSERT_THAT(
				IsTrue(UVestigeCharacterMovementComponent::GetTurnTarget(Current, FVector::ZeroVector, FVector::ZeroVector) == Current));
		}

		TEST_METHOD(OnlyTheGroundDirectionCounts)
		{
			// Straight up or down has no ground direction, so the idle facing decides; a slope only turns, never tilts.
			const FRotator Target =
				UVestigeCharacterMovementComponent::GetTurnTarget(FacingX, FVector(0.0, 0.0, 100.0), FVector(0.0, -1.0, 5.0));

			ASSERT_THAT(IsTrue(FacesYaw(Target, -90.f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyZero(Target.Pitch)));
		}
	};
}

#endif
