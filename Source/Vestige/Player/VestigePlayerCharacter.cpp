// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerCharacter.h"

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
#include "Player/VestigePlayerState.h"

#define LOCTEXT_NAMESPACE "VestigePlayerCharacter"

namespace FarmerInput
{
	/** Above the template's movement contexts (priority 0), so farming keys win if a key is ever mapped twice. */
	constexpr int32 MappingContextPriority = 1;
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

void AVestigePlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CreateInput();
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

void AVestigePlayerCharacter::CreateInput()
{
	FarmingMappingContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);

	UseToolAction = AddInputAction(EKeys::E, LOCTEXT("UseTool", "Use tool"));
	InteractAction = AddInputAction(EKeys::F, LOCTEXT("Interact", "Interact"));

	// The number keys pick hotbar slots. Their hints name the item in the slot, so the actions have no description.
	const FKey HotbarKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	for (const FKey& Key : HotbarKeys)
	{
		HotbarSlotActions.Add(AddInputAction(Key, FText::GetEmpty()));
	}
}

UInputAction* AVestigePlayerCharacter::AddInputAction(const FKey& Key, const FText& Description)
{
	UInputAction* Action = NewObject<UInputAction>(this, NAME_None, RF_Transient);
	Action->ActionDescription = Description;
	FarmingMappingContext->MapKey(Action, Key);
	return Action;
}

void AVestigePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	// Holding the key repeats the use, so walking along a row works it.
	EnhancedInput->BindAction(UseToolAction, ETriggerEvent::Started, Farming.Get(), &UFarmingComponent::StartUsingTool);
	EnhancedInput->BindAction(UseToolAction, ETriggerEvent::Completed, Farming.Get(), &UFarmingComponent::StopUsingTool);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, Interaction.Get(), &UVestigeInteractionComponent::Interact);
	for (int32 Slot = 0; Slot < HotbarSlotActions.Num(); ++Slot)
	{
		EnhancedInput->BindAction(HotbarSlotActions[Slot], ETriggerEvent::Started, this, &AVestigePlayerCharacter::SelectHotbarSlot, Slot);
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem())
	{
		Subsystem->AddMappingContext(FarmingMappingContext, FarmerInput::MappingContextPriority);
	}
}

void AVestigePlayerCharacter::UnPossessed()
{
	// Removed while the old controller is still known, so a later possession doesn't stack a second copy.
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem())
	{
		Subsystem->RemoveMappingContext(FarmingMappingContext);
	}
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

TArray<FVestigeKeyHint> AVestigePlayerCharacter::GetHotbarKeyHints(const UVestigeInventoryComponent& Inventory) const
{
	TArray<FVestigeKeyHint> Hints;
	const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
	const int32 SlotCount = FMath::Min(Inventory.GetHotbarSlotCount(), HotbarSlotActions.Num());
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		if (const UVestigeItemData* Item = Slots[Slot].Item)
		{
			Hints.Add({GetMappedKey(HotbarSlotActions[Slot]).GetDisplayName(false), Item->GetDisplayName()});
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

TArray<FVestigeKeyHint> AVestigePlayerCharacter::GetActionKeyHints() const
{
	TArray<FVestigeKeyHint> Hints;
	if (!FarmingMappingContext)
	{
		return Hints;
	}
	for (const FEnhancedActionKeyMapping& Mapping : FarmingMappingContext->GetMappings())
	{
		if (Mapping.Action && !Mapping.Action->ActionDescription.IsEmpty())
		{
			Hints.Add({Mapping.Key.GetDisplayName(false), Mapping.Action->ActionDescription});
		}
	}
	return Hints;
}

FText AVestigePlayerCharacter::GetInteractKey() const
{
	const FKey Key = GetMappedKey(InteractAction);
	return Key.IsValid() ? Key.GetDisplayName(false) : FText::GetEmpty();
}

FKey AVestigePlayerCharacter::GetMappedKey(const UInputAction* Action) const
{
	if (FarmingMappingContext)
	{
		for (const FEnhancedActionKeyMapping& Mapping : FarmingMappingContext->GetMappings())
		{
			if (Mapping.Action == Action)
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
#endif

#undef LOCTEXT_NAMESPACE
