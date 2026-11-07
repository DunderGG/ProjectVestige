// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmingComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "Engine/AssetManager.h"
#include "Farming/FarmLog.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayAbilitySpec.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UFarmingComponent::UFarmingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFarmingComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadToolsAndCrops();
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UFarmingComponent::FollowController);
	}

	// The pawn a map starts with is possessed before the world begins play, so its controller change has already happened.
	FollowInventory();
}

void UFarmingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// By now the owner is unpossessed and reaches no ability system, so the grant goes through the one it was made on.
	ReleaseGrantedTool();

	if (UVestigeInventoryComponent* OldInventory = Inventory.Get())
	{
		OldInventory->OnItemInHandChanged.RemoveAll(this);
	}
	Inventory.Reset();
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UFarmingComponent::FollowController);
	}
	Super::EndPlay(EndPlayReason);
}

void UFarmingComponent::LoadToolsAndCrops()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmingComponent::LoadToolsAndCrops);

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		return;
	}

	TArray<FPrimaryAssetId> ToolIds;
	AssetManager->GetPrimaryAssetIdList(UVestigeToolData::AssetType, ToolIds);
	TArray<FPrimaryAssetId> CropIds;
	AssetManager->GetPrimaryAssetIdList(UFarmCropData::AssetType, CropIds);

	// Waiting is fine at play start, and with the meshes loaded now, holding or planting never stalls on a load. The asset
	// manager keeps the handle, so a later pawn finds them in memory.
	TArray<FPrimaryAssetId> AssetIds = ToolIds;
	AssetIds.Append(CropIds);
	if (const TSharedPtr<FStreamableHandle> Handle = AssetManager->LoadPrimaryAssets(AssetIds, {UVestigeContentData::VisualsBundle}))
	{
		Handle->WaitUntilComplete();
	}

	for (const FPrimaryAssetId& ToolId : ToolIds)
	{
		if (const UVestigeToolData* Tool = AssetManager->GetPrimaryAssetObject<UVestigeToolData>(ToolId))
		{
			if (!Tool->GetAbility())
			{
				VESTIGE_LOG(this, LogFarm, Warning, "The tool {ToolId} names no Ability, so it does nothing.", ToolId.ToString());
			}
			Tools.Add(ToolId, Tool);
		}
	}
	for (const FPrimaryAssetId& CropId : CropIds)
	{
		if (const UFarmCropData* Crop = AssetManager->GetPrimaryAssetObject<UFarmCropData>(CropId))
		{
			Crops.Add(CropId, Crop);
		}
	}
	if (!Tools.Contains(HandToolId))
	{
		VESTIGE_LOG(
			this, LogFarm, Warning, "The Hand Tool Id ({ToolId}) names no tool, so an empty hand does nothing.", HandToolId.ToString());
	}
}

const UVestigeToolData* UFarmingComponent::GetCurrentTool() const
{
	const UVestigeInventoryComponent* CurrentInventory = FindInventory();
	if (!CurrentInventory)
	{
		return nullptr;
	}

	const UVestigeItemData* Item = CurrentInventory->GetItemInHand();
	const UVestigeToolFragment* ToolFragment = Item ? Item->FindFragment<UVestigeToolFragment>() : nullptr;
	if (ToolFragment)
	{
		if (const TObjectPtr<const UVestigeToolData>* Tool = Tools.Find(ToolFragment->GetToolId()))
		{
			return *Tool;
		}
	}
	const TObjectPtr<const UVestigeToolData>* HandTool = Tools.Find(HandToolId);
	return HandTool ? HandTool->Get() : nullptr;
}

const UFarmCropData* UFarmingComponent::FindCrop(const FPrimaryAssetId& CropId) const
{
	const TObjectPtr<const UFarmCropData>* Crop = Crops.Find(CropId);
	return Crop ? Crop->Get() : nullptr;
}

UVestigeInventoryComponent* UFarmingComponent::FindInventory() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
}

void UFarmingComponent::FollowInventory()
{
	UVestigeInventoryComponent* NewInventory = FindInventory();
	if (NewInventory != Inventory.Get())
	{
		if (UVestigeInventoryComponent* OldInventory = Inventory.Get())
		{
			OldInventory->OnItemInHandChanged.RemoveAll(this);
		}
		Inventory = NewInventory;
		if (NewInventory)
		{
			NewInventory->OnItemInHandChanged.AddUObject(this, &UFarmingComponent::HandleItemInHandChanged);
		}
	}
	HandleItemInHandChanged();
}

void UFarmingComponent::HandleItemInHandChanged()
{
	const UVestigeToolData* Tool = GetCurrentTool();
	if (Tool != BroadcastTool.Get())
	{
		// A tool item whose tool isn't loaded falls back to the hand; said once, when it comes into hand.
		const UVestigeInventoryComponent* CurrentInventory = Inventory.Get();
		const UVestigeItemData* Item = CurrentInventory ? CurrentInventory->GetItemInHand() : nullptr;
		const UVestigeToolFragment* ToolFragment = Item ? Item->FindFragment<UVestigeToolFragment>() : nullptr;
		if (ToolFragment && !Tools.Contains(ToolFragment->GetToolId()))
		{
			VESTIGE_LOG(this, LogFarm, Warning, "The item {ItemId} names the tool {ToolId}, which doesn't exist, so the hand is used.",
				Item->GetContentId(), ToolFragment->GetToolId().ToString());
		}
	}

	UpdateGrantedTool();
	if (Tool != BroadcastTool.Get())
	{
		// Remembered first, so a listener that changes the hand again is compared against the latest tool.
		BroadcastTool = Tool;
		OnToolChanged.Broadcast();
	}
}

void UFarmingComponent::UseTool()
{
	FollowInventory();
	if (UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get(); AbilitySystem && GrantedSpec.IsValid())
	{
		AbilitySystem->TryActivateAbility(GrantedSpec);
	}
}

void UFarmingComponent::StartUsingTool()
{
	bToolUseHeld = true;
	OnToolUseHeldChanged.Broadcast(bToolUseHeld);

	// Followed here too, since the owner may have been given a player state without a controller.
	FollowInventory();
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

	// The spec's source object is not const, but the ability only reads the tool. While the ability list is locked (inside
	// a tool use's first try) the grant is queued and added when the lock is released.
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
		// The default lookup doesn't find a queued grant; never ask with EConsiderPending::PendingAdd, whose loop over the
		// queued specs doesn't compare handles in 5.8.
		FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(GrantedSpec);
		if (Spec && Spec->IsActive())
		{
			// Released first, so an ability waiting for a tile ends now and one in a use ends when the use's time has passed.
			AbilitySystem->AbilitySpecInputReleased(*Spec);
			AbilitySystem->SetRemoveAbilityOnEnd(GrantedSpec);
		}
		else
		{
			// Removes a queued grant from the pending adds, an inactive one at once (or when the list is unlocked), and does
			// nothing for a handle the ability system no longer knows.
			AbilitySystem->ClearAbility(GrantedSpec);
		}
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
	FollowInventory();
}

#if WITH_DEV_AUTOMATION_TESTS
void UFarmingComponent::AddCropForTests(const UFarmCropData& Crop)
{
	Crops.Add(Crop.GetPrimaryAssetId(), &Crop);
}
#endif
