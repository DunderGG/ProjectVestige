// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Engine/CollisionProfile.h"
#include "Farming/VestigeBed.h"
#include "Farming/VestigeRestComponent.h"
#include "Game/VestigeGameMode.h"
#include "Interaction/VestigeInteractable.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestInteractable.h"
#include "Time/VestigeClockSubsystem.h"

#define LOCTEXT_NAMESPACE "VestigeInteractionTests"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeInteractionTests
{
	const FName Open(TEXT("Open"));
	const FName Take(TEXT("Take"));

	TEST_CLASS(Interaction, "Vestige")
	{
		FActorTestSpawner Spawner;

		/**
		 * An actor at the origin facing along X, with an interaction component, which looks for interactables only when a test
		 * scans. It has a root, so a test can turn it.
		 */
		UVestigeInteractionComponent& MakeInteractor()
		{
			AActor& Interactor = Spawner.SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(&Interactor);
			Interactor.SetRootComponent(Root);
			Root->RegisterComponent();
			UVestigeInteractionComponent* Interaction = NewObject<UVestigeInteractionComponent>(&Interactor);
			Interaction->RegisterComponent();
			return *Interaction;
		}

		/** An interactable at Distance cm from the origin along X, offering Action when Action is set. */
		AVestigeTestInteractable& MakeInteractable(float Distance, FName Action = NAME_None)
		{
			AVestigeTestInteractable& Interactable = Spawner.SpawnActor<AVestigeTestInteractable>();
			Interactable.SetActorLocation(FVector(Distance, 0.f, 0.f));
			if (!Action.IsNone())
			{
				Interactable.Offer(FText::FromName(Action), Action);
			}
			return Interactable;
		}

		TEST_METHOD(TheInteractableChannelAndProfileAreConfigured)
		{
			const UCollisionProfile* Profiles = UCollisionProfile::Get();
			FCollisionResponseTemplate Profile;

			ASSERT_THAT(IsTrue(Profiles->ReturnChannelNameFromContainerIndex(VestigeInteraction::ObjectChannel) == TEXT("Interactable")));
			ASSERT_THAT(IsTrue(Profiles->GetProfileTemplate(VestigeInteraction::CollisionProfile, Profile)));
			ASSERT_THAT(IsTrue(Profile.ObjectType == VestigeInteraction::ObjectChannel));
		}

		TEST_METHOD(FocusesTheNearestInteractableWithOptions)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			MakeInteractable(80.f, Take);
			AVestigeTestInteractable& Nearest = MakeInteractable(40.f, Open);

			Interaction.Scan();

			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Nearest));
			ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action == Open));
		}

		TEST_METHOD(SkipsAnInteractableThatOffersNothing)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			MakeInteractable(40.f);
			AVestigeTestInteractable& Offering = MakeInteractable(80.f, Open);

			Interaction.Scan();

			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Offering));
		}

		TEST_METHOD(IgnoresInteractablesOutOfRange)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			MakeInteractable(500.f, Open);

			Interaction.Scan();

			ASSERT_THAT(IsNull(Interaction.GetFocusedActor()));
			ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action.IsNone()));
		}

		TEST_METHOD(BroadcastsOnlyWhenTheFocusChanges)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Interactable = MakeInteractable(40.f, Open);
			int32 Changes = 0;
			Interaction.OnFocusChanged.AddLambda(
				[&Changes]()
				{
					++Changes;
				});

			Interaction.Scan();
			Interaction.Scan();
			ASSERT_THAT(IsTrue(Changes == 1));

			Interactable.SetActorLocation(FVector(500.f, 0.f, 0.f));
			Interaction.Scan();
			ASSERT_THAT(IsTrue(Changes == 2));
			ASSERT_THAT(IsNull(Interaction.GetFocusedActor()));
		}

		TEST_METHOD(InteractingUsesTheFirstOption)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Interactable = MakeInteractable(40.f, Open);
			Interactable.Offer(LOCTEXT("Take", "Take"), Take);

			ASSERT_THAT(IsTrue(Interaction.InteractWithFocus()));

			ASSERT_THAT(IsTrue(Interactable.GetInteractions().Num() == 1));
			ASSERT_THAT(IsTrue(Interactable.GetInteractions()[0] == Open));
		}

		TEST_METHOD(InteractingWithNothingInRangeDoesNothing)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Interactable = MakeInteractable(500.f, Open);

			ASSERT_THAT(IsFalse(Interaction.InteractWithFocus()));
			ASSERT_THAT(IsTrue(Interactable.GetInteractions().IsEmpty()));
		}

		TEST_METHOD(TheInteractKeyGoesThroughThePlayersAbility)
		{
			// The game's Blueprints, so the test also fails if DA_AbilitySet_Player doesn't grant the interact ability.
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			UClass* CharacterClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			ASSERT_THAT(IsNotNull(CharacterClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			AVestigePlayerCharacter& Character = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), CharacterClass);
			Character.SetPlayerState(&PlayerState);
			UVestigeInteractionComponent* Interaction = Character.FindComponentByClass<UVestigeInteractionComponent>();
			ASSERT_THAT(IsNotNull(Interaction));
			AVestigeTestInteractable& Interactable = MakeInteractable(0.f, Open);
			Interactable.SetActorLocation(Character.GetActorLocation() + FVector(40.f, 0.f, 0.f));

			Interaction->Interact();

			ASSERT_THAT(IsTrue(Interactable.GetInteractions().Num() == 1));
		}

		TEST_METHOD(TheBedSleepsAnActorWithARestComponent)
		{
			Spawner.InitializeGameSubsystems();
			const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
			Spawner.SpawnActor<AVestigeGameMode>().DispatchBeginPlay();
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AActor& Interactor = *Interaction.GetOwner();
			UVestigeRestComponent* Rest = NewObject<UVestigeRestComponent>(&Interactor);
			Rest->RegisterComponent();
			Interactor.DispatchBeginPlay();
			Spawner.SpawnActor<AVestigeBed>().SetActorLocation(FVector(100.f, 0.f, 0.f));

			Interaction.Scan();
			ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action == AVestigeBed::SleepAction));
			ASSERT_THAT(IsTrue(Interaction.InteractWithFocus()));

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
		}

		/** An interactable out of the way, offering Action at Location. */
		AVestigeTestInteractable& MakeInteractableAt(const FVector& Location, FName Action)
		{
			AVestigeTestInteractable& Interactable = MakeInteractable(60.f);
			Interactable.Offer(FText::FromName(Action), Action, Location);
			return Interactable;
		}

		TEST_METHOD(OnlyAnOptionInFrontCanBeFocused)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			const float Angle = FMath::DegreesToRadians(60.f);
			MakeInteractableAt(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 50.f, Open);

			Interaction.Scan();

			ASSERT_THAT(IsNull(Interaction.GetFocusedActor()));
		}

		TEST_METHOD(AmongFacedOptionsTheNearerWins)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			const float Wide = FMath::DegreesToRadians(40.f);
			const float Narrow = FMath::DegreesToRadians(5.f);
			AVestigeTestInteractable& Nearer = MakeInteractableAt(FVector(FMath::Cos(Wide), FMath::Sin(Wide), 0.f) * 40.f, Open);
			MakeInteractableAt(FVector(FMath::Cos(Narrow), FMath::Sin(Narrow), 0.f) * 80.f, Take);

			Interaction.Scan();

			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Nearer));
		}

		TEST_METHOD(AnOptionWithoutALocationIsFacedOnlyWhenTheFacingRayHitsItsShape)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Beside = MakeInteractable(0.f, Open);
			Beside.SetActorLocation(FVector(0.f, 50.f, 0.f));

			Interaction.Scan();
			ASSERT_THAT(IsNull(Interaction.GetFocusedActor()));

			Interaction.GetOwner()->SetActorRotation(FRotator(0.f, 90.f, 0.f));
			Interaction.Scan();
			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Beside));
		}

		TEST_METHOD(AnOptionsLocationIsUsedForItsAngle)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Behind = MakeInteractable(-50.f);
			Behind.Offer(FText::FromName(Open), Open, FVector(50.f, 0.f, 0.f));

			Interaction.Scan();

			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Behind));
		}

		TEST_METHOD(TheFacingDelegateIsUsedOverTheActorsRotation)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Beside = MakeInteractable(0.f, Open);
			Beside.SetActorLocation(FVector(0.f, 50.f, 0.f));
			Interaction.FacingDirection.BindLambda(
				[]()
				{
					return FVector(0.f, 1.f, 0.f);
				});

			Interaction.Scan();

			ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Beside));
		}

		TEST_METHOD(MovingBetweenOptionsWithTheSamePromptUpdatesTheLocation)
		{
			UVestigeInteractionComponent& Interaction = MakeInteractor();
			AVestigeTestInteractable& Interactable = MakeInteractableAt(FVector(40.f, 0.f, 0.f), Open);
			int32 Changes = 0;
			Interaction.OnFocusChanged.AddLambda(
				[&Changes]()
				{
					++Changes;
				});
			Interaction.Scan();

			Interactable.MoveOffersTo(FVector(60.f, 10.f, 0.f));
			Interaction.Scan();

			ASSERT_THAT(IsTrue(Changes == 1));
			ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Location.Equals(FVector(60.f, 10.f, 0.f))));
		}

		TEST_METHOD(TheBedOffersNothingToAnActorThatCantSleep)
		{
			AVestigeBed& Bed = Spawner.SpawnActor<AVestigeBed>();
			AActor& Visitor = Spawner.SpawnActor<AActor>();
			TArray<FVestigeInteractionOption> Options;

			IVestigeInteractable::Execute_GetInteractionOptions(&Bed, &Visitor, Options);

			ASSERT_THAT(IsTrue(Options.IsEmpty()));
		}
	};
}

#undef LOCTEXT_NAMESPACE

#endif
