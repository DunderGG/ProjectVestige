// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "VestigeFeedbackTests"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeFeedbackTests
{
	TEST_CLASS(Feedback, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(MessagesReachEveryListener)
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			TArray<FText> Received;
			Feedback->OnFeedback.AddLambda(
				[&Received](const FText& Message)
				{
					Received.Add(Message);
				});

			UVestigeFeedbackSubsystem::Notify(&Spawner.GetWorld(), LOCTEXT("Tilled", "Tilled the soil."));

			ASSERT_THAT(IsTrue(Received.Num() == 1));
			ASSERT_THAT(IsTrue(Received[0].EqualTo(LOCTEXT("Tilled", "Tilled the soil."))));
		}
	};
}

#undef LOCTEXT_NAMESPACE

#endif
