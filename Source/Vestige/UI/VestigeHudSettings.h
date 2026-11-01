// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VestigeHudSettings.generated.h"

namespace VestigeHudLimits
{
	/** The range the message cap is kept in, whatever the config says. Above the most, the boxes would reach the status line. */
	constexpr int32 FewestMessages = 1;
	constexpr int32 MostMessages = 10;

	/** The shortest time a message stays, so it can fade in and out (VestigeHudMessageFade) and still be read. */
	constexpr float ShortestMessageSeconds = 1.f;
}

/**
 * How the HUD shows feedback messages. Project Settings > Game > Vestige HUD, stored in Config/DefaultGame.ini. The HUD
 * reads it once at BeginPlay.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Vestige HUD"))
class UVestigeHudSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UVestigeHudSettings& Get() { return *GetDefault<UVestigeHudSettings>(); }

	/** Most messages on screen at once, kept between VestigeHudLimits::FewestMessages and MostMessages. */
	int32 GetMaxMessages() const { return ClampMaxMessages(MaxMessages); }

	/** How long each message stays, at least VestigeHudLimits::ShortestMessageSeconds. */
	float GetMessageSeconds() const { return FMath::Max(MessageSeconds, VestigeHudLimits::ShortestMessageSeconds); }

	static int32 ClampMaxMessages(int32 Value)
	{
		return FMath::Clamp(Value, VestigeHudLimits::FewestMessages, VestigeHudLimits::MostMessages);
	}

private:
	/** Most messages on screen at once, each in its own box. A new message beyond this pushes out the oldest. */
	UPROPERTY(Config, EditAnywhere, Category = "Messages", meta = (AllowPrivateAccess = "true", ClampMin = 1, ClampMax = 10))
	int32 MaxMessages = 5;

	/** How long each message stays on screen, in seconds, fading in and out included. */
	UPROPERTY(Config, EditAnywhere, Category = "Messages", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float MessageSeconds = 3.f;
};
