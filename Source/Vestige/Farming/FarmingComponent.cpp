// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmingComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SceneComponent.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeContentData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/World.h"
#include "Farming/FarmFarmingSaveData.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmTileTargeting.h"
#include "GameFramework/Pawn.h"
#include "GameplayAbilitySpec.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FarmingComponent"

UFarmingComponent::UFarmingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFarmingComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadSeedCrop();
	LoadToolbar();

	// After the toolbar loads, since restoring the tool in hand looks for it there.
	UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *this);

	Grid = AFarmGrid::Find(GetWorld());
	if (Grid)
	{
		// The grid finds its field in its own BeginPlay, which may run after this one.
		Grid->OnFieldReady.AddUObject(this, &UFarmingComponent::UpdateTargetTile);

		// Until the first movement input, aim where the owner faces.
		const FIntPoint FacingStep = FarmTileTargeting::SnapToStep(Grid->ToGridDirection(GetOwner()->GetActorForwardVector()));
		if (FacingStep != FIntPoint::ZeroValue)
		{
			AimStep = FacingStep;
		}
		OwnerCoordinates = Grid->GetCoordinatesAt(GetOwner()->GetActorLocation());
	}
	else
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The level {Level} has no FarmGrid, so there is nothing to farm.", GetWorld()->GetMapName());

		// The HUD may not be listening for feedback yet during BeginPlay.
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			[WeakThis = TWeakObjectPtr<UFarmingComponent>(this)]()
			{
				if (const UFarmingComponent* Farming = WeakThis.Get())
				{
					Farming->Notify(LOCTEXT("NoGrid", "No FarmGrid in the level. Drag one in from the Place Actors panel."));
				}
			});
	}

	if (USceneComponent* Root = GetOwner() ? GetOwner()->GetRootComponent() : nullptr)
	{
		Root->TransformUpdated.AddUObject(this, &UFarmingComponent::FollowOwnerTransform);
	}
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UFarmingComponent::FollowController);
	}
	UpdateTargetTile();
}

void UFarmingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *this);

	// By now the owner is unpossessed and reaches no ability system, so the grant goes through the one it was made on.
	ReleaseGrantedTool();

	if (Grid)
	{
		Grid->OnFieldReady.RemoveAll(this);
	}
	if (USceneComponent* Root = GetOwner() ? GetOwner()->GetRootComponent() : nullptr)
	{
		Root->TransformUpdated.RemoveAll(this);
	}
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UFarmingComponent::FollowController);
	}
	Super::EndPlay(EndPlayReason);
}

void UFarmingComponent::LoadSeedCrop()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmingComponent::LoadSeedCrop);

	// Waiting is fine at play start, and with the meshes loaded now, planting never stalls on a load.
	const UVestigeItemData* SeedItem = Cast<UVestigeItemData>(UVestigeContentData::LoadWithVisuals(SeedItemId));
	const UFarmSeedFragment* Seed = SeedItem ? SeedItem->FindFragment<UFarmSeedFragment>() : nullptr;
	if (!Seed)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The Seed Item Id ({ItemId}) names no item with a seed fragment, so nothing can be planted.",
			SeedItemId.ToString());
		return;
	}
	SeedCrop = Cast<UFarmCropData>(UVestigeContentData::LoadWithVisuals(Seed->GetCropId()));
}

void UFarmingComponent::LoadToolbar()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmingComponent::LoadToolbar);

	if (ToolbarIds.IsEmpty())
	{
		VESTIGE_LOG(this, LogFarm, Warning, "No Toolbar Ids are set, so the farmer has no tools.");
	}

	// A missing tool leaves its slot empty, so the other tools keep their keys.
	TArray<TObjectPtr<const UVestigeToolData>> Tools;
	for (const FPrimaryAssetId& ToolId : ToolbarIds)
	{
		const UVestigeToolData* Tool = Cast<UVestigeToolData>(UVestigeContentData::LoadWithVisuals(ToolId));
		if (!Tool)
		{
			VESTIGE_LOG(this, LogFarm, Warning, "Toolbar slot {Slot} ({ToolId}) names no tool asset, so the slot is empty.",
				Tools.Num() + 1, ToolId.ToString());
		}
		else if (!Tool->GetAbility())
		{
			VESTIGE_LOG(this, LogFarm, Warning, "The tool {ToolId} names no Ability, so it does nothing.", ToolId.ToString());
		}
		Tools.Add(Tool);
	}
	Toolbar.SetTools(MoveTemp(Tools));
	HandleToolChanged();
}

void UFarmingComponent::SelectToolbarSlot(int32 Slot)
{
	if (Toolbar.SelectSlot(Slot))
	{
		HandleToolChanged();
	}
}

bool UFarmingComponent::RestoreCurrentTool(FName ToolContentId)
{
	const bool bFound = Toolbar.SelectTool(ToolContentId);
	if (!bFound)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The saved tool {ToolId} isn't on the toolbar, so the first slot is in hand.", ToolContentId);
		Toolbar.SelectSlot(0);
	}
	HandleToolChanged();
	return bFound;
}

void UFarmingComponent::HandleToolChanged()
{
	UpdateGrantedTool();
	OnToolChanged.Broadcast(GetCurrentTool());
}

FFarmFarmingSaveData UFarmingComponent::GetSaveData() const
{
	FFarmFarmingSaveData Data;
	const UVestigeToolData* Tool = GetCurrentTool();
	Data.CurrentToolId = Tool ? Tool->GetContentId() : NAME_None;
	return Data;
}

bool UFarmingComponent::RestoreFromSaveData(const FFarmFarmingSaveData& Data)
{
	return RestoreCurrentTool(Data.CurrentToolId);
}

FName UFarmingComponent::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Farming"));
	return Key;
}

int32 UFarmingComponent::GetSaveVersion() const
{
	return FFarmFarmingSaveData::Version;
}

FVestigeSaveRecord UFarmingComponent::WriteSaveRecord() const
{
	return FVestigeSaveRecord::Write(GetSaveData());
}

bool UFarmingComponent::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FFarmFarmingSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	if (!RestoreFromSaveData(Data))
	{
		++OutLeftOutCount;
	}
	return true;
}

void UFarmingComponent::StartNewGame()
{
	SelectToolbarSlot(0);
}

void UFarmingComponent::UseTool()
{
	if (!GetCurrentTool())
	{
		Notify(LOCTEXT("NoTool", "No tool in this slot."));
		return;
	}

	// Granted here too, since the owner may have been possessed after the tool came into the hand.
	UpdateGrantedTool();
	if (UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get(); AbilitySystem && GrantedSpec.IsValid())
	{
		AbilitySystem->TryActivateAbility(GrantedSpec);
	}
}

void UFarmingComponent::StartUsingTool()
{
	bToolUseHeld = true;
	OnToolUseHeldChanged.Broadcast(bToolUseHeld);
	if (!GetCurrentTool())
	{
		Notify(LOCTEXT("NoTool", "No tool in this slot."));
		return;
	}

	UpdateGrantedTool();
	UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get();
	FGameplayAbilitySpec* Spec = FindGrantedSpec();
	if (!AbilitySystem || !Spec)
	{
		return;
	}

	// Pressed before activating, so the ability sees the key held from its first use. If a use is still finishing, the
	// press reaches it instead, and the repeat goes on.
	AbilitySystem->AbilitySpecInputPressed(*Spec);
	AbilitySystem->TryActivateAbility(GrantedSpec);
}

void UFarmingComponent::StopUsingTool()
{
	bToolUseHeld = false;
	OnToolUseHeldChanged.Broadcast(bToolUseHeld);
	if (FGameplayAbilitySpec* Spec = FindGrantedSpec())
	{
		GrantedAbilitySystem->AbilitySpecInputReleased(*Spec);
	}
}

void UFarmingComponent::ReportToolUsed(const UVestigeToolData& Tool, const FIntPoint& Tile)
{
	OnToolUsed.Broadcast(&Tool, Tile);
}

void UFarmingComponent::ReportCropHarvested(const UFarmCropData& Crop, const FIntPoint& Tile)
{
	OnCropHarvested.Broadcast(&Crop, Tile);
}

void UFarmingComponent::UpdateGrantedTool()
{
	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	const UVestigeToolData* Tool = GetCurrentTool();
	if (AbilitySystem == GrantedAbilitySystem.Get() && Tool == GrantedTool.Get())
	{
		return;
	}

	ReleaseGrantedTool();
	if (!AbilitySystem || !Tool || !Tool->GetAbility())
	{
		return;
	}

	// The spec's source object is not const, but the ability only reads the tool.
	UVestigeToolData* SourceTool = const_cast<UVestigeToolData*>(Tool);
	constexpr int32 Level = 1;
	GrantedSpec = AbilitySystem->GiveAbility(FGameplayAbilitySpec(Tool->GetAbility(), Level, INDEX_NONE, SourceTool));
	GrantedAbilitySystem = AbilitySystem;
	GrantedTool = Tool;
}

void UFarmingComponent::ReleaseGrantedTool()
{
	UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get();
	if (AbilitySystem && GrantedSpec.IsValid())
	{
		// Released first, so an ability waiting for a tile ends now and one in a use ends when the use's time has passed.
		if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(GrantedSpec); Spec && Spec->IsActive())
		{
			AbilitySystem->AbilitySpecInputReleased(*Spec);
		}

		// Removes the ability now if it isn't active, or when it ends.
		AbilitySystem->SetRemoveAbilityOnEnd(GrantedSpec);
	}
	GrantedSpec = FGameplayAbilitySpecHandle();
	GrantedAbilitySystem.Reset();
	GrantedTool.Reset();
}

UAbilitySystemComponent* UFarmingComponent::FindAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

FGameplayAbilitySpec* UFarmingComponent::FindGrantedSpec() const
{
	UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get();
	return AbilitySystem && GrantedSpec.IsValid() ? AbilitySystem->FindAbilitySpecFromHandle(GrantedSpec) : nullptr;
}

void UFarmingComponent::FollowController(APawn* Pawn, AController* OldController, AController* NewController)
{
	UpdateGrantedTool();
}

void UFarmingComponent::AimAt(const FVector& WorldDirection)
{
	if (!Grid)
	{
		return;
	}

	// Called every frame while a movement input is held, so the usual case stops here: no input, or the same direction.
	const FIntPoint Step = FarmTileTargeting::SnapToStep(Grid->ToGridDirection(WorldDirection));
	if (Step == FIntPoint::ZeroValue || Step == AimStep)
	{
		return;
	}
	AimStep = Step;
	UpdateTargetTile();
}

FVector UFarmingComponent::GetAimDirection() const
{
	return Grid ? Grid->ToWorldDirection(AimStep) : FVector::ZeroVector;
}

TOptional<FIntPoint> UFarmingComponent::FindTargetTile() const
{
	const AActor* Owner = GetOwner();
	if (!Grid || !Owner)
	{
		return {};
	}
	const FIntPoint Neighbour = FarmTileTargeting::GetNeighbour(Grid->GetCoordinatesAt(Owner->GetActorLocation()), AimStep);
	return Grid->GetTile(Neighbour) ? TOptional<FIntPoint>(Neighbour) : TOptional<FIntPoint>();
}

void UFarmingComponent::UpdateTargetTile()
{
	const TOptional<FIntPoint> NewTarget = FindTargetTile();
	if (NewTarget == TargetTile)
	{
		return;
	}
	TargetTile = NewTarget;

	// A tool ability waiting with the key held listens, so walking along a row works each tile it reaches.
	OnTargetTileChanged.Broadcast(NewTarget);
}

void UFarmingComponent::FollowOwnerTransform(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	// Fires on every move and turn; the target only changes when the owner reaches another tile.
	const AActor* Owner = GetOwner();
	const FIntPoint Coordinates = Grid && Owner ? Grid->GetCoordinatesAt(Owner->GetActorLocation()) : OwnerCoordinates;
	if (Coordinates == OwnerCoordinates)
	{
		return;
	}
	OwnerCoordinates = Coordinates;
	UpdateTargetTile();
}

void UFarmingComponent::Notify(const FText& Message) const
{
	UVestigeFeedbackSubsystem::Notify(GetWorld(), Message);
}

#undef LOCTEXT_NAMESPACE
