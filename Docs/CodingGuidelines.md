# Project Vestige coding guidelines

How code in this project should look. How the systems fit together, who owns what and why, is in [Architecture.md](Architecture.md). How each game system (saving, input, UI, abilities, AI...) is built is in [Systems.md](Systems.md).

This document builds on three sources:

1. **Epic's C++ Coding Standard** (https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine). It is the baseline. Where this document says nothing, follow Epic.
2. **The Lyra Starter Game** sample. We borrow its structure and patterns but not its full machinery. See [Architecture.md](Architecture.md#11-patterns-borrowed-from-lyra).
3. **Clean code**: small focused units, intention-revealing names, no duplication, no dead code.

Other official references are listed under [Further reading](#18-further-reading).

The top priority is that the project stays **easy to read and easy to change** for one developer coming back to it after weeks away. When a rule here gets in the way of that, raise it and change the rule rather than quietly ignoring it.

Contents

1. [Ownership and separation of concerns](#1-ownership-and-separation-of-concerns)
2. [Naming](#2-naming)
3. [Formatting](#3-formatting)
4. [Architecture](#4-architecture)
5. [Classes and headers](#5-classes-and-headers)
6. [UPROPERTY, UFUNCTION and Blueprint exposure](#6-uproperty-ufunction-and-blueprint-exposure)
7. [Memory, pointers and object lifetime](#7-memory-pointers-and-object-lifetime)
8. [Functions and control flow](#8-functions-and-control-flow)
9. [Modern C++ usage](#9-modern-c-usage)
10. [Errors, logging and assertions](#10-errors-logging-and-assertions)
11. [Comments and documentation](#11-comments-and-documentation)
12. [Performance](#12-performance)
13. [Testing](#13-testing)
14. [Project layout and assets](#14-project-layout-and-assets)
15. [Git workflow](#15-git-workflow)
16. [Known deviations to clean up](#16-known-deviations-to-clean-up)
17. [Review checklist](#17-review-checklist)
18. [Further reading](#18-further-reading)

---

## 1. Ownership and separation of concerns

This is the most important rule in the project.

**Every actor, component and subsystem has one job. Logic, components and settings live on the class whose job they are, never on whichever class happens to be convenient.**

Before adding a component, property, timer or `Tick` logic to an existing class, ask: *is this that class's job?*

- If yes, add it there.
- If no, put it on the class that does own it, or create a new actor, component or subsystem for it. Say so explicitly in the commit message and add it to the ownership table.

A real example from this project: night post-processing and sun control were first put on `AFarmGrid` because the grid was already in the level. They were moved to a dedicated day cycle actor, and later the clock moved on into its own subsystem, leaving the actor only the lighting (`AVestigeSkyLighting`). That is the pattern to follow.

The current ownership table is in [Architecture.md, section 13](Architecture.md#13-systems-and-ownership). How to choose the right kind of owner (actor, component, subsystem, data asset) is in [section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go) and how owners communicate is in [section 4](Architecture.md#4-how-systems-talk-to-each-other).

## 2. Naming

Follow Epic's naming rules. The `.editorconfig` in the repo enforces the prefixes in Visual Studio and Rider.

### 2.1 Type prefixes

| Prefix | For | Example |
|---|---|---|
| `A` | Classes deriving from `AActor` | `AFarmGrid` |
| `U` | Classes deriving from `UObject` (components, subsystems, data assets) | `UFarmingComponent` |
| `S` | Slate widgets | `SVestigeHudWidget` |
| `F` | Structs and most other classes | `FFarmTileSaveData` |
| `E` | Enums | `EFarmTileState` |
| `I` | Interfaces | `IVestigeInteractable` |
| `T` | Templates | `TVestigePool` |
| `b` | Booleans (variables, members, parameters) | `bWatered`, `bHasCrop` |

### 2.2 The `Vestige` and `Farm` project prefixes

Like Lyra prefixes its classes with `Lyra` (`ALyraCharacter`, `ULyraHealthComponent`), every class, struct, enum, delegate and log category we add starts with a project prefix after the type prefix. This keeps our types apart from engine and plugin types in autocomplete and in the editor's class pickers. A bare name often isn't even possible: `APlayerState` is the engine's own class.

- **`Vestige`** is the project prefix: `AVestigePlayerState`, `UVestigeInventoryComponent`, `FOnVestigeInventoryChanged`. It matches the module, `LogVestige` and the template classes.
- **`Farm`** is for types about the farm itself: its tiles, grid, crops and the farming actions (`AFarmGrid`, `UFarmField`, `UFarmCropData`, `UFarmingComponent`, `EFarmTileState`). The game is more than farming, so player, inventory, time, saving, UI and other general types never use it.
- Types made before 2026-10-31 used `Farm` for everything. The general ones were renamed to `Vestige` on 2026-11-01 (`AFarmHUD` became `AVestigeHUD`, `AFarmerCharacter` became `AVestigePlayerCharacter`), with `[CoreRedirects]` in `Config/DefaultEngine.ini` so assets and saves that name the old types keep loading.
- **Renaming a reflected type** that an asset, a level, a config file or a save names needs a redirect in `[CoreRedirects]` (`ClassRedirects`, `StructRedirects`, `EnumRedirects`) and a test that loads the old name. Redirects work for asset references and saved struct and enum names, but not for a save file's own class: loading a save finds its class by the name in the file's header without applying redirects, so once saves must stay compatible (from the first public build), a renamed save game class keeps its old name as an empty subclass.

The template's class, `AVestigeCharacter`, keeps the template's naming: the module name plus the class role. It lives in `Template/`; our own `Vestige` types live in their layer's folder. The game mode and player controller kept their template names when we rewrote them in `Game/` (2026-11-06), so the assets and config that name them needed no redirect.

**Epic template code isn't ours to maintain.** The template files (`VestigeCharacter`) keep Epic's style and copyright header, and this guide doesn't apply to them: don't reformat or tidy them in passing, and don't copy their style (raw `UObject*` properties, their own log category) into new code. Delete a template file once nothing uses it. If we ever need to change one substantially, first bring it up to this guide in a commit of its own.

### 2.3 Case and wording

- **PascalCase for everything**: types, functions, members, locals, parameters, constants. No `snake_case`, no `m_` or `_` prefixes, no Hungarian notation beyond the prefixes above.
- Names are **descriptive and unabbreviated**: `ReachDistance`, not `RchDst`; `HarvestedCrops`, not `NumHarv`. Common, well-known short forms are fine (`HUD`, `UI`, `Max`, `Min`, `Num`).
- Functions that do something are **verbs**: `Till()`, `StartNextDay()`, `RefreshVisuals()`.
- Functions that only **query state** and return a `bool` ask a **question**: `IsRipe()`, `AreTilesReady()`, `HasMessages()`.
- Functions that **do something that can fail** keep their verb name and return `bool` for success: `FFarmTileState::Till()` returns `false` when the tile isn't grass. Say in the `/** */` comment what `false` means. Don't rename them as questions (`DidTillSucceed()`) or add a `Try` prefix.
- Getters use `Get`: `GetCurrentDay()`. `Find` returns `nullptr` when nothing exists; `Get` may create it, or returns a subsystem that always exists in a game (`AFarmGrid::Find`, `UVestigeClockSubsystem::Get`).
- Delegates: the type is `FOn<Prefix><Event>` with the type's project prefix, the member is `On<Event>`: `FOnVestigeNewDay OnNewDay`. A Blueprint twin of a native delegate ([Architecture.md, section 4](Architecture.md#4-how-systems-talk-to-each-other)) adds `Blueprint` to both: `FOnVestigeNewDayBlueprint OnNewDayBlueprint`, shown to Blueprints with `DisplayName = "On New Day"`.
- Event handlers are named for what they do, not when they run: `AdvanceTiles(int32 NewDay)`, `SaveFarm(int32 NewDay)`, not `HandleOnNewDay`.
- Units go in the name or the comment when not obvious: `RealSecondsPerGameHour`, `TileSize` with `/** ...in cm */`.
- Out parameters start with `Out`: `void GetBounds(FVector& OutOrigin, FVector& OutExtent)`.
- A parameter that would clash with a member gets the `In` prefix (`InWorld`), or a different name. Never reuse a name from a base class.

### 2.4 Never shadow

The build treats MSVC warning **C4458 (declaration hides class member)** as an error. A local or parameter must never have the same name as a member of the class or any base class. Watch out especially for names inherited from engine classes:

```cpp
// Wrong: ACharacter already has a member called Mesh.
UStaticMesh* Mesh = Tool->GetHeldMesh();

// Right
UStaticMesh* ToolMesh = Tool->GetHeldMesh();
```

Other common traps in actors and pawns: `Controller`, `Owner`, `Tags`, `Instigator`, `RootComponent`, `Children`. In Slate widgets: `ChildSlot`.

## 3. Formatting

The repo has a **`.clang-format`** file that encodes these rules, so formatting is automatic rather than something to review.

- In Visual Studio, check *Tools > Options > Text Editor > C/C++ > Code Style > Formatting > Enable ClangFormat support*. VS then uses the repo's file. Format a document with *Ctrl+K, Ctrl+D*.
- **The reference formatter is the clang-format that ships with Visual Studio** (version 22.1 in Visual Studio 2026, at `VC\Tools\Llvm\x64\bin\clang-format.exe`). Other versions can differ in small details; when they disagree, this one wins.
- All of our files (everything outside `Template/`) are clean under it. Format the files you change, so a diff only shows your own lines. The Epic template files keep Epic's formatting ([section 2.2](#22-the-vestige-and-farm-project-prefixes)).
- clang-format can't lay out Slate's declarative syntax. Wrap `SLATE_BEGIN_ARGS` ... `SLATE_END_ARGS()` and each `ChildSlot [ ... ]` layout in `// clang-format off` and `// clang-format on`.
- The rules below still apply where the formatter has no opinion (blank lines between groups, where to break long expressions).

- **Tabs for indentation**, 4 spaces wide. No trailing whitespace.
- **Braces on their own line** (Allman style), for every block:

```cpp
bool FFarmTileState::Till()
{
	if (State != EFarmTileState::Grass)
	{
		return false;
	}
	State = EFarmTileState::Tilled;
	return true;
}
```

- **Always use braces**, even for one-line `if`, `for` and `while` bodies.
- One-line function bodies are fine in headers for trivial getters: `int32 GetCurrentDay() const { return CurrentDay; }`.
- `else if` stays on one line. Don't write `else` after a branch that returns.
- A space after `if`, `for`, `while`, `switch` and around binary operators. No space between a function name and its `(`.
- Pointer and reference symbols attach to the type: `AFarmGrid* Grid`, `const FVector& WorldLocation`.
- Keep lines under 140 characters. Break long argument lists after a comma and indent one extra tab.
- When a ternary breaks over lines, the `?` and `:` start the new lines (the formatter does this). Don't nest ternaries: write an `if` with early returns instead, as `FarmTileLooks::GetLook` does for the soil's color.
- Function bodies in `.cpp` files always go on their own lines, even one-liners.
- One blank line between functions. Group related statements inside a function and separate groups with a blank line.

## 4. Architecture

Architecture lives in its own document, [Architecture.md](Architecture.md): design principles, where things go in Unreal's gameplay framework, lifetimes and initialization order, communication between systems, data, saving, presentation, the C++/Blueprint split, the Lyra patterns we use, Project Vestige's current design and the decision log.

Code that changes the design must follow it, and update its Part B and decision log in the same commit.

## 5. Classes and headers

### 5.1 One class per file pair

Each class lives in `ClassName.h` and `ClassName.cpp`, named without the type prefix: `AFarmGrid` in `FarmGrid.h`. Small helper types tied to one class (an enum, a delegate, a save struct) can share its header.

### 5.2 Header layout

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VestigeSkyLighting.generated.h"   // Always the last include

class ADirectionalLight;              // Forward declarations instead of includes
class UPostProcessComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDay, int32 /* Day */);

/** What this class is for, in one or two sentences. */
UCLASS()
class AVestigeSkyLighting : public AActor
{
	GENERATED_BODY()

public:
	// Constructor, then public functions, then public delegates and properties.

protected:
	// Engine overrides (BeginPlay, EndPlay, Tick) and anything subclasses need.

private:
	// Helpers, owned components, runtime state.
};
```

- Order sections `public`, `protected`, `private`.
- Inside each section: constructor, static functions, functions, delegates, properties.
- Mark every override with `override`. Mark classes and functions `final` when they must not be extended further.
- Keep engine overrides `protected` unless callers outside the class need them.

### 5.3 Includes

- **Include what you use** (IWYU). Each file includes the headers for what it uses directly. Do not rely on another header to bring something in.
- In headers, **forward declare** pointer and reference types instead of including their headers. Include headers in the `.cpp`.
- In a `.cpp`, include its own header first, then a blank line, then the rest in **alphabetical order** of their paths. Our headers are included by their folder path ([section 14.1](#141-source)):

```cpp
#include "UI/VestigeHUD.h"

#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Time/VestigeClockSubsystem.h"
#include "UI/VestigeHudWidget.h"
```

- Never include `Engine.h` or `EngineMinimal.h`.
- New engine module dependencies go in `Vestige.Build.cs`. Put them in `PrivateDependencyModuleNames` unless our public headers expose that module's types.

### 5.4 Const correctness

- Getters and anything that doesn't change the object are `const`.
- Pass non-trivial types (`FString`, `FVector`, `TArray`) by `const&`. Pass small types (`int32`, `float`, `bool`, enums, pointers) by value.
- Use `const` for locals that never change after initialization.
- Return `const` references to member containers rather than copies: `const TArray<FFarmTileState>& GetTiles() const`.
- Do not return `const` values by value (`const FString GetName()` blocks moves).

### 5.5 Encapsulation

- **Runtime state is private.** Expose it through getters, and change it only through functions that keep the object consistent (`UFarmField::Till` changes the tile *and* broadcasts `OnTileChanged`, so its view redraws).
- **Tunables** that designers edit are `UPROPERTY(EditAnywhere / EditDefaultsOnly)`. Prefer making them `protected` or `private` with `meta = (AllowPrivateAccess = "true")`, as Lyra does, and add a getter when other classes need to read them.
- Public mutable data members are a smell. If another class needs to set a value, add a function with a name that says why (`RestoreFromSaveData`, `SelectHotbarSlot`).

## 6. UPROPERTY, UFUNCTION and Blueprint exposure

- A **strong** reference from a `UObject` to another `UObject` is a `UPROPERTY()` holding a `TObjectPtr<>`, so the garbage collector sees it and keeps the target alive. This applies even to private, non-edited members:

```cpp
UPROPERTY()
TObjectPtr<AFarmGrid> Grid;
```

- Not every reference should be strong. Weak and soft references have different meanings and stay weak or soft; see [section 7](#7-memory-pointers-and-object-lifetime) for which to pick. Don't turn one into a `TObjectPtr` just to satisfy the rule above.

- Pick the narrowest specifier:

| Specifier | Use for |
|---|---|
| `EditDefaultsOnly` | Settings that are the same for every instance, set in the class or Blueprint defaults (meshes, materials) |
| `EditAnywhere` | Settings that differ per placed instance (grid size) |
| `VisibleAnywhere` | Components created in the constructor |
| `VisibleInstanceOnly` | Runtime state you want to inspect while debugging in the editor |
| no edit specifier | Internal references the editor shouldn't show |

- Always give edited properties a `Category` named for what the settings are about, in plain words without a project prefix: `"Time"`, `"Lighting"`, `"Rest"`, `"Inventory"`, `"Visuals"`. The `Farm` types ([section 2.2](#22-the-vestige-and-farm-project-prefixes)) put theirs under `"Farming"`, with subcategories after `|`: `"Farming|Growth"`, `"Farming|Visuals"`. Where our setting belongs with the engine's, use the engine's category, as our input settings join the template's in `"Input"`. The same names place Blueprint functions and events in the Blueprint menus (Get Day Cycle under Time), and a component's `ClassGroup`, which groups it in the Add Component list, follows the same rule.
- Add `ClampMin` / `ClampMax` meta where a value has a sensible range.
- Expose to Blueprints (`BlueprintReadOnly`, `BlueprintCallable`, `BlueprintPure`) **only when a Blueprint actually needs it, or as a deliberate modding extension point** marked `/** Extension point: ... */` (see [Architecture.md, Designing for mods](Architecture.md#12-designing-for-mods)). Everything exposed becomes an API we have to keep stable. Prefer `BlueprintReadOnly` to `BlueprintReadWrite`.
- Core rules live in C++. Blueprints are for content: picking meshes, tuning values, laying out levels, small visual-only behavior.
- Mark components created with `CreateDefaultSubobject` as `VisibleAnywhere`, and create them only in the constructor.

## 7. Memory, pointers and object lifetime

- Never use `new` / `delete` for `UObject`s. Create them with `NewObject`, `CreateDefaultSubobject` or `SpawnActor`.

Choose a member's pointer type by what the reference **means**, not by habit:

| The reference... | Use | Effect |
|---|---|---|
| should keep the target alive (components we create, assets we always need, actors this object depends on) | `UPROPERTY() TObjectPtr<T>` | Strong: the garbage collector keeps the target while we point at it |
| points at something we don't own and must not keep alive (another actor that may be destroyed, the HUD from a Slate widget) | `TWeakObjectPtr<T>` | Weak: becomes null when the target is destroyed. Check with `IsValid()` or `Get()` before use. Add `UPROPERTY()` when the editor or serialization needs to see it |
| names an asset that shouldn't load with the class (crop meshes, data assets for content not in use) | `UPROPERTY() TSoftObjectPtr<T>` / `TSoftClassPtr<T>` | Soft: stores the asset's path; load it when needed |

`SVestigeHudWidget` holding its HUD in a `TWeakObjectPtr` is an example of the second row.

- Non-`UObject` heap data: `TUniquePtr`, `TSharedPtr`, `TSharedRef` (Slate widgets use `TSharedPtr`).
- Raw pointers are fine for locals and parameters that don't outlive the call.
- **Check pointers before using them** whenever they can legitimately be null: actors found in the level, `GetOwner()`, `GetWorld()`, `GetController()`, cast results. Use `IsValid()` for actors that may have been destroyed.
- Use `Cast<T>()` for `UObject` casts. Never `dynamic_cast` or C-style casts on `UObject`s. Use `CastChecked<T>()` only where a failure is a programming error.

## 8. Functions and control flow

- **Small functions that do one thing.** If you need a comment to separate the parts of a function, those parts probably want to be functions (`RefreshVisuals` calls `RefreshSoil` and `RefreshCrop`).
- Aim for functions that fit on one screen. Length is a prompt, not a limit: split a function when it does more than one thing or is hard to follow, not because it crossed a line count. A long but single-purpose function is fine.
- **Return early** to keep the happy path unindented:

```cpp
void UVestigeSaveSubsystem::SaveFarm(int32 NewDay)
{
	const UFarmingComponent* Farming = GetPlayerFarming();
	if (!Farming || !bSavingAllowed)
	{
		return;
	}
	...
}
```

- Use `if (T* X = ...)` to scope a pointer to the block that uses it.
- Few parameters: three or fewer is the goal. Group more into a struct.
- No boolean flag parameters that switch behavior (`DoThing(true)`). Write two functions or use an enum.
- `switch` on enums lists every value and has no `default`, so the compiler warns when a value is added.
- **No magic numbers.** Name constants in a namespace at the top of the `.cpp` (`FarmTileColors`, `FarmTilePlaceholder`) or make them tunable properties.
- **Don't repeat yourself.** When the same few lines appear a second time, extract a function. When they appear in two classes, ask which class owns that logic.
- Prefer composition of small, obvious steps over clever one-liners.

## 9. Modern C++ usage

- **Default to UE types**: `int32`, `uint8`, `float`, `double`, `FString`, `FName`, `FText`, `TArray`, `TMap`, `TSet`, `TOptional`, `TFunction`. They are required for anything reflected (`UPROPERTY`, `UFUNCTION`), saved, passed to engine APIs or shown in the editor, and they're what readers of Unreal code expect. Don't use `int`/`long` where size matters.
- The standard library is allowed in private implementation code when it does something the UE types don't do as well (for example `<algorithm>` helpers or `std::array` inside a `.cpp`). Keep `std::` types out of headers, reflected members and save data.
- `FString` for runtime strings, `FName` for identifiers, `FText` for anything the player reads (so it can be localized later).
- Wrap string literals in `TEXT("...")`.
- `nullptr`, never `NULL` or `0`.
- `enum class` for every new enum. `UENUM(BlueprintType)` with `uint8` underlying type when it is used in properties.
- Range-based `for` over containers. Use an index loop only when you need the index.
- `auto`: only where the type is obvious from the right-hand side (`auto* Hud = Cast<AVestigeHUD>(...)`), for iterators and for lambdas. Spell out the type otherwise. Write `auto*` and `const auto&` so pointers and references stay visible.
- Lambdas: capture explicitly (`[this]`, `[Tile]`). Never capture `this` in a lambda that can outlive the object; bind delegates with `AddUObject` / `AddWeakLambda` instead.
- `constexpr` for compile-time constants. `static constexpr` for class constants (`UVestigeSaveGame::CurrentVersion`).
- `FMath` is the normal choice for math, especially on engine types (`FVector`, `FRotator`) and for helpers like `FMath::Clamp` and `FMath::Lerp`. Use `<cmath>` only for something `FMath` lacks.
- No exceptions and no RTTI; the engine builds without them.

## 10. Errors, logging and assertions

- Log through `VESTIGE_LOG` with the system's own category, declared in its layer's log header (`Farming/FarmLog.h`). It prints the sender and the function before the message. The categories, the format and when to use which verbosity are in [Systems.md, section 21](Systems.md#21-logging):

```cpp
VESTIGE_LOG(this, LogFarm, Warning, "No Tile Class is set, so the field is empty.");
```

- Verbosity in short: `Error` for broken state, `Warning` for recoverable setup problems, `Log` for notable one-off events, `Verbose` for tracing an action. Nothing is logged every frame.
- Never leave `GEngine->AddOnScreenDebugMessage` as a permanent feature.
- Assertions:
  - `check(Condition)`: a programmer error that makes continuing pointless. Compiled out in shipping builds, so never put side effects inside.
  - `ensure(Condition)` / `ensureMsgf`: something is wrong but the game can carry on. Logs a callstack once.
  - Plain `if` with a graceful fallback for things that can legitimately happen at runtime or through level setup (a missing grid, a missing save).
- Content mistakes a designer can make (a missing mesh, no grid in the level) must not crash. Fall back to a placeholder and warn, as the field's meshes component does with its placeholder shapes.
- Failures the player should know about (a save that couldn't be written) must reach the player, but game code never calls the HUD to do it. The code that failed logs the failure, returns or broadcasts it, and raises a presentation-neutral feedback event; the HUD listens and displays it (see [Architecture.md, section 7](Architecture.md#7-presentation-ui-audio-and-effects)). The event is `UVestigeFeedbackSubsystem::Notify(World, Text)`, with the message written as `LOCTEXT`.

## 11. Comments and documentation

- Every class gets a `/** */` comment saying **what it is for and what it owns**.
- Public functions and edited properties get a `/** */` comment when the name alone doesn't say everything: units, valid ranges, when it is called, what `nullptr` means.
- Inside functions, comment **why**, not what. Good: `// Static or stationary lights can't be turned at runtime.` Bad: `// Set the mobility to movable.`
- Write comments as full sentences in plain English.
- Don't leave commented-out code. Git remembers it.
- `TODO:` comments are allowed for known follow-ups but must say what is missing: `// TODO: Save each planted tile's crop ContentId once saves store content IDs.`
- New source files start with our copyright and license header (see [LICENSING.md](../LICENSING.md#license-notices-in-files)), then a blank line:

  ```cpp
  // Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
  // SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception
  ```

  They don't carry the Epic copyright line; the template's files keep theirs.

## 12. Performance

Performance is a requirement, not a polish step. Unreal Engine 5 makes it easy to ship a game that stutters, and stutter is far cheaper to prevent than to find later. The frame budget, the target hardware and how we measure are in [Architecture.md, section 10](Architecture.md#10-performance-by-design). This section is the code side.

There are two kinds of performance rule:

- **Always-on rules** (12.1 to 12.8). Cheap patterns that cost nothing in clarity. They apply to every change without a measurement, and breaking one fails review the same way a naming or ownership mistake does.
- **Optimizations that make code harder to read**: caching tricks, pooling, data-oriented rewrites, spreading work over frames, threads. These need a measurement that shows the problem, and the commit message says what was measured before and after. Clear code first; clever code only with numbers.

### 12.1 Ticking

- Ticking is **off by default**: `PrimaryActorTick.bCanEverTick = false` in actors, `PrimaryComponentTick.bCanEverTick = false` in components.
- A class that ticks says why in a comment next to `bCanEverTick = true` (`AVestigeSkyLighting` moves the sun). If it doesn't need every frame, it sets `TickInterval`, and it turns its tick off while it has nothing to do (`SetActorTickEnabled(false)`, `SetComponentTickEnabled(false)`).
- **Never poll another object's state in `Tick`.** Ask its owner for an event and bind to it (rule 12 in `AGENTS.md`).
- **No Event Tick in Blueprints.** Per-frame Blueprint logic is slow and invisible in code review; if something really needs it, it moves to C++.
- Work that has to happen often but not every frame uses a timer (`FTimerManager`) at the rate it needs.

### 12.2 What never happens every frame

Not in `Tick`, not in a timer that runs every frame, and not in a Slate or UMG binding:

- Searching the world: `TActorIterator`, `GetAllActorsOfClass`, `GetActorOfClass`, `FindComponentByClass`, `GetComponents`. Find things once (in `BeginPlay`, through a `Find` helper or by registration) and keep the reference, strong or weak as [section 7](#7-memory-pointers-and-object-lifetime) says.
- Loading assets, spawning or destroying actors, or creating `UObject`s.
- Building strings for display (`FString::Printf`, `FText::Format`) when the value hasn't changed.
- Debug drawing (`DrawDebug*`) as a game feature. It is compiled out of shipping builds; it is only for debugging tools behind a console variable.
- Physics traces that could be math. The target tile is found with the grid's lookup, not a trace.

### 12.3 Memory and allocation

- `Reserve` an array when its final size is known. Pass and return containers by `const&`, and move (`MoveTemp`) into sink parameters.
- Every `UObject` costs memory and garbage collection time. Short-lived data is a struct, not a `UObject`. Don't create `UObject`s or spawn actors per action for something that could be reused; when something is spawned often (projectiles, effects, floating text), measure and pool it. One exception: an ability may create its standard ability tasks (`WaitDelay`, `WaitMovementModeChange`, later montage tasks) per action, including from the previous task's callback, and the farming component may grant a tool's ability (one ability object) when the tool comes into the hand, because that is the engine's way of writing abilities and costs one small object per action ([CodeReview.md, L3](CodeReview.md#low), decided 2026-11-07). Never per frame: a held tool use makes at most a few a second.
- Look things up by key with `TMap` or `TSet` once a collection can grow past a few dozen entries. Content is looked up by `FName` ID, which compares as cheaply as an integer.

### 12.4 Assets and loading

- **A hard reference loads everything it points to, and everything those assets point to in turn.** Use `TObjectPtr` / `TSubclassOf` for assets only when the class always needs them. Content uses soft references and asset bundles ([section 7](#7-memory-pointers-and-object-lifetime), [Architecture.md, section 5](Architecture.md#5-data-and-content)).
- **No synchronous loads during interactive play.** `LoadSynchronous`, `TryLoad`, `WaitUntilComplete` and `UVestigeContentData::LoadWithVisuals` are only for play start and loading screens, where the cost is known. Everything else loads asynchronously (`FStreamableManager`, the Asset Manager) before it is needed.
- In Blueprints, a **Cast To** node or a variable of another Blueprint's type is a hard reference that loads that Blueprint and everything it references. Cast to C++ base classes or use interfaces instead.
- Check the **Size Map** and **Reference Viewer** (right-click the asset in the Content Browser) for every new or changed Blueprint and data asset, and make sure it doesn't drag in unrelated content.
- `ConstructorHelpers` only in constructors, and only for engine placeholder assets. Our own assets are set in a Blueprint's defaults or a data asset (`M_TileHighlight` on `BP_PlayerCharacter`'s highlight component), so code never names our content by path and the editor can move it.

### 12.5 Components and rendering from code

- Components that don't need collision have `NoCollision`. Primitive components generate overlap events by default; turn that off (`SetGenerateOverlapEvents(false)`) unless something listens.
- Small cosmetic meshes (crops, held tools, props) don't cast dynamic shadows unless they visibly need them (`SetCastShadow(false)`).
- Use `Movable` mobility only for components that move or change at runtime.
- Many copies of the same mesh (fences, rocks, a large field) use instanced static meshes or Nanite rather than one actor or component each, once profiling shows the count matters ([Architecture.md, section 10](Architecture.md#10-performance-by-design)).
- Prefer material parameters that don't need a dynamic material instance per object (per-instance custom data, a few shared material instances) for anything there will be many of. Each dynamic material instance is its own material for the renderer.

### 12.6 User interface

- Slate attribute bindings (`.Text(this, &SVestigeHudWidget::GetStatusText)`) and UMG property bindings run every frame. Bind only values that change every frame (the clock). Everything else is set when an event says it changed.
- A bound getter only reads values the widget or HUD already holds. It never searches, loads or calls into game code that does.

### 12.7 Bulk and once-a-day work

- Work that touches every tile, crop or robot happens while the player isn't playing: in the sleep transition or behind a loading screen, never in the middle of play.
- Each such pass stays within the hitch budget in [Architecture.md, section 10](Architecture.md#10-performance-by-design). When it doesn't, spread it over frames or move it behind the transition, and measure again.
- Saving is synchronous while a save stays within that budget; when it doesn't, it moves to `AsyncSaveGameToSlot`.

### 12.8 Instrument the expensive parts

- Every function that loops over many objects, runs on the new day, loads or saves, or runs on a frequent timer starts with `TRACE_CPUPROFILER_EVENT_SCOPE(ClassName::FunctionName);` so it shows up by name in Unreal Insights. It costs nothing when tracing is off.

### 12.9 Measuring

- Measure in a **packaged Development build** or a Standalone Game launch, not in Play in Editor: the editor adds its own cost and hides loading and shader hitches.
- Quick checks: `stat unit` (game thread, render thread, GPU and frame time), `stat unitgraph`, `stat game`, `stat gpu`. `ProfileGPU` breaks down one frame on the GPU.
- Detailed profiling: start the game with `-trace=default`, then open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`).
- First-time stutter (shader and PSO compilation, asset streaming) only shows in a packaged build run for the first time. Test that before every milestone build.
- Each roadmap milestone ends with a **performance pass**: the numbers in the [baseline table](Architecture.md#10-performance-by-design) are measured again on the target hardware and recorded. A change that breaks the budget is a bug, fixed before more features go in.

## 13. Testing

Game rules get automated tests. They run in seconds without playing the game, and they catch a broken rule the moment it breaks instead of three play sessions later.

- **Framework: CQTest**, Epic's newest C++ test framework (https://dev.epicgames.com/documentation/unreal-engine/cqtest-test-framework-for-unreal-engine). It wraps the engine's automation system with `TEST_CLASS` / `TEST` macros and automatic setup and teardown. It is the chosen framework for all new tests. Don't mix in the older Automation Spec or `IMPLEMENT_SIMPLE_AUTOMATION_TEST` styles.
- **Setup:** `Vestige.Build.cs` adds `"CQTest"` to `PrivateDependencyModuleNames` only when `Target.bBuildDeveloperTools` is set. CQTest is an engine developer module, so it never ships. No plugin needs enabling in `Vestige.uproject`: the CQTest plugin only contains Epic's tests of the framework itself.
- **What to test**: rules and data, not visuals. Tile transitions (grass, tilled, planted, ripe), growth only when watered, harvesting, tool use with no seeds left, the day clock rolling over, and save data round-tripping.
- **Where**: `Source/Vestige/Tests/<ClassName>Tests.cpp`, wrapped in `#if WITH_DEV_AUTOMATION_TESTS` so tests never ship. Name tests `Vestige.<Area>.<Behavior>`, for example `Vestige.Tile.WateredCropGrowsOneStage`. CQTest names a test `<directory>.<class>.<method>`, so write `TEST_CLASS(Tile, "Vestige")` with one `TEST_METHOD` per behavior. Put the class in a namespace named after the file (`FarmTileTests`) so the short fixture name can't clash with other declarations in unity builds.
- **Running**: from the editor's Test Automation window (Tools menu), filtered on `Vestige`, or from the command line with the editor closed:

  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Unreal Projects\Vestige\Vestige.uproject" -ExecCmds="Automation RunTests Vestige; Quit" -unattended -nopause -nullrhi -nosplash -testexit="Automation Test Queue Empty"
  ```

  Each test logs a `Test Completed. Result={Success}` line.
- **Design for testability**: keep rules in code that doesn't need a running level. When rule code is tangled with visuals or actor setup, that's a sign to separate them (see [Architecture.md, Known gaps](Architecture.md#17-known-gaps)).
- Run the tests before committing a change to game rules. A bug fix gets a test that would have caught it.

## 14. Project layout and assets

### 14.1 Source

The game is one module, `Vestige`, split into folders. **The folders are the architecture's layers** ([Architecture.md, section 14](Architecture.md#14-dependencies)), lowest first:

```
Source/Vestige/
    Vestige.Build.cs
    Core/           Shared services that know no game rules: VestigeActorLookup, UVestigeFeedbackSubsystem, the save registry (UVestigeSaveRegistry, IVestigeSaveable), the module, VESTIGE_LOG and the shared log categories
    AbilitySystem/  Gameplay Ability System parts that know no farm: the stamina set and effects, ability sets, native tags
    Interaction/    The interactable interface, the interaction component and the interact ability
    Content/        Content definitions: UVestigeContentData and its subclasses, UVestigeAssetManager
    Time/           The clock, its settings and save struct, and the day cycle's lighting
    Inventory/      The inventory component, its slot rules and save struct
    Farming/        Tiles and tile state, the grid, the farming, targeting, rest and highlight components, the bed, the tool abilities, their save structs
    Save/           UVestigeSaveGame and UVestigeSaveSubsystem: the save files
    Player/         The player character, its movement component, the player state and the cheat manager
    UI/             The HUD and its widgets
    Game/           The game mode and the player controller: the game's wiring, at the top
    Template/       Epic's template character; not maintained to this guide
    Tests/          CQTest tests, and test-only helpers such as UVestigeTestEventRecorder (WITH_TESTS only)
```

- **A folder includes only from its own layer and the layers below it.** `Farming/` may include `Time/`, `Content/` and `Core/`, never `Save/`, `Player/` or `UI/`. When a lower layer needs to tell a higher one something, it broadcasts an event and the higher one listens ([Architecture.md, section 4](Architecture.md#4-how-systems-talk-to-each-other)). `Tests/` may include anything, and nothing includes `Tests/`. `Template/` isn't maintained to this guide, but its includes are checked: it sits outside the layers and may include any of them, so only `Player/` (where the farmer derives from the template character) and the layers above may include it. `Game/`, the top layer, holds the game's wiring (the game mode names the HUD and the player state), so nothing includes it.
- **The test `Vestige.Architecture.IncludeDirection` enforces this.** It fails on a wrong-way include, on one of our headers included without its folder, and on a file in a folder that isn't a layer yet.
- **A new file goes in the folder of its layer.** Decide the owner first ([section 1](#1-ownership-and-separation-of-concerns)), then the layer. If a class doesn't fit any folder, or would have to include upward, that's a design question: raise it rather than bending the rule. A new folder is added to the layer table in the test and in Architecture.md section 14 in the same commit.
- **A save struct lives with its owner** (`FVestigeClockSaveData` in `Time/`), and the owner registers with the save registry in `Core/`, so saving never pulls a lower layer upward and `Save/` names no owner.
- **Include our headers by their path from the module root**, own header included: `#include "Farming/FarmGrid.h"`. The `"Vestige"` entry in `PublicIncludePaths` is what puts that root on the include path (a build without it fails), so keep it. Don't add the feature folders to the include path: bare includes like `"FarmTile.h"` hide where a file lives.
- **Folders, not modules.** Splitting the game into several modules is a separate decision ([Architecture.md, section 9](Architecture.md#9-modules-and-plugins) and the decision log). If another module ever needs our headers, give the module a `Public/` and `Private/` split and move only the headers meant for it into `Public/`.
- Move files between folders in a commit of its own, with no other change. Unreal names classes by module and name, not by folder, so moving a file never affects Blueprints, levels or saves. Regenerate the Visual Studio project files afterwards (right-click `Vestige.uproject` > Generate Visual Studio project files).

### 14.2 Content and asset naming

Follow Epic's **Recommended Asset Naming Conventions** (https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects): `Prefix_BaseAssetName_Variant_Suffix`. The table below lists the prefixes we use most; for any other asset type, use the prefix from Epic's page.

Our assets live under `Content/Vestige/`, split by type. Leave the template's folders (`Characters`, `ThirdPerson`, `Input`) alone.

| Prefix | Asset type | Example |
|---|---|---|
| `L_` | Level / map | `L_Farm` |
| `BP_` | Blueprint class | `BP_FarmGrid` |
| `WBP_` | Widget Blueprint | `WBP_Inventory` |
| `ABP_` | Animation Blueprint | `ABP_Farmer` |
| `DA_` | Data asset | `DA_Crop_Parsnip` |
| `SM_` | Static mesh | `SM_Crop_Ripe` |
| `SK_` | Skeletal mesh | `SK_Farmer` |
| `M_` / `MI_` | Material / material instance | `MI_Soil_Watered` |
| `T_` | Texture (with suffix `_D`, `_N`, `_ORM`) | `T_Soil_D` |
| `IA_` / `IMC_` | Input action / mapping context | `IA_UseTool` |
| `S_` / `SC_` | Sound wave / sound cue | `S_Hoe` |
| `NS_` | Niagara system | `NS_WaterSplash` |

Variants go after the base name with underscores: `SM_Crop_Stage0`, `SM_Crop_Stage1`.

Blender sources go in `Art/Blender/`, exported FBX files in `Art/Export/`, with the same names as the imported assets.

## 15. Git workflow

- **Small, focused commits.** One logical change per commit. Refactors and moves go in their own commits, separate from behavior changes.
- **Commit messages**: an imperative subject line under about 70 characters (`Move lighting from FarmGrid to VestigeDayCycle`), a blank line, then a short body saying *what* changed and *why*.
- **Author and committer**: `David Bennehag <david.bennehag@gmail.com>`.
- **No `Co-Authored-By:` or `Claude-Session:` trailers** or any other tool attribution in commit messages.
- **No pull requests.** Commits are made locally on `main` on David's machine. David reviews and pushes. Nobody else pushes.
- Binary files (`.uasset`, `.umap`, `.blend`, `.fbx`, `.png`) go through Git LFS, as set up in `.gitattributes`.
- Never commit `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` or `.vs/`.
- Code must **compile and be play-tested** in the editor before it is pushed. What to play-test is in [PlayTest.md](PlayTest.md).

## 16. Known deviations to clean up

Existing code that doesn't yet match these guidelines. Fix them when touching the code nearby, each in a small commit. Don't do a sweeping cleanup in the middle of a feature. Design-level gaps are listed in [Architecture.md, Known gaps](Architecture.md#17-known-gaps).

Add an entry here when code is committed that doesn't meet the guide, with what is missing, and a roadmap item to fix it.


## 17. Review checklist

Before committing, check:

- [ ] Each change lives on the class that owns that responsibility ([section 1](#1-ownership-and-separation-of-concerns)).
- [ ] Design changes are reflected in [Architecture.md](Architecture.md) (ownership table, decision log).
- [ ] The change follows the guide in [Systems.md](Systems.md) for each system it touches, and the guide is updated if the system changed.
- [ ] Names follow the prefixes, the `Vestige` or `Farm` project prefix and PascalCase, and nothing shadows a base class member.
- [ ] Changed files are formatted with clang-format.
- [ ] New source files start with the copyright and SPDX license header.
- [ ] Headers forward declare; `.cpp` files include what they use, own header first.
- [ ] New files are in the folder of their layer, and includes only go to the same layer or lower ones (`Vestige.Architecture.IncludeDirection` passes).
- [ ] Strong `UObject` references are `UPROPERTY() TObjectPtr<>`; non-owning ones are weak, deferred assets are soft. Pointers that can be null are checked.
- [ ] Runtime state is private. Nothing new is exposed to Blueprints without a reason.
- [ ] No magic numbers, no duplicated logic, no commented-out code, no stray debug messages.
- [ ] Ticking is off unless needed and justified in a comment. Nothing polls another object's state; nothing in `Tick`, timers or UI bindings searches the world, loads, spawns, allocates `UObject`s or rebuilds unchanged strings ([section 12](#12-performance)); an ability's tasks per action are the one exception ([section 12.3](#123-memory-and-allocation)).
- [ ] New assets and Blueprints are soft-referenced where they aren't always needed, and their Size Map doesn't pull in unrelated content. No synchronous loads during play.
- [ ] Loops over many objects, new-day work, loading and saving have a `TRACE_CPUPROFILER_EVENT_SCOPE`.
- [ ] Classes and non-obvious public members have `/** */` comments.
- [ ] Changed game rules have tests, and the `Vestige` tests pass.
- [ ] It compiles and works in Play in Editor.
- [ ] Anything only a person can check is listed in [PlayTest.md, Pending checks](PlayTest.md#2-pending-checks); rules have automated tests, not only a manual check.
- [ ] The commit message is clear and has no tool attribution lines.

## 18. Further reading

Official Epic references, checked against UE5:

- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine): the baseline for this document.
- [Recommended Asset Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects): asset prefixes and naming.
- [Class Specifiers](https://dev.epicgames.com/documentation/en-us/unreal-engine/class-specifiers), [UProperties](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-uproperties), [UFunctions](https://dev.epicgames.com/documentation/en-us/unreal-engine/ufunctions-in-unreal-engine) and [Metadata Specifiers](https://dev.epicgames.com/documentation/en-us/unreal-engine/metadata-specifiers-in-unreal-engine): what every `UCLASS`, `UPROPERTY`, `UFUNCTION` and `meta=` option does.
- [CQTest](https://dev.epicgames.com/documentation/unreal-engine/cqtest-test-framework-for-unreal-engine) and the [Automation Test Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine): writing and running tests.
- The Lyra Starter Game documentation and source: architecture reference (see [Architecture.md](Architecture.md#11-patterns-borrowed-from-lyra)).

Beyond Epic:

- [Tom Looman](https://www.tomlooman.com/): practical Unreal C++ articles on the gameplay framework, subsystems and performance, kept up to date with recent engine versions.
- [Game Programming Patterns](https://gameprogrammingpatterns.com/) by Robert Nystrom: not Unreal-specific, but its chapters on State, Observer, Component and Update Method describe patterns this project already uses. Older, but the patterns haven't aged.
