// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "Farming/FarmContamination.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmContaminationTests
{
	const FName FieldId = TEXT("Base.Farm");

	/** The roll at an even chance, so its answer shows on which side of one half the stream's fraction fell. */
	bool RollAtHalf(int32 Seed, FName Field, const FIntPoint& Tile, int32 Day)
	{
		return ::FarmContamination::RollsMutation(Seed, Field, Tile, Day, 0.5f);
	}

	/** The shared contamination numbers and the repeatable mutation roll. */
	TEST_CLASS(FarmContamination, "Vestige")
	{
		TEST_METHOD(ANoChanceNeverMutatesAndACertainOneAlwaysDoes)
		{
			for (int32 Day = 1; Day <= 50; ++Day)
			{
				const FIntPoint Tile(Day % 8, Day / 8);
				ASSERT_THAT(IsFalse(::FarmContamination::RollsMutation(17, FieldId, Tile, Day, 0.f)));
				ASSERT_THAT(IsFalse(::FarmContamination::RollsMutation(17, FieldId, Tile, Day, -1.f)));
				ASSERT_THAT(IsTrue(::FarmContamination::RollsMutation(17, FieldId, Tile, Day, 1.f)));
				ASSERT_THAT(IsTrue(::FarmContamination::RollsMutation(17, FieldId, Tile, Day, 2.f)));
			}
		}

		/**
		 * Recorded on 2026-11-22 from this code: if an engine update changes HashCombine, FCrc::StrCrc32 or FRandomStream, every
		 * saved game would roll differently after a reload, so this fails instead.
		 */
		TEST_METHOD(PinnedInputsGiveTheirRecordedAnswers)
		{
			FString Answers;
			for (int32 Index = 0; Index < 16; ++Index)
			{
				Answers += RollAtHalf(12345 + Index, FieldId, FIntPoint(Index % 8, Index / 8), 3 + Index) ? TEXT("1") : TEXT("0");
			}

			ASSERT_THAT(AreEqual(Answers, FString(TEXT("1101000111011010"))));
		}

		TEST_METHOD(TheSameInputsGiveTheSameAnswerAndTheFieldsCaseDoesntMatter)
		{
			for (int32 Day = 1; Day <= 40; ++Day)
			{
				const FIntPoint Tile(Day % 8, Day % 5);
				const bool bFirst = ::FarmContamination::RollsMutation(99, FieldId, Tile, Day, 0.25f);
				ASSERT_THAT(IsTrue(::FarmContamination::RollsMutation(99, FieldId, Tile, Day, 0.25f) == bFirst));
				ASSERT_THAT(IsTrue(::FarmContamination::RollsMutation(99, TEXT("base.FARM"), Tile, Day, 0.25f) == bFirst));
			}
		}

		TEST_METHOD(AnotherSeedTileOrDayCanGiveAnotherAnswer)
		{
			// Over 64 inputs each, at an even chance, a changed input that never changes an answer is a broken hash.
			int32 SeedChanges = 0;
			int32 TileChanges = 0;
			int32 DayChanges = 0;
			for (int32 Index = 0; Index < 64; ++Index)
			{
				const FIntPoint Tile(Index % 8, Index / 8);
				const bool bAnswer = RollAtHalf(7, FieldId, Tile, 10);
				SeedChanges += RollAtHalf(8, FieldId, Tile, 10) != bAnswer;
				TileChanges += RollAtHalf(7, FieldId, Tile + FIntPoint(1, 0), 10) != bAnswer;
				DayChanges += RollAtHalf(7, FieldId, Tile, 11) != bAnswer;
			}

			ASSERT_THAT(IsTrue(SeedChanges > 0));
			ASSERT_THAT(IsTrue(TileChanges > 0));
			ASSERT_THAT(IsTrue(DayChanges > 0));
		}

		TEST_METHOD(AQuarterChanceMutatesAboutAQuarterOfTheCrops)
		{
			// A fixed seed, so the share is the same on every run and can't flake.
			int32 Mutated = 0;
			constexpr int32 Tiles = 4096;
			for (int32 Index = 0; Index < Tiles; ++Index)
			{
				Mutated += ::FarmContamination::RollsMutation(2026, FieldId, FIntPoint(Index % 64, Index / 64), 5, 0.25f);
			}

			const float Share = static_cast<float>(Mutated) / Tiles;
			ASSERT_THAT(IsTrue(Share >= 0.22f && Share <= 0.28f, *FString::Printf(TEXT("share %.3f"), Share)));
		}

		TEST_METHOD(TheDefaultRulesAreTwoLevelsAndAQuarterChanceOnLightSoilOnly)
		{
			const FFarmContaminationRules Rules;

			ASSERT_THAT(IsTrue(Rules.MaxLevel == 2));
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(0) == 0.f));
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(1) == 0.25f));
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(2) == 0.f));
		}

		TEST_METHOD(AChancePastTheListIsItsLastAndAnEmptyListIsNone)
		{
			FFarmContaminationRules Rules;
			Rules.MutationChanceByLevel = {0.f, 0.5f};
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(1) == 0.5f));
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(4) == 0.5f));
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(-1) == 0.f));

			Rules.MutationChanceByLevel.Reset();
			ASSERT_THAT(IsTrue(Rules.GetMutationChance(1) == 0.f));
		}

		TEST_METHOD(ClampedRulesHaveALevelAndChancesFromZeroToOne)
		{
			FFarmContaminationRules Rules;
			Rules.MaxLevel = 0;
			Rules.MutationChanceByLevel = {-0.5f, 0.3f, 4.f};

			const FFarmContaminationRules Clamped = FFarmContaminationRules::GetClamped(Rules);

			ASSERT_THAT(IsTrue(Clamped.MaxLevel == 1));
			ASSERT_THAT(IsTrue(Clamped.MutationChanceByLevel == TArray<float>{0.f, 0.3f, 1.f}));
			Rules.MaxLevel = 3;
			ASSERT_THAT(IsTrue(FFarmContaminationRules::GetClamped(Rules).MaxLevel == 3));
		}
	};
}

#endif
