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
	/** Collects the lines the feedback subsystem logs. */
	struct FReportLogCapture : public FOutputDevice
	{
		TArray<FString> Lines;

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == TEXT("LogVestigeFeedback"))
			{
				Lines.Add(Message);
			}
		}
	};

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

		TEST_METHOD(AReportCarriesItsClausesToEveryListener)
		{
			FVestigeFeedbackReport Report{VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 4}}};
			Report.Clauses.Add(FVestigeFeedbackClause{TEXT("Paid"), VestigeGameplayTags::Feedback_Shipping_Paid, {{TEXT("Amount"), 250}}});

			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), MoveTemp(Report));

			ASSERT_THAT(IsTrue(Received.Num() == 1));
			ASSERT_THAT(IsTrue(Received[0].Tag == VestigeGameplayTags::Feedback_Day_Slept));
			ASSERT_THAT(IsTrue(Received[0].Clauses.Num() == 1));
			const FVestigeFeedbackClause& Clause = Received[0].Clauses[0];
			ASSERT_THAT(IsTrue(Clause.Argument == FName(TEXT("Paid"))));
			ASSERT_THAT(IsTrue(Clause.Tag == VestigeGameplayTags::Feedback_Shipping_Paid));
			const FFormatArgumentValue* Amount = Clause.Arguments.Find(TEXT("Amount"));
			ASSERT_THAT(IsNotNull(Amount));
			ASSERT_THAT(IsTrue(Amount->GetType() == EFormatArgumentType::Int && Amount->GetIntValue() == 250));
		}

		TEST_METHOD(TheLogShowsAReportsClauses)
		{
			FReportLogCapture Capture;
			GLog->AddOutputDevice(&Capture);
			FVestigeFeedbackReport Filled{VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 4}}};
			Filled.Clauses.Add(FVestigeFeedbackClause{TEXT("Paid"), VestigeGameplayTags::Feedback_Shipping_Paid, {{TEXT("Amount"), 250}}});
			FVestigeFeedbackReport Empty{VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 5}}};
			Empty.Clauses.Add(FVestigeFeedbackClause{TEXT("Paid"), FGameplayTag(), {}});

			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), MoveTemp(Filled));
			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), MoveTemp(Empty));
			GLog->RemoveOutputDevice(&Capture);

			ASSERT_THAT(IsTrue(Capture.Lines.ContainsByPredicate(
				[](const FString& Line)
				{
					return Line.Contains(TEXT("Day=4")) && Line.Contains(TEXT("Paid=[Vestige.Feedback.Shipping.Paid Amount=250]"));
				})));
			ASSERT_THAT(IsTrue(Capture.Lines.ContainsByPredicate(
				[](const FString& Line)
				{
					return Line.Contains(TEXT("Day=5")) && Line.Contains(TEXT("Paid=[]"));
				})));
		}

		TEST_METHOD(AReportWithClausesAndNoTagIsNotReported)
		{
			FVestigeFeedbackReport Report{FGameplayTag(), {}};
			Report.Clauses.Add(FVestigeFeedbackClause{TEXT("Paid"), FGameplayTag(), {}});
			TestRunner->AddExpectedMessagePlain(TEXT("A report without a valid tag was ignored."), ELogVerbosity::Warning);

			UVestigeFeedbackSubsystem::Report(&Spawner.GetWorld(), MoveTemp(Report));

			ASSERT_THAT(IsTrue(Received.IsEmpty()));
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
