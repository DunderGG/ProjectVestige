// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTargetHighlightComponent.h"
#include "Farming/FarmTile.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/VestigePlayerCharacter.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmTargetingTests
{
	TEST_CLASS(Targeting, "Vestige")
	{
		FActorTestSpawner Spawner;
		AFarmGrid* Grid = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UFarmingComponent* Farming = nullptr;

		/** A default 8 x 8 grid of 1 m tiles with a corner at the origin, and a farmer, both playing. */
		BEFORE_EACH()
		{
			// The grid's field lives in the game instance.
			Spawner.InitializeGameSubsystems();
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();
			Farmer = &Spawner.SpawnActor<AVestigePlayerCharacter>();
			Farmer->DispatchBeginPlay();
			Farming = Farmer->FindComponentByClass<UFarmingComponent>();
		}

		/** The tile with grid coordinates X, Y in the default grid. */
		const AFarmTile* TileAt(int32 X, int32 Y) const
		{
			const float TileSize = AFarmTile::UnscaledSize;
			return Grid->GetTileAtLocation(FVector((X + 0.5f) * TileSize, (Y + 0.5f) * TileSize, 0.f));
		}

		TEST_METHOD(TargetStartsOnTheNeighbourTheFarmerFaces)
		{
			ASSERT_THAT(IsNotNull(Farming));

			Farmer->SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);

			ASSERT_THAT(IsTrue(Farming->GetTargetTile() == TileAt(3, 0)));
		}

		TEST_METHOD(AimingPicksTheNeighbourInThatDirectionIncludingDiagonals)
		{
			ASSERT_THAT(IsNotNull(Farming));
			Farmer->SetActorLocation(FVector(250.f, 150.f, 0.f));

			Farming->AimAt(FVector(0.f, 1.f, 0.f));
			ASSERT_THAT(IsTrue(Farming->GetTargetTile() == TileAt(2, 2)));

			Farming->AimAt(FVector(-1.f, -1.f, 0.f));
			ASSERT_THAT(IsTrue(Farming->GetTargetTile() == TileAt(1, 0)));
		}

		TEST_METHOD(TurningOrStoppingKeepsTheAim)
		{
			ASSERT_THAT(IsNotNull(Farming));
			Farmer->SetActorLocation(FVector(250.f, 150.f, 0.f));
			Farming->AimAt(FVector(0.f, -1.f, 0.f));

			Farming->AimAt(FVector::ZeroVector);
			Farmer->SetActorRotation(FRotator(0.f, 90.f, 0.f));

			ASSERT_THAT(IsTrue(Farming->GetTargetTile() == TileAt(2, 0)));
		}

		TEST_METHOD(ANeighbourOutsideTheGridIsNoTarget)
		{
			ASSERT_THAT(IsNotNull(Farming));
			Farmer->SetActorLocation(FVector(50.f, 50.f, 0.f));

			Farming->AimAt(FVector(-1.f, 0.f, 0.f));

			ASSERT_THAT(IsNull(Farming->GetTargetTile()));
		}

		TEST_METHOD(TargetChangeIsBroadcastOnlyWhenTheTileChanges)
		{
			ASSERT_THAT(IsNotNull(Farming));
			int32 Changes = 0;
			Farming->OnTargetTileChanged.AddLambda(
				[&Changes](const AFarmTile* Tile)
				{
					++Changes;
				});

			Farmer->SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);
			Farmer->SetActorLocation(FVector(260.f, 60.f, 0.f));
			ASSERT_THAT(IsTrue(Changes == 1));

			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));
			ASSERT_THAT(IsTrue(Changes == 2));
			ASSERT_THAT(IsNull(Farming->GetTargetTile()));
		}

		TEST_METHOD(HighlightFramesTheTargetAndHidesWithoutOne)
		{
			const UFarmTargetHighlightComponent* Highlight = Farmer->FindComponentByClass<UFarmTargetHighlightComponent>();
			ASSERT_THAT(IsNotNull(Highlight));

			Farmer->SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);
			ASSERT_THAT(IsFalse(Highlight->bHiddenInGame));
			ASSERT_THAT(IsTrue(Highlight->GetComponentLocation().Equals(TileAt(3, 0)->GetActorLocation())));
			ASSERT_THAT(IsTrue(Highlight->GetInstanceCount() == 4));

			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));
			ASSERT_THAT(IsTrue(Highlight->bHiddenInGame));
		}

		/**
		 * The character Blueprint the game uses sets the frame's material, and without the instanced mesh usage saved on
		 * that material the game draws the engine's default material instead. The editor sets a missing flag in memory
		 * and marks it dirty, so a dirty flag fails too.
		 */
		TEST_METHOD(HighlightMaterialIsSavedForInstancedMeshes)
		{
			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			AVestigePlayerCharacter& BlueprintFarmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			BlueprintFarmer.DispatchBeginPlay();
			const UFarmTargetHighlightComponent* Highlight = BlueprintFarmer.FindComponentByClass<UFarmTargetHighlightComponent>();
			ASSERT_THAT(IsNotNull(Highlight));
			UMaterialInterface* Material = Highlight->GetMaterial(0);
			// The frame tints a dynamic instance of the material the Blueprint sets; without one it keeps the mesh's own.
			ASSERT_THAT(IsNotNull(Cast<UMaterialInstanceDynamic>(Material)));
			UMaterial* BaseMaterial = Material->GetMaterial();
			ASSERT_THAT(IsNotNull(BaseMaterial));

			ASSERT_THAT(IsTrue(Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
			ASSERT_THAT(IsFalse(BaseMaterial->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes)));
		}
	};
}

#endif
