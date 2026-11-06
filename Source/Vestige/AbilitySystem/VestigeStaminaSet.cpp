// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeStaminaSet.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSaveData.h"
#include "AbilitySystemComponent.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "VestigeStaminaSet"

namespace VestigeStamina
{
	/** The player's stamina at the start of a game, until characters get their starting stats from data. */
	constexpr float StartingMaxStamina = 100.f;
}

UVestigeStaminaSet::UVestigeStaminaSet()
{
	InitMaxStamina(VestigeStamina::StartingMaxStamina);
	InitStamina(VestigeStamina::StartingMaxStamina);
}

FVestigeStaminaSaveData UVestigeStaminaSet::GetSaveData(const UAbilitySystemComponent& AbilitySystem)
{
	FVestigeStaminaSaveData Data;
	Data.bHasStamina = AbilitySystem.HasAttributeSetForAttribute(GetStaminaAttribute());
	if (Data.bHasStamina)
	{
		// The base value, so a temporary effect active at the time isn't saved into it.
		Data.Stamina = AbilitySystem.GetNumericAttributeBase(GetStaminaAttribute());
	}
	return Data;
}

void UVestigeStaminaSet::RestoreFromSaveData(UAbilitySystemComponent& AbilitySystem, const FVestigeStaminaSaveData& Data)
{
	if (!Data.bHasStamina || !AbilitySystem.HasAttributeSetForAttribute(GetStaminaAttribute()))
	{
		return;
	}
	AbilitySystem.SetNumericAttributeBase(GetStaminaAttribute(), Data.Stamina);
}

FName UVestigeStaminaSet::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Stamina"));
	return Key;
}

int32 UVestigeStaminaSet::GetSaveVersion() const
{
	return FVestigeStaminaSaveData::Version;
}

FVestigeSaveRecord UVestigeStaminaSet::WriteSaveRecord() const
{
	FVestigeStaminaSaveData Data;
	if (const UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent())
	{
		Data = GetSaveData(*AbilitySystem);
	}
	return FVestigeSaveRecord::Write(Data);
}

bool UVestigeStaminaSet::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeStaminaSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	if (UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent())
	{
		RestoreFromSaveData(*AbilitySystem, Data);
	}
	return true;
}

void UVestigeStaminaSet::StartNewGame()
{
	if (UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent())
	{
		AbilitySystem->SetNumericAttributeBase(GetStaminaAttribute(), GetMaxStamina());
	}
}

void UVestigeStaminaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UVestigeStaminaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UVestigeStaminaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent();
	if (Attribute != GetStaminaAttribute() || !AbilitySystem)
	{
		return;
	}

	// A tag kept in step with the value rather than an effect of its own, so it can't drift from the number or need saving.
	const bool bOutOfStamina = NewValue <= 0.f;
	AbilitySystem->SetLooseGameplayTagCount(VestigeGameplayTags::State_OutOfStamina, bOutOfStamina ? 1 : 0);
	const AActor* OwningActor = GetOwningActor();
	if (bOutOfStamina && OldValue > 0.f && OwningActor)
	{
		UVestigeFeedbackSubsystem::Notify(
			OwningActor->GetWorld(), LOCTEXT("OutOfStamina", "You're out of stamina. Everything takes longer until you sleep."));
	}
}

void UVestigeStaminaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

#undef LOCTEXT_NAMESPACE
