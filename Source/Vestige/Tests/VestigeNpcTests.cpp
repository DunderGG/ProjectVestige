// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AIController.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Characters/VestigeWateringHelpComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Components/CapsuleComponent.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "CQTest.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileState.h"
#include "Farming/FarmWaterSettings.h"
#include "Farming/FarmWaterSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Interaction/VestigeInteractable.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeNpcTests
{
	const TCHAR* const HobPath = TEXT("/Game/Vestige/Characters/BP_Hob.BP_Hob_C");

	TEST_CLASS(Robots, "Vestige")
	{
		FActorTestSpawner Spawner;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		AVestigeNpcCharacter* Hob = nullptr;
		AAIController* Controller = nullptr;

		/** The game's Hob with an AI controller, and a player state with the conversation component to talk to. */
		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState->DispatchBeginPlay();
			Talk = PlayerState->FindComponentByClass<UVestigeConversationComponent>();
			ASSERT_THAT(IsNotNull(Talk));

			UClass* HobClass = LoadClass<AVestigeNpcCharacter>(nullptr, HobPath);
			ASSERT_THAT(IsNotNull(HobClass));
			Hob = &Spawner.SpawnActor<AVestigeNpcCharacter>(FActorSpawnParameters(), HobClass);
			Hob->DispatchBeginPlay();

			// The placed Hob gets the controller from AutoPossessAI when the level starts; the test world may not, so make sure.
			Controller = Cast<AAIController>(Hob->GetController());
			if (!Controller)
			{
				Controller = &Spawner.SpawnActor<AAIController>();
				Controller->Possess(Hob);
			}
			ASSERT_THAT(IsNotNull(Controller));
		}

		TArray<FVestigeInteractionOption> GetOptions(AActor* Interactor) const
		{
			TArray<FVestigeInteractionOption> Options;
			Hob->GetInteractionOptions_Implementation(Interactor, Options);
			return Options;
		}

		TEST_METHOD(HobIsTheCharacterBaseHob)
		{
			ASSERT_THAT(IsTrue(Hob->GetCharacterId() == FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"))));
			ASSERT_THAT(IsNotNull(Hob->GetCharacterData()));
		}

		TEST_METHOD(HobOffersTalkToHob)
		{
			const TArray<FVestigeInteractionOption> Options = GetOptions(PlayerState);

			ASSERT_THAT(IsTrue(Options.Num() == 1));
			ASSERT_THAT(IsTrue(Options[0].Text.ToString() == TEXT("Talk to Hob")));
			ASSERT_THAT(IsTrue(Options[0].Action == AVestigeNpcCharacter::TalkAction));
		}

		TEST_METHOD(HobOffersNothingDuringATalk)
		{
			Hob->Interact_Implementation(PlayerState, GetOptions(PlayerState)[0]);
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));

			ASSERT_THAT(IsTrue(GetOptions(PlayerState).IsEmpty()));
		}

		TEST_METHOD(HobOffersNothingWithoutAConversationComponent)
		{
			AActor& Stranger = Spawner.SpawnActor<AActor>();

			ASSERT_THAT(IsTrue(GetOptions(&Stranger).IsEmpty()));
			ASSERT_THAT(IsTrue(GetOptions(nullptr).IsEmpty()));
		}

		TEST_METHOD(TalkingToHobStartsTheConversation)
		{
			const TArray<FVestigeInteractionOption> Options = GetOptions(PlayerState);
			ASSERT_THAT(IsTrue(Options.Num() == 1));

			Hob->Interact_Implementation(PlayerState, Options[0]);

			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
			ASSERT_THAT(IsTrue(Talk->GetSpeakerActor() == Hob));
			ASSERT_THAT(IsTrue(Talk->GetSpeaker() == Hob->GetCharacterData()));
		}

		TEST_METHOD(AnOtherActionStartsNoConversation)
		{
			FVestigeInteractionOption Wrong;
			Wrong.Action = TEXT("Dance");

			Hob->Interact_Implementation(PlayerState, Wrong);

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
		}

		TEST_METHOD(HobTurnsToTheFarmerAndBack)
		{
			const FVector Ahead = Hob->GetActorRotation().Vector();

			Hob->Interact_Implementation(PlayerState, GetOptions(PlayerState)[0]);
			ASSERT_THAT(IsTrue(Controller->GetFocusActor() == PlayerState));

			Talk->EndConversation();

			// The focal point replaces the focus actor, a point ahead along the placed facing.
			ASSERT_THAT(IsNull(Controller->GetFocusActor()));
			const FVector ToFocus = (Controller->GetFocalPoint() - Hob->GetActorLocation()).GetSafeNormal2D();
			ASSERT_THAT(IsTrue(FVector::DotProduct(ToFocus, Ahead.GetSafeNormal2D()) > 0.99));
		}

		TEST_METHOD(HobBlocksThePlayer)
		{
			ASSERT_THAT(IsTrue(Hob->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block));
			ASSERT_THAT(IsTrue(Hob->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics));
		}

		TEST_METHOD(AnUnknownCharacterOffersNothing)
		{
			// The class with no character ID set.
			TestRunner->AddExpectedMessagePlain(TEXT("offers nothing"), ELogVerbosity::Warning);
			AVestigeNpcCharacter& Nobody = Spawner.SpawnActor<AVestigeNpcCharacter>();
			Nobody.DispatchBeginPlay();

			TArray<FVestigeInteractionOption> Options;
			Nobody.GetInteractionOptions_Implementation(PlayerState, Options);

			ASSERT_THAT(IsTrue(Options.IsEmpty()));
			Nobody.Interact_Implementation(PlayerState, FVestigeInteractionOption());
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
		}

		TEST_METHOD(AGoneConversationComponentIsNoProblemForHob)
		{
			// The player state goes first; Hob must not touch the dead component when his own end comes.
			Hob->Interact_Implementation(PlayerState, GetOptions(PlayerState)[0]);
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));

			PlayerState->Destroy();
			Hob->Destroy();

			ASSERT_THAT(IsFalse(IsValid(Hob)));
		}

		// ---- Hob's dawn watering ----

		UFarmField* Field = nullptr;
		AFarmGrid* Grid = nullptr;
		UFarmFieldSubsystem* Fields = nullptr;
		UFarmWaterSubsystem* Water = nullptr;
		UVestigeRelationshipComponent* Relationships = nullptr;
		UVestigeWateringHelpComponent* Help = nullptr;

		FGameplayTag MakeTag(const TCHAR* Name) const { return FGameplayTag::RequestGameplayTag(FName(Name)); }

		/** The farm, the player Hob helps and Hob on tile (0, 0); with the offer accepted, unless it is not. */
		void SetUpFarm(bool bAccepted = true)
		{
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();
			Field = Grid->GetField();
			Fields = UFarmFieldSubsystem::Get(&Spawner.GetWorld());
			Water = UFarmWaterSubsystem::Get(&Spawner.GetWorld());
			Relationships = PlayerState->FindComponentByClass<UVestigeRelationshipComponent>();
			Help = Hob->FindComponentByClass<UVestigeWateringHelpComponent>();
			ASSERT_THAT(IsNotNull(Field));
			ASSERT_THAT(IsNotNull(Fields));
			ASSERT_THAT(IsNotNull(Water));
			ASSERT_THAT(IsNotNull(Relationships));
			ASSERT_THAT(IsNotNull(Help));

			APlayerController& PlayerController = Spawner.SpawnActor<APlayerController>();
			PlayerController.PlayerState = PlayerState;
			Hob->SetActorLocation(Grid->GetTileTransform(FIntPoint(0, 0)).GetLocation());
			if (bAccepted)
			{
				Relationships->SetFlag(*Hob->GetCharacterData(), MakeTag(TEXT("Vestige.Relationship.Help.Watering")), 1);
			}
		}

		const UFarmCropData* FindTurnip()
		{
			return UVestigeContentCatalog::Get(&Spawner.GetWorld())
				->Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
		}

		/** Tills and plants a turnip: growing, dry. */
		void PlantDry(const FIntPoint& Coordinates)
		{
			const UFarmCropData* Turnip = FindTurnip();
			ASSERT_THAT(IsNotNull(Turnip));
			Field->Till(Coordinates);
			ASSERT_THAT(IsTrue(Field->Plant(Coordinates, Turnip)));
		}

		/** Puts a turnip of this state on the tile, as loading a save does. */
		void RestoreTurnip(const FIntPoint& Coordinates, EFarmTileState State, int32 GrowthDays)
		{
			const UFarmCropData* Turnip = FindTurnip();
			ASSERT_THAT(IsNotNull(Turnip));
			FFarmTileSaveData Saved;
			Saved.Coordinates = Coordinates;
			Saved.State = State;
			Saved.GrowthDays = GrowthDays;
			Saved.CropId = TEXT("Base.Turnip");
			Field->RestoreTilesFromSaveData({Saved}, {{TEXT("Base.Turnip"), Turnip}});
		}

		int32 GetTileWater(const FIntPoint& Coordinates) const { return Field->GetTile(Coordinates)->GetWaterLevel(); }

		/** The night's farm work is done on Day, as the field subsystem tells. */
		void Dawn(int32 Day = 2) { Fields->OnFieldsAdvanced.Broadcast(Day); }

		int32 GetHelpedDay() const
		{
			const int32* Day =
				Relationships->GetFlagDays(Hob->GetCharacterId()).Find(MakeTag(TEXT("Vestige.Relationship.Helped.Watering")));
			return Day ? *Day : INDEX_NONE;
		}

		TEST_METHOD(HobWatersTheNearestDryGrowingTilesAtDawn)
		{
			SetUpFarm();
			for (const FIntPoint& Tile : {FIntPoint(5, 5), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(7, 7), FIntPoint(2, 2), FIntPoint(3, 0)})
			{
				PlantDry(Tile);
			}
			const int32 Left = Water->GetCleanWaterLeft();
			ASSERT_THAT(IsTrue(Left >= 4));

			Dawn(2);

			// Hob stands on (0, 0): the four nearest are watered, the tie between (1, 0) and (0, 1) goes to the first row.
			for (const FIntPoint& Tile : {FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(2, 2), FIntPoint(3, 0)})
			{
				ASSERT_THAT(IsTrue(GetTileWater(Tile) > 0, *Tile.ToString()));
			}
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(5, 5)) == 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(7, 7)) == 0));
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == Left - 4));
			ASSERT_THAT(IsTrue(GetHelpedDay() == 2));
		}

		TEST_METHOD(HobDoesNotWaterWithoutTheAcceptedOffer)
		{
			SetUpFarm(false);
			PlantDry(FIntPoint(1, 0));
			const int32 Left = Water->GetCleanWaterLeft();

			Dawn();

			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 0)) == 0));
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == Left));
			ASSERT_THAT(IsTrue(GetHelpedDay() == INDEX_NONE));
		}

		TEST_METHOD(HobSkipsRipeWitheredAndWetTiles)
		{
			SetUpFarm();
			PlantDry(FIntPoint(1, 0));
			PlantDry(FIntPoint(2, 0));
			Field->Water(FIntPoint(2, 0));
			const int32 WetBefore = GetTileWater(FIntPoint(2, 0));
			ASSERT_THAT(IsTrue(WetBefore > 0));
			RestoreTurnip(FIntPoint(3, 0), EFarmTileState::Planted, FindTurnip()->GetDaysToMature());
			RestoreTurnip(FIntPoint(4, 0), EFarmTileState::Withered, 1);
			Field->Till(FIntPoint(0, 1));
			ASSERT_THAT(IsTrue(Field->GetTile(FIntPoint(3, 0))->IsRipe()));
			ASSERT_THAT(IsTrue(Field->GetTile(FIntPoint(4, 0))->IsWithered()));
			const int32 Left = Water->GetCleanWaterLeft();

			Dawn();

			// Only the dry growing tile is watered, and only it is paid for.
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 0)) > 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(2, 0)) == WetBefore));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(3, 0)) == 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(4, 0)) == 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(0, 1)) == 0));
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == Left - 1));
		}

		TEST_METHOD(HobWatersNoMoreThanTheWellHasLeft)
		{
			SetUpFarm();
			for (const FIntPoint& Tile : {FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1), FIntPoint(2, 0)})
			{
				PlantDry(Tile);
			}
			Water->DrawCleanWater(Water->GetCleanWaterLeft() - 2);
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == 2));

			Dawn();

			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 0)) > 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(0, 1)) > 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 1)) == 0));
			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(2, 0)) == 0));
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == 0));
			ASSERT_THAT(IsTrue(GetHelpedDay() == 2));
		}

		TEST_METHOD(HobWatersNothingAndSaysNothingWhenTheWellIsDry)
		{
			SetUpFarm();
			PlantDry(FIntPoint(1, 0));
			Water->DrawCleanWater(Water->GetCleanWaterLeft());

			Dawn();

			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 0)) == 0));
			ASSERT_THAT(IsTrue(GetHelpedDay() == INDEX_NONE));
		}

		TEST_METHOD(HobsWateredTilesAreWetAfterTheNewDay)
		{
			SetUpFarm();
			PlantDry(FIntPoint(1, 0));
			UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));

			// The night's fall comes before the help, so the tile does not dry again.
			Clock->StartNextDay();

			ASSERT_THAT(IsTrue(GetTileWater(FIntPoint(1, 0)) > 0));
			ASSERT_THAT(IsTrue(GetHelpedDay() == Clock->GetCurrentDay()));
			ASSERT_THAT(IsTrue(Water->GetCleanWaterLeft() == UFarmWaterSettings::Get().GetWellWaterPerDay() - 1));
		}

		TEST_METHOD(HobsWateringSetsTheMorningFlagOnlyWhenItWatered)
		{
			SetUpFarm();

			Dawn(2);
			ASSERT_THAT(IsTrue(GetHelpedDay() == INDEX_NONE));

			PlantDry(FIntPoint(1, 0));
			Dawn(3);
			ASSERT_THAT(IsTrue(GetHelpedDay() == 3));
		}

		TEST_METHOD(TheHelpDoesNothingWithoutAFarmOrAPlayer)
		{
			// No grid and no player controller: the broadcast is no problem.
			Fields = UFarmFieldSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Fields));

			Dawn();

			ASSERT_THAT(IsTrue(IsValid(Hob)));
		}

		TEST_METHOD(BP_HobHelpsWithFourTilesAndTheHelpFlags)
		{
			const UVestigeWateringHelpComponent* Component = Hob->FindComponentByClass<UVestigeWateringHelpComponent>();
			ASSERT_THAT(IsNotNull(Component));

			ASSERT_THAT(IsTrue(Component->GetMaxTiles() == 4));
			ASSERT_THAT(IsTrue(Component->GetRequiredFlag() == MakeTag(TEXT("Vestige.Relationship.Help.Watering"))));
			ASSERT_THAT(IsTrue(Component->GetHelpedFlag() == MakeTag(TEXT("Vestige.Relationship.Helped.Watering"))));
		}
	};
}

#endif
