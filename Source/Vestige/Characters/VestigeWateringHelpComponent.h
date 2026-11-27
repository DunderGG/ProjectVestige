// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "VestigeWateringHelpComponent.generated.h"

class AFarmGrid;
class UFarmFieldSubsystem;

/**
 * A robot's dawn watering (Systems.md, section 10): once the player has accepted the help (the character's RequiredFlag), each
 * morning it waters up to MaxTiles dry, growing, unripe tiles nearest its body, with water from the farm's clean water store, and
 * sets HelpedFlag for the day so the character can say so. It runs when the farm's night is done (UFarmFieldSubsystem's
 * OnFieldsAdvanced), so it never waters a tile the night then dries. Added to the character's Blueprint (BP_Hob): the owner must be
 * an AVestigeNpcCharacter. Missing pieces (no player, grid, field or water) mean no watering and no error. Ticking off.
 */
UCLASS(ClassGroup = (Characters), meta = (BlueprintSpawnableComponent))
class UVestigeWateringHelpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeWateringHelpComponent();

	/** The flag the player's relationship must have set for the character, which says the help was accepted. */
	const FGameplayTag& GetRequiredFlag() const { return RequiredFlag; }

	/** The flag set for the day when the character watered at least one tile. */
	const FGameplayTag& GetHelpedFlag() const { return HelpedFlag; }

	/** Most tiles watered in a morning. */
	int32 GetMaxTiles() const { return MaxTiles; }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The Blueprint sets these. */
	void SetHelpForTests(const FGameplayTag& InRequiredFlag, const FGameplayTag& InHelpedFlag, int32 InMaxTiles)
	{
		RequiredFlag = InRequiredFlag;
		HelpedFlag = InHelpedFlag;
		MaxTiles = InMaxTiles;
	}
#endif

protected:
	/** Listens for the end of the farm's night. */
	virtual void BeginPlay() override;

	/** Stops listening. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Waters the nearest dry growing tiles, if the help was accepted. Bound to UFarmFieldSubsystem::OnFieldsAdvanced. */
	void WaterAtDawn(int32 Day);

	/** The flag that says the player accepted the help. Set on the Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Help", meta = (AllowPrivateAccess = "true"))
	FGameplayTag RequiredFlag;

	/** Set for the day when at least one tile was watered. Set on the Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Help", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HelpedFlag;

	/** The most tiles watered each morning, at least 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Help", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 MaxTiles = 4;

	/** The grid, found on the first morning it is needed. */
	TWeakObjectPtr<AFarmGrid> Grid;

	/** The subsystem listened to, so EndPlay unbinds from the same one. */
	TWeakObjectPtr<UFarmFieldSubsystem> Fields;
};
