// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/VestigeNpcCharacter.h"
#include "Content/VestigeCharacterData.h"
#include "CQTest.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/VestigeBed.h"
#include "GameFramework/PlayerStart.h"

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

		/**
		 * Docs/Levels/HomeFarm/README.md, item 2: Hob stands off the field's apron, in view of the Player Start. The farmhouse isn't
		 * built yet, so the test holds the part of the plan the map can show; the layout item moves Hob beside the door and makes this test check west of the field.
		 */
		TEST_METHOD(HobStandsOffTheFieldInViewOfTheStart)
		{
			const UWorld* Map = LoadObject<UWorld>(nullptr, FarmMapPath);
			ASSERT_THAT(IsTrue(Map && Map->PersistentLevel));
			int32 HobCount = 0;
			const AVestigeNpcCharacter* Hob = nullptr;
			for (const AActor* Actor : Map->PersistentLevel->Actors)
			{
				const AVestigeNpcCharacter* Npc = Cast<AVestigeNpcCharacter>(Actor);
				if (Npc && Npc->GetCharacterId() == FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob")))
				{
					Hob = Npc;
					++HobCount;
				}
			}
			ASSERT_THAT(IsTrue(HobCount == 1));
			const AFarmGrid* Grid = FindPlaced<AFarmGrid>(*Map->PersistentLevel);
			const APlayerStart* Start = FindPlaced<APlayerStart>(*Map->PersistentLevel);
			ASSERT_THAT(IsNotNull(Grid));
			ASSERT_THAT(IsNotNull(Start));

			// The field's footprint, from its corner tiles' centres and half a tile, grown by the 1.5 m apron.
			constexpr double Apron = 150.0;
			const FTransform First = Grid->GetTileTransform(FIntPoint(0, 0));
			const FTransform Last = Grid->GetTileTransform(FIntPoint(Grid->GetWidth() - 1, Grid->GetWidth() - 1));
			const double HalfTile = First.GetScale3D().X * AFarmGrid::UnscaledTileSize * 0.5;
			const FBox2D Field(FVector2D(FMath::Min(First.GetLocation().X, Last.GetLocation().X) - HalfTile - Apron,
								   FMath::Min(First.GetLocation().Y, Last.GetLocation().Y) - HalfTile - Apron),
				FVector2D(FMath::Max(First.GetLocation().X, Last.GetLocation().X) + HalfTile + Apron,
					FMath::Max(First.GetLocation().Y, Last.GetLocation().Y) + HalfTile + Apron));
			const FVector2D HobSpot(Hob->GetActorLocation().X, Hob->GetActorLocation().Y);
			ASSERT_THAT(IsFalse(Field.IsInside(HobSpot)));

			// Within 15 m of the start and 45 degrees of where it faces: in view.
			const FVector ToHob = (Hob->GetActorLocation() - Start->GetActorLocation()) * FVector(1.0, 1.0, 0.0);
			ASSERT_THAT(IsTrue(ToHob.Size() <= 1500.0));
			const double AngleDegrees =
				FMath::RadiansToDegrees(FMath::Acos(ToHob.GetSafeNormal() | Start->GetActorForwardVector().GetSafeNormal2D()));
			ASSERT_THAT(IsTrue(AngleDegrees <= 45.0));
		}
	};
}

#endif
