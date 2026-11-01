// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHudMessages.h"

void FVestigeHudMessages::Add(const FText& Text, double Now, int32 MaxMessages)
{
	while (!Messages.IsEmpty() && Messages.Num() >= MaxMessages)
	{
		Messages.RemoveAt(0);
	}
	Messages.Add({Text, Now});
}

bool FVestigeHudMessages::RemoveExpired(double Now, double Lifetime)
{
	const int32 Removed = Messages.RemoveAll(
		[Now, Lifetime](const FVestigeHudMessage& Message)
		{
			return Now - Message.StartTime >= Lifetime;
		});
	return Removed > 0;
}

TOptional<double> FVestigeHudMessages::GetSecondsUntilNextExpiry(double Now, double Lifetime) const
{
	if (Messages.IsEmpty())
	{
		return {};
	}

	// Every message lives as long, so the oldest expires first.
	return FMath::Max(0.0, Messages[0].StartTime + Lifetime - Now);
}

float FVestigeHudMessages::GetOpacity(double StartTime, double Now, double Lifetime)
{
	using namespace VestigeHudMessageFade;
	const double Age = Now - StartTime;
	const double FadeIn = FMath::Clamp(Age / FadeInSeconds, 0.0, 1.0);
	const double FadeOut = FMath::Clamp((Lifetime - Age) / FadeOutSeconds, 0.0, 1.0);
	return static_cast<float>(FMath::Min(FadeIn, FadeOut));
}
