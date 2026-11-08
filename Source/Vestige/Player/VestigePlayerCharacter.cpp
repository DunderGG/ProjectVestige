// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerCharacter.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTargetHighlightComponent.h"
#include "Farming/FarmTargetingComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Player/VestigeCharacterMovementComponent.h"
#include "Player/VestigeInputConfig.h"
#include "Player/VestigeInputLog.h"
#include "Player/VestigePlayerState.h"

#define LOCTEXT_NAMESPACE "VestigePlayerCharacter"

namespace FarmerInput
{
	/** The direction StepHotbarSlot takes for the hotbar's next and previous actions. */
	constexpr int32 NextSlot = 1;
	constexpr int32 PreviousSlot = -1;
}

namespace FarmerMesh
{
	/** The right hand bone on the UE5 mannequin skeleton. */
	const FName HandSocket(TEXT("hand_r"));
}

AVestigePlayerCharacter::AVestigePlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVestigeCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set here because the template's constructor sets its own rate after the movement component is created.
	GetCharacterMovement()->RotationRate = FRotator(0.f, VestigeMovement::TurnDegreesPerSecond, 0.f);

	Farming = CreateDefaultSubobject<UFarmingComponent>(TEXT("Farming"));
	Targeting = CreateDefaultSubobject<UFarmTargetingComponent>(TEXT("Targeting"));
	Interaction = CreateDefaultSubobject<UVestigeInteractionComponent>(TEXT("Interaction"));
	TargetHighlight = CreateDefaultSubobject<UFarmTargetHighlightComponent>(TEXT("TargetHighlight"));
	TargetHighlight->SetupAttachment(GetRootComponent());

	HeldTool = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldTool"));
	HeldTool->SetupAttachment(GetMesh(), FarmerMesh::HandSocket);
	HeldTool->SetMobility(EComponentMobility::Movable);
	HeldTool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldTool->SetGenerateOverlapEvents(false);
}

UAbilitySystemComponent* AVestigePlayerCharacter::GetAbilitySystemComponent() const
{
	const AVestigePlayerState* VestigePlayerState = GetPlayerState<AVestigePlayerState>();
	return VestigePlayerState ? VestigePlayerState->GetAbilitySystemComponent() : nullptr;
}

void AVestigePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Follow the component's tool so the hand also updates when a save is loaded or a slot is picked.
	Farming->OnToolChanged.AddUObject(this, &AVestigePlayerCharacter::ShowHeldTool);
	ShowHeldTool();

	// What the farmer means to use is what he aims at, not where his body points while he walks.
	Interaction->FacingDirection.BindUObject(Targeting.Get(), &UFarmTargetingComponent::GetAimDirection);

	// Holding the use key slows the walk, which belongs to the movement component.
	if (UVestigeCharacterMovementComponent* Movement = GetCharacterMovement<UVestigeCharacterMovementComponent>())
	{
		Farming->OnToolUseHeldChanged.AddUObject(Movement, &UVestigeCharacterMovementComponent::SetToolUseHeld);
	}
}

void AVestigePlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Farming->OnToolChanged.RemoveAll(this);
	Interaction->FacingDirection.Unbind();
	Farming->OnToolUseHeldChanged.RemoveAll(GetCharacterMovement());
	Super::EndPlay(EndPlayReason);
}

void AVestigePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// The template binds moving, looking and jumping through its own properties until it is replaced (CodeReview.md, M6).
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		BindNativeActions(*EnhancedInput);
	}
	AddMappingContexts();
}

void AVestigePlayerCharacter::BindNativeActions(UEnhancedInputComponent& EnhancedInput)
{
	if (!InputConfig)
	{
		VESTIGE_LOG(this, LogVestigeInput, Error, "No Input Config is set, so the farming keys are not bound.");
		return;
	}

	// Holding the key repeats the use, so walking along a row works it.
	if (const UInputAction* UseToolAction = InputConfig->FindNativeInputActionForTag(VestigeGameplayTags::Input_UseTool, true))
	{
		EnhancedInput.BindAction(UseToolAction, ETriggerEvent::Started, Farming.Get(), &UFarmingComponent::StartUsingTool);
		EnhancedInput.BindAction(UseToolAction, ETriggerEvent::Completed, Farming.Get(), &UFarmingComponent::StopUsingTool);
	}
	if (const UInputAction* InteractAction = InputConfig->FindNativeInputActionForTag(VestigeGameplayTags::Input_Interact, true))
	{
		EnhancedInput.BindAction(InteractAction, ETriggerEvent::Started, Interaction.Get(), &UVestigeInteractionComponent::Interact);
	}
	for (int32 Slot = 0; Slot < static_cast<int32>(UE_ARRAY_COUNT(VestigeGameplayTags::Input_HotbarSlots)); ++Slot)
	{
		if (const UInputAction* SlotAction = InputConfig->FindNativeInputActionForTag(*VestigeGameplayTags::Input_HotbarSlots[Slot], true))
		{
			EnhancedInput.BindAction(SlotAction, ETriggerEvent::Started, this, &AVestigePlayerCharacter::SelectHotbarSlot, Slot);
		}
	}

	// One wheel notch is one press; several notches in one frame are one step.
	if (const UInputAction* NextAction = InputConfig->FindNativeInputActionForTag(VestigeGameplayTags::Input_HotbarNext, true))
	{
		EnhancedInput.BindAction(NextAction, ETriggerEvent::Started, this, &AVestigePlayerCharacter::StepHotbarSlot, FarmerInput::NextSlot);
	}
	if (const UInputAction* PreviousAction = InputConfig->FindNativeInputActionForTag(VestigeGameplayTags::Input_HotbarPrevious, true))
	{
		EnhancedInput.BindAction(
			PreviousAction, ETriggerEvent::Started, this, &AVestigePlayerCharacter::StepHotbarSlot, FarmerInput::PreviousSlot);
	}
}

void AVestigePlayerCharacter::AddMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	// Registered with the player's Enhanced Input user settings, so remapped keys apply once rebinding is built. A context
	// already applied isn't added twice, it only takes the new priority.
	FModifyContextOptions Options;
	Options.bNotifyUserSettings = true;
	for (const FVestigeMappingContextAndPriority& Entry : MappingContexts)
	{
		if (Entry.MappingContext)
		{
			Subsystem->AddMappingContext(Entry.MappingContext, Entry.Priority, Options);
		}
	}
}

void AVestigePlayerCharacter::RemoveMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	// Default options, so the contexts stay registered with the user settings for the next possession.
	for (const FVestigeMappingContextAndPriority& Entry : MappingContexts)
	{
		if (Entry.MappingContext)
		{
			Subsystem->RemoveMappingContext(Entry.MappingContext);
		}
	}
}

void AVestigePlayerCharacter::UnPossessed()
{
	// The input component, and the use key's release binding with it, goes in Super, so a held key is released first.
	Farming->StopUsingTool();

	// Removed while the old controller is still known, so a later possession doesn't stack a second copy.
	RemoveMappingContexts();
	Super::UnPossessed();
}

void AVestigePlayerCharacter::DoMove(float Right, float Forward)
{
	const AController* CurrentController = GetController();
	if (!CurrentController)
	{
		return;
	}

	// The same camera-relative direction the template's DoMove walks in.
	const FRotationMatrix YawRotation(FRotator(0.f, CurrentController->GetControlRotation().Yaw, 0.f));
	const FVector MoveDirection = YawRotation.GetUnitAxis(EAxis::X) * Forward + YawRotation.GetUnitAxis(EAxis::Y) * Right;
	Targeting->AimAt(MoveDirection);
	if (UVestigeCharacterMovementComponent* Movement = GetCharacterMovement<UVestigeCharacterMovementComponent>())
	{
		Movement->SetIdleFacing(Targeting->GetAimDirection());
	}

	const float InputScale = GetMoveInputScale(GetMoveHeldSeconds(), MoveRampSeconds);
	Super::DoMove(Right * InputScale, Forward * InputScale);
}

float AVestigePlayerCharacter::GetMoveInputScale(float HeldSeconds, float RampSeconds)
{
	// Compared before dividing, so the endless hold of on-screen controls (TNumericLimits<float>::Max()) can't overflow.
	if (RampSeconds <= 0.f || HeldSeconds >= RampSeconds)
	{
		return 1.f;
	}
	return FMath::Max(HeldSeconds / RampSeconds, 0.f);
}

float AVestigePlayerCharacter::GetMoveHeldSeconds() const
{
	// Enhanced Input times every action from the moment it starts, and starts again on each press, so the ramp is read here
	// instead of from a modifier on the template's IA_Move. That keeps the move action and the template as they are, and
	// works the same for keys and the gamepad stick.
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	const UEnhancedPlayerInput* PlayerInput = Subsystem ? Subsystem->GetPlayerInput() : nullptr;
	const FInputActionInstance* MoveInstance = PlayerInput && MoveAction ? PlayerInput->FindActionInstanceData(MoveAction) : nullptr;

	// Without an action instance, DoMove came from on-screen controls, which walk at full strength.
	return MoveInstance ? MoveInstance->GetElapsedTime() : TNumericLimits<float>::Max();
}

UEnhancedInputLocalPlayerSubsystem* AVestigePlayerCharacter::GetInputSubsystem() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	return PlayerController ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) : nullptr;
}

TArray<FVestigeKeyHint> AVestigePlayerCharacter::GetHotbarKeyHints(
	const UVestigeInventoryComponent& Inventory, EVestigeHintDevice Device) const
{
	TArray<FVestigeKeyHint> Hints;
	const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
	const int32 HotbarSlotCount = FMath::Min(Inventory.GetHotbarSlotCount(), Slots.Num());

	// The gamepad has no slot keys: it steps through the hotbar, so its hint is the two stepping keys.
	if (Device == EVestigeHintDevice::Gamepad)
	{
		bool bHotbarHasAnItem = false;
		for (int32 Slot = 0; Slot < HotbarSlotCount; ++Slot)
		{
			bHotbarHasAnItem |= !Slots[Slot].IsEmpty();
		}
		const FKey PreviousKey = FindHintKey(FindActionForHint(VestigeGameplayTags::Input_HotbarPrevious), Device);
		const FKey NextKey = FindHintKey(FindActionForHint(VestigeGameplayTags::Input_HotbarNext), Device);
		if (bHotbarHasAnItem && PreviousKey.IsValid() && NextKey.IsValid())
		{
			Hints.Add({FText::FormatNamed(LOCTEXT("HotbarStepKeys", "{Previous} / {Next}"), TEXT("Previous"),
						   VestigeKeyHints::GetKeyName(PreviousKey), TEXT("Next"), VestigeKeyHints::GetKeyName(NextKey)),
				LOCTEXT("HotbarStepHint", "Hotbar")});
		}
		return Hints;
	}

	const int32 SlotCount = FMath::Min(HotbarSlotCount, static_cast<int32>(UE_ARRAY_COUNT(VestigeGameplayTags::Input_HotbarSlots)));
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		const UVestigeItemData* Item = Slots[Slot].Item;
		const FKey Key = FindHintKey(FindActionForHint(*VestigeGameplayTags::Input_HotbarSlots[Slot]), Device);
		if (Item && Key.IsValid())
		{
			Hints.Add({VestigeKeyHints::GetKeyName(Key), Item->GetDisplayName()});
		}
	}
	return Hints;
}

void AVestigePlayerCharacter::SelectHotbarSlot(int32 Slot)
{
	const AVestigePlayerState* VestigePlayerState = GetPlayerState<AVestigePlayerState>();
	if (UVestigeInventoryComponent* Inventory = VestigePlayerState ? VestigePlayerState->GetInventory() : nullptr)
	{
		Inventory->SelectHotbarSlot(Slot);
	}
}

void AVestigePlayerCharacter::StepHotbarSlot(int32 Direction)
{
	const AVestigePlayerState* VestigePlayerState = GetPlayerState<AVestigePlayerState>();
	if (UVestigeInventoryComponent* Inventory = VestigePlayerState ? VestigePlayerState->GetInventory() : nullptr)
	{
		Inventory->StepHotbarSlot(Direction);
	}
}

TArray<FVestigeKeyHint> AVestigePlayerCharacter::GetActionKeyHints(EVestigeHintDevice Device) const
{
	TArray<FVestigeKeyHint> Hints;
	if (!InputConfig)
	{
		return Hints;
	}
	for (const FVestigeInputAction& Entry : InputConfig->GetNativeInputActions())
	{
		if (!Entry.InputAction || Entry.InputAction->ActionDescription.IsEmpty())
		{
			continue;
		}
		const FKey Key = FindHintKey(Entry.InputAction, Device);
		if (Key.IsValid())
		{
			Hints.Add({VestigeKeyHints::GetKeyName(Key), Entry.InputAction->ActionDescription});
		}
	}
	return Hints;
}

FText AVestigePlayerCharacter::GetInteractKey(EVestigeHintDevice Device) const
{
	const FKey Key = FindHintKey(FindActionForHint(VestigeGameplayTags::Input_Interact), Device);
	return Key.IsValid() ? VestigeKeyHints::GetKeyName(Key) : FText::GetEmpty();
}

const UInputAction* AVestigePlayerCharacter::FindActionForHint(const FGameplayTag& Tag) const
{
	return InputConfig ? InputConfig->FindNativeInputActionForTag(Tag, false) : nullptr;
}

FKey AVestigePlayerCharacter::FindHintKey(const UInputAction* Action, EVestigeHintDevice Device) const
{
	// The assets' default mappings are right while keys can't be rebound. With rebinding, this asks Enhanced Input for the
	// player's current key instead (Roadmap.md, the settings item).
	if (!Action)
	{
		return EKeys::Invalid;
	}
	for (const FVestigeMappingContextAndPriority& Entry : MappingContexts)
	{
		if (!Entry.MappingContext)
		{
			continue;
		}
		for (const FEnhancedActionKeyMapping& Mapping : Entry.MappingContext->GetMappings())
		{
			if (Mapping.Action != Action)
			{
				continue;
			}
			const bool bOnDevice = Device == EVestigeHintDevice::Gamepad
				? Mapping.Key.IsGamepadKey()
				: !Mapping.Key.IsGamepadKey() && !Mapping.Key.IsTouch() && !Mapping.Key.IsGesture();
			if (bOnDevice)
			{
				return Mapping.Key;
			}
		}
	}
	return EKeys::Invalid;
}

void AVestigePlayerCharacter::ShowHeldTool()
{
	// Read now rather than passed, since a nested change may reach this handler out of order.
	const UVestigeToolData* Tool = Farming->GetCurrentTool();
	HeldTool->SetStaticMesh(Tool ? Tool->GetHeldMesh() : nullptr);
}

#if WITH_DEV_AUTOMATION_TESTS
UStaticMesh* AVestigePlayerCharacter::GetHeldToolMeshForTests() const
{
	return HeldTool->GetStaticMesh();
}

void AVestigePlayerCharacter::SetupPlayerInputComponentForTests(UInputComponent& TestInputComponent)
{
	SetupPlayerInputComponent(&TestInputComponent);
}
#endif

#undef LOCTEXT_NAMESPACE
