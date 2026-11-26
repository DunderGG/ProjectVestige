// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AIController.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Components/ActorTestSpawner.h"
#include "Components/CapsuleComponent.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "CQTest.h"
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
	};
}

#endif
