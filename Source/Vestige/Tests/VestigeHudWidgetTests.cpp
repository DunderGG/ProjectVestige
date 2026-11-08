// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/ActorTestSpawner.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "CQTest.h"
#include "GameFramework/GameModeBase.h"
#include "UI/VestigeHUD.h"
#include "UI/VestigeHudMessageWidget.h"
#include "UI/VestigeHudMessages.h"
#include "UI/VestigeHudWidget.h"

#define LOCTEXT_NAMESPACE "VestigeHudWidgetTests"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHudWidgetTests
{
	const TCHAR* const HudWidgetPath = TEXT("/Game/Vestige/UI/WBP_Hud.WBP_Hud_C");
	const TCHAR* const MessageWidgetPath = TEXT("/Game/Vestige/UI/WBP_HudMessage.WBP_HudMessage_C");
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");
	constexpr double Lifetime = 3.0;

	/** The text a message box shows. */
	FText GetMessageBoxText(const UWidget* Box)
	{
		const UUserWidget* MessageBox = Cast<UUserWidget>(Box);
		const UTextBlock* Text = MessageBox ? Cast<UTextBlock>(MessageBox->GetWidgetFromName(TEXT("MessageText"))) : nullptr;
		return Text ? Text->GetText() : FText::GetEmpty();
	}

	/** The widgets are presentation, so these check that the assets hold together and that each part follows what it is told. */
	TEST_CLASS(UI, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeHudWidget* Hud = nullptr;

		BEFORE_EACH()
		{
			UClass* HudClass = LoadClass<UVestigeHudWidget>(nullptr, HudWidgetPath);
			ASSERT_THAT(IsNotNull(HudClass));
			Hud = CreateWidget<UVestigeHudWidget>(&Spawner.GetWorld(), HudClass);
			ASSERT_THAT(IsNotNull(Hud));
		}

		TEST_METHOD(TheGameModeShowsTheHudWidget)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));

			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));

			const TSubclassOf<UVestigeHudWidget> WidgetClass = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetHudWidgetClass();
			ASSERT_THAT(IsNotNull(WidgetClass.Get()));
			ASSERT_THAT(IsNotNull(WidgetClass->GetDefaultObject<UVestigeHudWidget>()->GetMessageWidgetClass().Get()));
		}

		TEST_METHOD(OnlyTheMessageBoxesTick)
		{
			const UWidgetBlueprintGeneratedClass* HudClass =
				Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr, HudWidgetPath));
			const UWidgetBlueprintGeneratedClass* MessageClass =
				Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr, MessageWidgetPath));
			ASSERT_THAT(IsNotNull(HudClass));
			ASSERT_THAT(IsNotNull(MessageClass));

			ASSERT_THAT(IsFalse(HudClass->ClassRequiresNativeTick()));
			ASSERT_THAT(IsTrue(MessageClass->ClassRequiresNativeTick()));
		}

		TEST_METHOD(TheHudDoesNotTakeTheMouse) { ASSERT_THAT(IsTrue(Hud->GetVisibility() == ESlateVisibility::HitTestInvisible)); }

		TEST_METHOD(EachMessageShowsInItsOwnBox)
		{
			FVestigeHudMessages Messages;
			Messages.Add(LOCTEXT("Slept", "You slept."), 1.0, 5);
			Messages.Add(LOCTEXT("Exhausted", "You wake up exhausted."), 1.0, 5);
			Messages.Add(LOCTEXT("Saved", "Game saved."), 1.0, 5);
			Hud->ShowMessages(Messages.GetMessages(), Lifetime);

			const UPanelWidget* MessageList = Cast<UPanelWidget>(Hud->GetWidgetFromName(TEXT("MessageList")));
			ASSERT_THAT(IsNotNull(MessageList));
			ASSERT_THAT(IsTrue(MessageList->GetChildrenCount() == 3));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				ASSERT_THAT(IsTrue(MessageList->GetChildAt(Index)->GetVisibility() == ESlateVisibility::HitTestInvisible));
			}
			ASSERT_THAT(IsTrue(GetMessageBoxText(MessageList->GetChildAt(2)).EqualTo(LOCTEXT("Saved", "Game saved."))));

			Messages.RemoveExpired(1.0 + Lifetime, Lifetime);
			Messages.Add(LOCTEXT("Harvested", "Harvested 2 Turnip."), 5.0, 5);
			Hud->ShowMessages(Messages.GetMessages(), Lifetime);

			ASSERT_THAT(IsTrue(MessageList->GetChildrenCount() == 3));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(0)->GetVisibility() == ESlateVisibility::HitTestInvisible));
			ASSERT_THAT(IsTrue(GetMessageBoxText(MessageList->GetChildAt(0)).EqualTo(LOCTEXT("Harvested", "Harvested 2 Turnip."))));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(1)->GetVisibility() == ESlateVisibility::Collapsed));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(2)->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(AMessageFadesInAndOut)
		{
			UClass* MessageClass = LoadClass<UVestigeHudMessageWidget>(nullptr, MessageWidgetPath);
			ASSERT_THAT(IsNotNull(MessageClass));
			UVestigeHudMessageWidget* MessageBox = CreateWidget<UVestigeHudMessageWidget>(&Spawner.GetWorld(), MessageClass);
			ASSERT_THAT(IsNotNull(MessageBox));

			constexpr double StartTime = 10.0;
			MessageBox->ShowMessage(LOCTEXT("Saved", "Game saved."), StartTime, Lifetime);
			for (const double Now : {StartTime, StartTime + VestigeHudMessageFade::FadeInSeconds / 2.0, StartTime + Lifetime / 2.0,
					 StartTime + Lifetime - VestigeHudMessageFade::FadeOutSeconds / 2.0, StartTime + Lifetime})
			{
				MessageBox->UpdateOpacity(Now);
				ASSERT_THAT(IsTrue(
					FMath::IsNearlyEqual(MessageBox->GetRenderOpacity(), FVestigeHudMessages::GetOpacity(StartTime, Now, Lifetime))));
			}
			MessageBox->UpdateOpacity(StartTime + Lifetime / 2.0);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(MessageBox->GetRenderOpacity(), 1.f)));
		}

		TEST_METHOD(ThePromptHidesWhenEmpty)
		{
			const UWidget* PromptBox = Hud->GetWidgetFromName(TEXT("PromptBox"));
			ASSERT_THAT(IsNotNull(PromptBox));

			Hud->SetPromptText(FText::GetEmpty());
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));

			Hud->SetPromptText(LOCTEXT("Sleep", "F Sleep"));
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::HitTestInvisible));

			Hud->SetPromptText(FText::GetEmpty());
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(TheStaminaLineWarnsWhileOutOfStamina)
		{
			const UTextBlock* StaminaText = Cast<UTextBlock>(Hud->GetWidgetFromName(TEXT("StaminaText")));
			ASSERT_THAT(IsNotNull(StaminaText));

			Hud->SetStamina(LOCTEXT("Full", "Stamina 100 / 100"), 100, 100, false, false);
			const FLinearColor Normal = StaminaText->GetColorAndOpacity().GetSpecifiedColor();

			Hud->SetStamina(LOCTEXT("Empty", "Stamina 0 / 100"), 0, 100, true, false);
			ASSERT_THAT(IsFalse(StaminaText->GetColorAndOpacity().GetSpecifiedColor().Equals(Normal)));
			ASSERT_THAT(IsTrue(StaminaText->GetText().EqualTo(LOCTEXT("Empty", "Stamina 0 / 100"))));

			Hud->SetStamina(LOCTEXT("Full", "Stamina 100 / 100"), 100, 100, false, false);
			ASSERT_THAT(IsTrue(StaminaText->GetColorAndOpacity().GetSpecifiedColor().Equals(Normal)));
		}
	};
}

#undef LOCTEXT_NAMESPACE

#endif
