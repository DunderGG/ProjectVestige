// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Engine/World.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeFeedbackTests
{
	TEST_CLASS(Feedback, "Vestige")
	{
		FActorTestSpawner Spawner;
		TArray<FVestigeFeedbackReport> Received;

		BEFORE_EACH()
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FVestigeFeedbackReport& Report)
				{
					Received.Add(Report);
				});
		}

		TEST_METHOD(ReportsReachEveryListener)
		{
			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 4}});

			ASSERT_THAT(IsTrue(Received.Num() == 1));
			ASSERT_THAT(IsTrue(Received[0].Tag == VestigeGameplayTags::Feedback_Day_Slept));
			const FFormatArgumentValue* Day = Received[0].Arguments.Find(TEXT("Day"));
			ASSERT_THAT(IsNotNull(Day));
			ASSERT_THAT(IsTrue(Day->GetType() == EFormatArgumentType::Int && Day->GetIntValue() == 4));
		}

		TEST_METHOD(AnInvalidTagIsNotReported)
		{
			TestRunner->AddExpectedMessagePlain(TEXT("A report without a valid tag was ignored."), ELogVerbosity::Warning);

			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), FGameplayTag());

			ASSERT_THAT(IsTrue(Received.IsEmpty()));
		}
	};
}

#endif
