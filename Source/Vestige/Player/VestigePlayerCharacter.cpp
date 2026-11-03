// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/VestigeToolData.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTargetHighlightComponent.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Player/VestigeCharacterMovementComponent.h"
#include "Player/VestigePlayerState.h"
#include "Save/VestigeSaveSubsystem.h"

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
	Rest = CreateDefaultSubobject<UVestigeRestComponent>(TEXT("Rest"));
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

	// Follow the component's tool so the hand also updates when a save is loaded.
	Farming->OnToolChanged.AddUObject(this, &AVestigePlayerCharacter::ShowHeldTool);
	ShowHeldTool(Farming->GetCurrentTool());

	// Holding the use key slows the walk, which belongs to the movement component.
	if (UVestigeCharacterMovementComponent* Movement = GetCharacterMovement<UVestigeCharacterMovementComponent>())
	{
		Farming->OnToolUseHeldChanged.AddUObject(Movement, &UVestigeCharacterMovementComponent::SetToolUseHeld);
	}
}

void AVestigePlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Farming->OnToolChanged.RemoveAll(this);
	Farming->OnToolUseHeldChanged.RemoveAll(GetCharacterMovement());
	Super::EndPlay(EndPlayReason);
}

void AVestigePlayerCharacter::CreateInput()
{
	FarmingMappingContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);

	UseToolAction = AddInputAction(EKeys::E, LOCTEXT("UseTool", "Use tool"));
	InteractAction = AddInputAction(EKeys::F, LOCTEXT("Interact", "Interact"));
	SkipDayAction = AddInputAction(EKeys::N, LOCTEXT("SkipDay", "Skip day (test)"));
	NewGameAction = AddInputAction(EKeys::F9, LOCTEXT("NewGame", "New game"));

	// The number keys pick toolbar slots. Their hints name the tool in the slot, so the actions have no description.
	const FKey ToolbarKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	for (const FKey& Key : ToolbarKeys)
	{
		ToolbarSlotActions.Add(AddInputAction(Key, FText::GetEmpty()));
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
	EnhancedInput->BindAction(SkipDayAction, ETriggerEvent::Started, Rest.Get(), &UVestigeRestComponent::Sleep);
	EnhancedInput->BindAction(NewGameAction, ETriggerEvent::Started, this, &AVestigePlayerCharacter::StartNewGame);
	for (int32 Slot = 0; Slot < ToolbarSlotActions.Num(); ++Slot)
	{
		EnhancedInput->BindAction(
			ToolbarSlotActions[Slot], ETriggerEvent::Started, Farming.Get(), &UFarmingComponent::SelectToolbarSlot, Slot);
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
	Farming->AimAt(MoveDirection);
	if (UVestigeCharacterMovementComponent* Movement = GetCharacterMovement<UVestigeCharacterMovementComponent>())
	{
		Movement->SetIdleFacing(Farming->GetAimDirection());
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

TArray<FVestigeKeyHint> AVestigePlayerCharacter::GetToolbarKeyHints() const
{
	TArray<FVestigeKeyHint> Hints;
	const TArray<TObjectPtr<const UVestigeToolData>>& Tools = Farming->GetToolbar().GetTools();
	for (int32 Slot = 0; Slot < FMath::Min(Tools.Num(), ToolbarSlotActions.Num()); ++Slot)
	{
		if (const UVestigeToolData* Tool = Tools[Slot])
		{
			Hints.Add({GetMappedKey(ToolbarSlotActions[Slot]).GetDisplayName(false), Tool->GetDisplayName()});
		}
	}
	return Hints;
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

void AVestigePlayerCharacter::StartNewGame()
{
	if (UVestigeSaveSubsystem* Saves = GetWorld()->GetSubsystem<UVestigeSaveSubsystem>())
	{
		Saves->StartNewGame();
	}
}

void AVestigePlayerCharacter::ShowHeldTool(const UVestigeToolData* Tool)
{
	HeldTool->SetStaticMesh(Tool ? Tool->GetHeldMesh() : nullptr);
}

#undef LOCTEXT_NAMESPACE
