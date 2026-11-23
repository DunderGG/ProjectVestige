// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmContamination.h"

#include "Math/RandomStream.h"
#include "Misc/Crc.h"
#include "Templates/TypeHash.h"

float FFarmContaminationRules::GetMutationChance(int32 Level) const
{
	if (MutationChanceByLevel.IsEmpty())
	{
		return 0.f;
	}
	return FMath::Clamp(MutationChanceByLevel[FMath::Clamp(Level, 0, MutationChanceByLevel.Num() - 1)], 0.f, 1.f);
}

FFarmContaminationRules FFarmContaminationRules::GetClamped(const FFarmContaminationRules& Rules)
{
	FFarmContaminationRules Clamped;
	Clamped.MaxLevel = FMath::Max(Rules.MaxLevel, 1);
	Clamped.MutationChanceByLevel.Reset(Rules.MutationChanceByLevel.Num());
	for (const float Chance : Rules.MutationChanceByLevel)
	{
		Clamped.MutationChanceByLevel.Add(FMath::Clamp(Chance, 0.f, 1.f));
	}
	return Clamped;
}

namespace FarmContamination
{
	bool RollsMutation(int32 Seed, FName FieldId, const FIntPoint& Coordinates, int32 Day, float Chance)
	{
		if (Chance <= 0.f)
		{
			return false;
		}
		if (Chance >= 1.f)
		{
			return true;
		}

		// Only functions documented as stable go in, so a roll is the same in every run: the field's name as lower-cased text
		// (an FName may keep another spelling in another run) and HashCombine, never GetTypeHash(FName) or HashCombineFast.
		const uint32 FieldHash = FCrc::StrCrc32(*FieldId.ToString().ToLower());
		uint32 Hash = static_cast<uint32>(Seed);
		for (const uint32 Value :
			{FieldHash, static_cast<uint32>(Coordinates.X), static_cast<uint32>(Coordinates.Y), static_cast<uint32>(Day)})
		{
			Hash = HashCombine(Hash, Value);
		}
		const FRandomStream Stream(static_cast<int32>(Hash));
		return Stream.GetFraction() < Chance;
	}
}
