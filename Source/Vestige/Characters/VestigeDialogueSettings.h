// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VestigeDialogueSettings.generated.h"

/**
 * How passing lines are timed (Systems.md, section 11). Project Settings > Game > Vestige Dialogue, stored in
 * Config/DefaultGame.ini. Read when a line is said, so a change applies at once. The earshot is per character, on its
 * UVestigePassingLineComponent.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Vestige Dialogue"))
class UVestigeDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UVestigeDialogueSettings& Get() { return *GetDefault<UVestigeDialogueSettings>(); }

	/** Seconds after a talk ends before a passing line may be said. At least 0. */
	float GetPassingQuietAfterTalkSeconds() const { return FMath::Max(PassingQuietAfterTalkSeconds, 0.f); }

	/** Seconds a passing line shows before its length counts. At least 0. */
	float GetPassingLineBaseSeconds() const { return FMath::Max(PassingLineBaseSeconds, 0.f); }

	/** Characters of a passing line read per second. Above 0. */
	float GetPassingLineCharactersPerSecond() const { return FMath::Max(PassingLineCharactersPerSecond, UE_KINDA_SMALL_NUMBER); }

	/** The shortest time a passing line shows. At least 0. */
	float GetPassingLineMinSeconds() const { return FMath::Max(PassingLineMinSeconds, 0.f); }

private:
	/** Seconds after a talk ends before a passing line may be said, so a line never lands on the player as they leave the box. */
	UPROPERTY(Config, EditAnywhere, Category = "Passing lines", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "s"))
	float PassingQuietAfterTalkSeconds = 10.f;

	/** How long a passing line shows before its length is counted, in seconds. */
	UPROPERTY(Config, EditAnywhere, Category = "Passing lines", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "s"))
	float PassingLineBaseSeconds = 1.5f;

	/** How many characters of a passing line are read per second. */
	UPROPERTY(Config, EditAnywhere, Category = "Passing lines", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float PassingLineCharactersPerSecond = 15.f;

	/** The shortest time a passing line shows, in seconds. */
	UPROPERTY(Config, EditAnywhere, Category = "Passing lines", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "s"))
	float PassingLineMinSeconds = 3.f;
};
