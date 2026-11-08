// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmingComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "Core/VestigeFeedbackSubsystem.h"
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
	// By now the owner is unpossessed and reaches no ability system, so the grants go through the one they were made on.
	ReleaseGrant(GrantedSpec, GrantedAbilitySystem);
	GrantedTool.Reset();
	ReleaseGrant(HarvestSpec, HarvestAbilitySystem);

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
	if (!Tools.Contains(HarvestToolId))
	{
		VESTIGE_LOG(
			this, LogFarm, Warning, "The Harvest Tool Id ({ToolId}) names no tool, so nothing can be harvested.", HarvestToolId.ToString());
	}
}

const UVestigeToolData* UFarmingComponent::GetCurrentTool() const
{
	const UVestigeInventoryComponent* CurrentInventory = FindInventory();
	const UVestigeItemData* Item = CurrentInventory ? CurrentInventory->GetItemInHand() : nullptr;
	const UVestigeToolFragment* ToolFragment = Item ? Item->FindFragment<UVestigeToolFragment>() : nullptr;
	const TObjectPtr<const UVestigeToolData>* Tool = ToolFragment ? Tools.Find(ToolFragment->GetToolId()) : nullptr;
	return Tool ? Tool->Get() : nullptr;
}

const UVestigeToolData* UFarmingComponent::GetHarvestTool() const
{
	const TObjectPtr<const UVestigeToolData>* Tool = Tools.Find(HarvestToolId);
	return Tool ? Tool->Get() : nullptr;
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
	// Followed here too, since the owner may have been given a player state without a controller.
	FollowInventory();
	if (!GetCurrentTool())
	{
		ReportNothingInHand();
		return;
	}

	bToolUseHeld = true;
	OnToolUseHeldChanged.Broadcast(bToolUseHeld);
	UAbilitySystemComponent* AbilitySystem = GrantedAbilitySystem.Get();
	FGameplayAbilitySpec* Spec = FindGrantedSpec();
	if (!AbilitySystem || !Spec)
	{
		// A grant still queued under the ability list's lock has no spec yet; the press does nothing then.
		return;
	}

	// Pressed before activating, so the ability sees the key held from its first use. If a use is still finishing, the
	// press reaches it instead, and the repeat goes on.
	AbilitySystem->AbilitySpecInputPressed(*Spec);
	AbilitySystem->TryActivateAbility(GrantedSpec);
}

void UFarmingComponent::StopUsingTool()
{
	// Only a press that held the key pressed the spec, so without one there is nothing to release. The pawn also calls this
	// when it is unpossessed, held or not.
	if (!bToolUseHeld)
	{
		return;
	}
	bToolUseHeld = false;
	OnToolUseHeldChanged.Broadcast(bToolUseHeld);
	if (FGameplayAbilitySpec* Spec = FindGrantedSpec())
	{
		GrantedAbilitySystem->AbilitySpecInputReleased(*Spec);
	}
}

void UFarmingComponent::Harvest()
{
	// Also keeps the harvest grant current, for an owner given a player state without a controller.
	FollowInventory();

	// Never pressed, so the harvest ability does one use and ends once its time has passed.
	if (UAbilitySystemComponent* AbilitySystem = HarvestAbilitySystem.Get(); AbilitySystem && HarvestSpec.IsValid())
	{
		AbilitySystem->TryActivateAbility(HarvestSpec);
	}
}

void UFarmingComponent::ReportNothingInHand() const
{
	const UVestigeInventoryComponent* CurrentInventory = FindInventory();
	if (!CurrentInventory)
	{
		return;
	}

	// An item that names a tool which isn't loaded is no tool; said on the press, since the hand's event can't tell it from an
	// empty slot.
	const UVestigeItemData* Item = CurrentInventory->GetItemInHand();
	const UVestigeToolFragment* ToolFragment = Item ? Item->FindFragment<UVestigeToolFragment>() : nullptr;
	if (ToolFragment)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "The item {ItemId} names the tool {ToolId}, which doesn't exist, so it can't be used.",
			Item->GetContentId(), ToolFragment->GetToolId().ToString());
	}
	UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Tool_NothingInHand);
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

	// The harvest is granted all the time, apart from the tool in hand, so selecting slots never touches it.
	if (AbilitySystem != HarvestAbilitySystem.Get() || !HarvestSpec.IsValid())
	{
		ReleaseGrant(HarvestSpec, HarvestAbilitySystem);
		HarvestSpec = Grant(AbilitySystem, GetHarvestTool());
		if (HarvestSpec.IsValid())
		{
			HarvestAbilitySystem = AbilitySystem;
		}
	}

	const UVestigeToolData* Tool = GetCurrentTool();
	if (AbilitySystem == GrantedAbilitySystem.Get() && Tool == GrantedTool.Get())
	{
		return;
	}
	ReleaseGrant(GrantedSpec, GrantedAbilitySystem);
	GrantedTool.Reset();
	GrantedSpec = Grant(AbilitySystem, Tool);
	if (GrantedSpec.IsValid())
	{
		GrantedAbilitySystem = AbilitySystem;
		GrantedTool = Tool;
	}
}

FGameplayAbilitySpecHandle UFarmingComponent::Grant(UAbilitySystemComponent* AbilitySystem, const UVestigeToolData* Tool)
{
	if (!AbilitySystem || !Tool || !Tool->GetAbility())
	{
		return FGameplayAbilitySpecHandle();
	}

	// The spec's source object is not const, but the ability only reads the tool. While the ability list is locked (inside
	// a tool use's first try) the grant is queued and added when the lock is released.
	UVestigeToolData* SourceTool = const_cast<UVestigeToolData*>(Tool);
	constexpr int32 Level = 1;
	return AbilitySystem->GiveAbility(FGameplayAbilitySpec(Tool->GetAbility(), Level, INDEX_NONE, SourceTool));
}

void UFarmingComponent::ReleaseGrant(FGameplayAbilitySpecHandle& Spec, TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystem)
{
	UAbilitySystemComponent* Granter = AbilitySystem.Get();
	if (Granter && Spec.IsValid())
	{
		// The default lookup doesn't find a queued grant; never ask with EConsiderPending::PendingAdd, whose loop over the
		// queued specs doesn't compare handles in 5.8.
		FGameplayAbilitySpec* Found = Granter->FindAbilitySpecFromHandle(Spec);
		if (Found && Found->IsActive())
		{
			// Released first, so an ability waiting for a tile ends now and one in a use ends when the use's time has passed.
			Granter->AbilitySpecInputReleased(*Found);
			Granter->SetRemoveAbilityOnEnd(Spec);
		}
		else
		{
			// Removes a queued grant from the pending adds, an inactive one at once (or when the list is unlocked), and does
			// nothing for a handle the ability system no longer knows.
			Granter->ClearAbility(Spec);
		}
	}
	Spec = FGameplayAbilitySpecHandle();
	AbilitySystem.Reset();
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

void UFarmingComponent::AddToolForTests(const UVestigeToolData& Tool)
{
	Tools.Add(Tool.GetPrimaryAssetId(), &Tool);
}
#endif
