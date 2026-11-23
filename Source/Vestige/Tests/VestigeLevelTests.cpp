// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/VestigeBed.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeLevelTests
{
	const TCHAR* const FarmMapPath = TEXT("/Game/Maps/L_Farm.L_Farm");

	/** The first actor of class T placed in Level, or nullptr. */
	template <typename T> T* FindPlaced(const ULevel& Level)
	{
		for (AActor* Actor : Level.Actors)
		{
			if (T* Found = Cast<T>(Actor))
			{
				return Found;
			}
		}
		return nullptr;
	}

	/** What the main map holds, read from the saved level without playing it. */
	TEST_CLASS(Level, "Vestige")
	{
		/**
		 * GameDesign.md, section 15, question 2's recommended bands: on the 8 by 8 field, the two lines of tiles nearest the bed
		 * clean, the next four lightly and the far two heavily contaminated.
		 */
		TEST_METHOD(TheFarmFieldStartsCleanOnTheBedsSideAndHeavyAtTheFarEdge)
		{
			const UWorld* Map = LoadObject<UWorld>(nullptr, FarmMapPath);
			ASSERT_THAT(IsTrue(Map && Map->PersistentLevel));
			const AFarmGrid* Grid = FindPlaced<AFarmGrid>(*Map->PersistentLevel);
			const AVestigeBed* Bed = FindPlaced<AVestigeBed>(*Map->PersistentLevel);
			ASSERT_THAT(IsNotNull(Grid));
			ASSERT_THAT(IsNotNull(Bed));
			ASSERT_THAT(IsTrue(Grid->GetWidth() == 8));

			// The field a new game makes from the grid's areas, with the default rules.
			UFarmField* Field = NewObject<UFarmField>();
			Field->Initialize(TEXT("Test.LevelField"), FIntPoint(8, 8), FFarmWaterRules(), FFarmContaminationRules());
			Field->ApplyContaminationAreas(Grid->GetStartingContamination());

			int32 Counts[3] = {0, 0, 0};
			double Distances[3] = {0.0, 0.0, 0.0};
			for (int32 Y = 0; Y < 8; ++Y)
			{
				for (int32 X = 0; X < 8; ++X)
				{
					const FIntPoint Coordinates(X, Y);
					const int32 TileLevel = Field->GetTile(Coordinates)->GetContamination();
					ASSERT_THAT(IsTrue(TileLevel >= 0 && TileLevel <= 2));
					++Counts[TileLevel];
					Distances[TileLevel] += FVector::Dist2D(Grid->GetTileTransform(Coordinates).GetLocation(), Bed->GetActorLocation());
				}
			}

			ASSERT_THAT(AreEqual(Counts[0], 16));
			ASSERT_THAT(AreEqual(Counts[1], 32));
			ASSERT_THAT(AreEqual(Counts[2], 16));
			// On average the clean tiles are nearest the bed and the heavy ones farthest.
			ASSERT_THAT(IsTrue(Distances[0] / Counts[0] < Distances[1] / Counts[1]));
			ASSERT_THAT(IsTrue(Distances[1] / Counts[1] < Distances[2] / Counts[2]));
		}
	};
}

#endif
