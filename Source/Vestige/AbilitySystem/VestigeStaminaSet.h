// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Core/VestigeSaveable.h"
#include "VestigeStaminaSet.generated.h"

class UAbilitySystemComponent;
struct FVestigeStaminaSaveData;

/**
 * The player's stamina: Stamina, from 0 to MaxStamina, which tool uses spend and sleep refills. Both change only through
 * gameplay effects (UVestigeStaminaCostEffect, UVestigeStaminaRefillEffect), and this set keeps them in range. While
 * Stamina is 0 it holds Vestige.State.OutOfStamina on its ability system, and it tells the player when stamina runs out.
 * Granted by an ability set (DA_AbilitySet_Player on BP_PlayerState).
 * Saves the stamina under Base.Stamina; the actor that holds its ability system registers it with UVestigeSaveRegistry,
 * since an attribute set has no BeginPlay or EndPlay of its own.
 */
UCLASS()
class UVestigeStaminaSet : public UAttributeSet, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	UVestigeStaminaSet();

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UVestigeStaminaSet, Stamina)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UVestigeStaminaSet, MaxStamina)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxStamina)

	/** The stamina AbilitySystem has, for saving. Left unsaved (bHasStamina false) if it has no stamina set. */
	static FVestigeStaminaSaveData GetSaveData(const UAbilitySystemComponent& AbilitySystem);

	/**
	 * Puts saved stamina back as its base value, kept in range, without replaying the effects that changed it.
	 * Does nothing for a save without stamina, or if AbilitySystem has no stamina set.
	 */
	static void RestoreFromSaveData(UAbilitySystemComponent& AbilitySystem, const FVestigeStaminaSaveData& Data);

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Fills Stamina up to MaxStamina, as a new player starts. */
	virtual void StartNewGame() override;

protected:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
	// Private, and only for the constructor: everything else changes the values through gameplay effects.
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxStamina)

	/** Keeps Stamina between 0 and MaxStamina, and MaxStamina at 0 or more. */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	/** What the player can spend on actions. Never below 0 or above MaxStamina. */
	UPROPERTY()
	FGameplayAttributeData Stamina;

	/** What sleep refills Stamina to. */
	UPROPERTY()
	FGameplayAttributeData MaxStamina;
};
