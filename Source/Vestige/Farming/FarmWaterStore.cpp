// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmWaterStore.h"

int32 FFarmWaterStore::GetLeft(int32 Today, int32 DailySupply) const
{
	const int32 Supply = FMath::Max(0, DailySupply);
	return Day == Today ? FMath::Max(0, Supply - Drawn) : Supply;
}

int32 FFarmWaterStore::Draw(int32 Today, int32 DailySupply, int32 Amount)
{
	const int32 Taken = FMath::Clamp(Amount, 0, GetLeft(Today, DailySupply));
	if (Taken == 0)
	{
		return 0;
	}
	if (Day != Today)
	{
		Day = Today;
		Drawn = 0;
	}
	Drawn += Taken;
	return Taken;
}
