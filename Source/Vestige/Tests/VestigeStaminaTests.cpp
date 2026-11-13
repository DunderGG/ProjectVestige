// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSaveData.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeGameData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Engine/AssetManager.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Player/VestigePlayerState.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeStaminaTests
{
	TEST_CLASS(Stamina, "Vestige")
	{
		FActorTestSpawner Spawner;
		UAbilitySystemComponent* AbilitySystem = nullptr;

		/** The player state Blueprint the game uses, whose ability set grants the stamina set. */
		BEFORE_EACH()
		{
			// The starting items are found through the content catalogue of the game instance.
			Spawner.InitializeGameSubsystems();
			// The stamina effects are GE_ assets named by the game data, which the asset manager loaded at startup.
			const UVestigeGameData& GameData = UVestigeGameData::Get();
			ASSERT_THAT(IsNotNull(GameData.GetStaminaCostEffect().Get()));
			ASSERT_THAT(IsNotNull(GameData.GetStaminaRefillEffect().Get()));
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState.DispatchBeginPlay();
			AbilitySystem = PlayerState.GetAbilitySystemComponent();
			ASSERT_THAT(IsNotNull(AbilitySystem));
		}

		float GetStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()); }
		float GetMaxStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()); }
		bool IsOutOfStamina() const { return AbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_OutOfStamina); }

		/** Spends Cost stamina through the cost effect, as a tool use does. */
		void Spend(float Cost)
		{
			const FGameplayEffectSpecHandle Spec =
				AbilitySystem->MakeOutgoingSpec(UVestigeGameData::Get().GetStaminaCostEffect(), 1.f, AbilitySystem->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -Cost);
			AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}

		/** Sets stamina to Amount through the refill effect, as sleeping does. */
		void Refill(float Amount)
		{
			const FGameplayEffectSpecHandle Spec =
				AbilitySystem->MakeOutgoingSpec(UVestigeGameData::Get().GetStaminaRefillEffect(), 1.f, AbilitySystem->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaRefill, Amount);
			AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}

		TEST_METHOD(ThePlayerStartsWithFullStamina)
		{
			ASSERT_THAT(IsTrue(AbilitySystem->HasAttributeSetForAttribute(UVestigeStaminaSet::GetStaminaAttribute())));
			ASSERT_THAT(IsTrue(GetMaxStamina() > 0.f));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsFalse(IsOutOfStamina()));
		}

		static const UVestigeToolData* LoadTool(const TCHAR* ContentId)
		{
			return Cast<UVestigeToolData>(
				UAssetManager::Get().GetPrimaryAssetPath(FPrimaryAssetId(UVestigeToolData::AssetType, ContentId)).TryLoad());
		}

		/** The first-season guarantee: till, plant and water every starting seed in the first day's stamina. */
		TEST_METHOD(TheFirstPlantingFitsInADaysStamina)
		{
			const UVestigeInventoryComponent* Inventory = AbilitySystem->GetOwner()->FindComponentByClass<UVestigeInventoryComponent>();
			ASSERT_THAT(IsNotNull(Inventory));
			const UVestigeToolData* Hoe = LoadTool(TEXT("Base.Hoe"));
			const UVestigeToolData* Seeds = LoadTool(TEXT("Base.Seeds"));
			const UVestigeToolData* WateringCan = LoadTool(TEXT("Base.WateringCan"));
			ASSERT_THAT(IsTrue(Hoe && Seeds && WateringCan));
			const float CostPerSeed = Hoe->GetStaminaCost() + Seeds->GetStaminaCost() + WateringCan->GetStaminaCost();

			int32 SeedCount = 0;
			for (const FVestigeInventorySlot& Slot : Inventory->GetSlots())
			{
				if (Slot.Item && Slot.Item->FindFragment<UFarmSeedFragment>())
				{
					SeedCount += Slot.Count;
				}
			}

			ASSERT_THAT(IsTrue(SeedCount > 0));
			ASSERT_THAT(IsTrue(SeedCount * CostPerSeed <= GetMaxStamina()));
		}

		TEST_METHOD(RunningOutReportsOnce)
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			TArray<FGameplayTag> Reports;
			Feedback->OnFeedback.AddLambda(
				[&Reports](const FVestigeFeedbackReport& Report)
				{
					Reports.Add(Report.Tag);
				});

			Spend(GetMaxStamina() - 1.f);
			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			Spend(5.f);
			Spend(5.f);

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Stamina_Empty}));
		}

		TEST_METHOD(ACostIsTakenFromStamina)
		{
			Spend(3.f);

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - 3.f));
		}

		TEST_METHOD(StaminaNeverGoesBelowZero)
		{
			Spend(GetMaxStamina() + 50.f);
			Spend(5.f);

			ASSERT_THAT(IsTrue(GetStamina() == 0.f));
		}

		TEST_METHOD(ZeroStaminaHoldsTheOutOfStaminaTag)
		{
			Spend(GetMaxStamina());

			ASSERT_THAT(IsTrue(IsOutOfStamina()));
		}

		TEST_METHOD(RefillingFillsStaminaAndClearsTheTag)
		{
			Spend(GetMaxStamina());

			Refill(GetMaxStamina());

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsFalse(IsOutOfStamina()));
		}

		TEST_METHOD(ARefillAboveTheMaximumIsClamped)
		{
			Spend(10.f);

			Refill(GetMaxStamina() + 50.f);

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(StaminaRoundTripsThroughTheSave)
		{
			Spend(30.f);
			const FVestigeStaminaSaveData Data = UVestigeStaminaSet::GetSaveData(*AbilitySystem);
			Refill(GetMaxStamina());

			UVestigeStaminaSet::RestoreFromSaveData(*AbilitySystem, Data);

			ASSERT_THAT(IsTrue(Data.bHasStamina));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - 30.f));
		}

		TEST_METHOD(ASavedZeroRestoresTheOutOfStaminaTag)
		{
			FVestigeStaminaSaveData Data;
			Data.bHasStamina = true;
			Data.Stamina = 0.f;

			UVestigeStaminaSet::RestoreFromSaveData(*AbilitySystem, Data);

			ASSERT_THAT(IsTrue(IsOutOfStamina()));
		}

		TEST_METHOD(ASaveWithoutStaminaKeepsItFull)
		{
			UVestigeStaminaSet::RestoreFromSaveData(*AbilitySystem, FVestigeStaminaSaveData());

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(SavedStaminaAboveTheMaximumIsClamped)
		{
			FVestigeStaminaSaveData Data;
			Data.bHasStamina = true;
			Data.Stamina = GetMaxStamina() + 20.f;
			Spend(10.f);

			UVestigeStaminaSet::RestoreFromSaveData(*AbilitySystem, Data);

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}
	};
}

#endif
