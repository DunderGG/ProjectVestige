// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveable.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VestigeMilestoneSubsystem.generated.h"

class UVestigeSaveRegistry;

/**
 * The game's one-time milestones, such as the farm's firsts: each is a gameplay tag (Vestige.Milestone.Farm.Harvested), reached
 * once per game. It knows no game rule: the owner of each event reaches its milestone, and whoever cares (Hob's trust, later Hob's
 * event lines) reads it, so the farm never knows who counts it. A game-instance subsystem, so a milestone reached with no player
 * state, or before a character is met, still counts. Saves under Base.Milestones through UVestigeSaveRegistry. Systems.md, section 11.
 */
UCLASS()
class UVestigeMilestoneSubsystem : public UGameInstanceSubsystem, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** The milestones of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UVestigeMilestoneSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Reaches Milestone. Returns whether it was new; an invalid tag or one already reached changes nothing. Logs the first time. */
	bool Reach(const FGameplayTag& Milestone);

	/** Whether Milestone has been reached. */
	bool HasReached(const FGameplayTag& Milestone) const;

	/** Every milestone reached, for a dialogue context's copy. */
	const FGameplayTagContainer& GetReached() const { return Reached; }

	/** Forgets every milestone, as a new game starts. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

private:
	FGameplayTagContainer Reached;

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> SaveRegistry;
};
