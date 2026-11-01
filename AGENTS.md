# Project Vestige agent instructions

Instructions for AI coding agents working in this repository. Humans can read it too; it is a short summary of how the project works and the rules to follow.

## Project

- A cozy 3D farming and life-sim game, currently a small playable prototype.
- Unreal Engine 5.8, C++ first, Blueprints for content. Art made in Blender.
- Single game module `Vestige` in `Source/Vestige/`, built from the Third Person C++ template, with its code in folders by architecture layer. Include our headers by folder path (`#include "Farming/FarmTile.h"`).
- Main level: `/Game/Maps/L_Farm`. Our assets live under `Content/Vestige/`. Blender sources in `Art/Blender/`, exports in `Art/Export/`.
- Developed by one person (David) on Windows, in `C:\Unreal Projects\Vestige`. Open the editor via `Vestige.uproject`.

## Read first

- **[Docs/CodingGuidelines.md](Docs/CodingGuidelines.md)** is the coding standard: how code looks. It is based on Epic's C++ Coding Standard and clean code principles.
- **[Docs/Architecture.md](Docs/Architecture.md)** is the design: principles, where things go in Unreal's framework, who owns what, how systems talk, and the decision log.
- **[Docs/Systems.md](Docs/Systems.md)**: best practice per game system (saving, content, input, UI, the Gameplay Ability System, AI, inventory...), from Epic's documentation and the Lyra sample. Read the guide for every system a change touches.
- **[Docs/Roadmap.md](Docs/Roadmap.md)**: what's done and what's next. Tick items in the same commit that finishes them.
- **[Docs/PlayTest.md](Docs/PlayTest.md)**: what David checks by hand in the editor: pending checks for recent changes and the smoke test.

- **[LICENSING.md](LICENSING.md)**: code is GPL-3.0-or-later with an Unreal Engine linking permission, assets are CC BY-NC-SA 4.0, Epic template material is under the UE EULA. New source files start with the SPDX license header shown there. Never add third-party code or assets without checking they are under a license CONTRIBUTING.md accepts, and record their source and license in CREDITS.md and the path table in LICENSING.md.
- **[CONTRIBUTING.md](CONTRIBUTING.md)** covers setup, art and audio pipelines, and the workflow for outside contributors.

Follow the two Docs files for every change. The rules below are the ones most often broken; the documents have the details.

## Hard rules

1. **Ownership.** Every actor, component and subsystem has one job. Put new logic, components and settings on the class that owns that responsibility, never on whichever class is convenient. If no class fits, create a new actor, component or subsystem and say so. Current owners (the full table is in `Docs/Architecture.md`, section 13; keep both in sync):
   - `AFarmTile` and its `FFarmTileState`: one tile's state and rules (including the crop planted in it), and its visuals.
   - `AFarmGrid`: spawns tiles, finds a tile by location, advances tiles on a new day. No clock, no lighting.
   - `AVestigeDayCycle`: day number, clock, sun, sky light, night exposure. Broadcasts `OnNewDay`, then `OnNewDayFinished`, and `OnDayEndReached` when the clock reaches the end of the day.
   - `UFarmingComponent`: the toolbar and the tool in hand, which seed item the Seeds tool plants and its crop, tile targeting, the tools' rules (taking seeds from and putting harvests into the inventory). Using a tool goes through the use-tool ability.
   - `AVestigePlayerState`: the player's data that outlives the pawn; holds the inventory and the ability system component, and grants the ability sets.
   - `UVestigeInventoryComponent` and its `FVestigeInventory`: the items the player carries, in slots with stacks, by item ID, and the starting items. No money yet.
   - `UVestigeRestComponent`: the end of the player's day: sleeping (asked by the bed, or N as a test key) and passing out, each refilling stamina (later the late-night rules and exhaustion).
   - `UVestigeGameplayAbility_UseTool`: what a tool use costs and how long it takes (twice as long at zero stamina); one use at a time.
   - `IVestigeInteractable` and its `FVestigeInteractionOption`: what anything the player can use offers an interactor now, and doing it. The interactable owns what happens.
   - `UVestigeInteractionComponent`: finding the nearest interactable (a timer query on the `Interactable` collision channel) and using its first option. Interacting goes through the interact ability (`UVestigeGameplayAbility_Interact`).
   - `AVestigeBed`: being a bed: offers Sleep and asks the interactor's rest component to sleep.
   - `UVestigeStaminaSet`: the stamina numbers and their limits (0 to the maximum), and the out-of-stamina tag. Changed only through gameplay effects.
   - `UVestigeAbilitySet`: a data asset of attribute sets and abilities granted together.
   - `UVestigeCharacterMovementComponent`: how the farmer walks and turns; without input it turns toward the aimed tile.
   - `UFarmTargetHighlightComponent`: the frame around the targeted tile (presentation only).
   - `AVestigePlayerCharacter`: input bindings (and the key hints that describe them), turning movement into the tile aim (the walk eases in, so a tap barely steps), showing the current tool's held mesh, and handing out the player state's ability system; nothing else.
   - `AVestigeHUD` / `SVestigeHudWidget`: display only.
   - `UVestigeHudSettings`: how the HUD shows feedback messages (how many at once, for how long), in Project Settings.
   - `UVestigeFeedbackSubsystem`: carries messages for the player from game code to the HUD. Game code calls `Notify`; it never calls the HUD.
   - `UVestigeSaveSubsystem` / `UVestigeSaveGame`: saving and loading.
   - `UVestigeContentData`: base of every content data asset. Owns the `ContentId`, display name and loading through the Asset Manager.
   - `UVestigeAssetManager`: the project's Asset Manager; startup checks of all content (duplicate content IDs).
   - `UFarmCropData`: one crop's definition (growth, meshes, the item it yields and how many). No runtime state.
   - `UVestigeToolData`: one tool's definition (action, held mesh, stamina cost, how long a use takes). No runtime state.
   - `UVestigeItemData`: one item's definition (stack size, sell price) and its fragments (`UVestigeItemFragment` subclasses such as `UFarmSeedFragment`) for what only some items have. No runtime state.
2. **Epic naming.** `A`/`U`/`S`/`F`/`E`/`I`/`T` type prefixes, `b` for booleans, PascalCase everywhere, and the `Vestige` project prefix on every new type (`AVestigePlayerState`, `FOnVestigeInventoryChanged`). Types about the farm itself (tiles, grid, crops, farming actions) use `Farm` instead (`AFarmTile`, `EFarmTileState`). The general types were renamed from `Farm` on 2026-11-01; a rename of a type that assets or saves name needs `[CoreRedirects]` and a test (guidelines section 2.2).
3. **No shadowing.** The build treats MSVC C4458 as an error. Never give a local or parameter the name of a member of the class or any engine base class (`Mesh` in a character, `Owner`, `Controller`, `Tags`...).
4. **Formatting.** Tabs, braces on their own lines, braces on every block. Format changed files with the repo's `.clang-format`; don't reformat untouched files.
5. **UObject pointers.** Strong references are `UPROPERTY() TObjectPtr<T>`; non-owning references are `TWeakObjectPtr<T>`, assets loaded on demand are `TSoftObjectPtr<T>` (guidelines section 7). Check pointers that can be null. No `new`/`delete` on UObjects.
6. **Encapsulation.** Runtime state is private with getters. Don't add public mutable data members. Expose to Blueprints only when a Blueprint needs it or as a deliberate, commented modding extension point.
7. **Communicate through delegates and small functions**, not by reaching into another class's state.
8. **Keep it simple.** Small functions, early returns, named constants instead of magic numbers, no duplicated logic, no commented-out code, no new debug-only on-screen messages. Don't add Lyra machinery (Experiences, Game Features, Common UI) or replication unless asked. The Gameplay Ability System is adopted, starting with stamina, and is built only as `Docs/Systems.md` section 9 describes.
9. **Dependencies point one way.** Game rules never call UI, audio or effects; presentation reads state and listens to events. No circular dependencies between classes. The source folders are layers (`Core`, `AbilitySystem`, `Interaction`, `Content`, `Time`, `Inventory`, `Farming`, `Save`, `Player`, `UI`, lowest first): a lower folder never includes a higher one, new files go in the folder of their layer, and the test `Vestige.Architecture.IncludeDirection` enforces it (guidelines section 14.1).
10. **Record design decisions.** A change that adds a system, moves a responsibility or makes a design choice updates `Docs/Architecture.md` (Part B and the decision log) in the same commit.
11. **Design for mods.** Content (crops, tools, items) is data assets found through the Asset Manager, never hard-coded lists or enums. Content has namespaced IDs (`Base.Parsnip`). Saves store IDs and skip unknown content. See `Docs/Architecture.md`, section 12.
12. **Performance is a requirement.** Ticking off by default; use events and timers instead of per-frame polling. Nothing in `Tick`, timers or UI bindings searches the world, loads assets, spawns, creates `UObject`s or rebuilds unchanged strings. Content is soft-referenced and never loaded synchronously during play. No debug drawing as a feature. Expensive loops and new-day work get a `TRACE_CPUPROFILER_EVENT_SCOPE`. Optimizations that cost readability need a measurement. Rules in `Docs/CodingGuidelines.md` section 12, budget in `Docs/Architecture.md` section 10.
13. **Follow the system's guide.** Each game system follows its guide in `Docs/Systems.md`. A system without a guide gets one first, written from Epic's documentation and Lyra's source and approved by David, before any code. Adopting an engine framework (Gameplay Ability System, Common UI, StateTree, Mass) is a decision log entry.
14. **Tests for rules.** Game rule changes get CQTest tests in `Source/Vestige/Tests/`, named `Vestige.<Area>.<Behavior>`. A bug fix gets a test that would have caught it. How to write and run them is in guidelines section 13.

## Building and testing

- C++ can only be compiled on David's Windows machine with Unreal Engine installed. Cloud containers have no engine.
- If you can build (for example in a session on David's PC), build the `VestigeEditor` target (Development Editor, Win64) before committing and fix every error and warning you introduced.
- Run the `Vestige` automation tests when you can build.
- If you can't build, say clearly that the change is uncompiled.
- **A change that needs a manual check** (visuals, feel, lighting, the HUD, input, saves on real files, performance, editor-only steps) adds it to `Docs/PlayTest.md`, section "Pending checks", in the same commit: a dated heading, the commit and what to look for. Game rules are always tested automatically, never by a manual check only.
- Live Coding can't add new `UPROPERTY`s, `UFUNCTION`s or classes, and a build can't run while the editor has the project open. Ask David to close the editor before such a change. When you built after your last source change, tell him the editor is safe to open; ask him to rebuild only when you couldn't build.

## Git

These rules are for agents working for David, the maintainer. If you are helping another contributor, follow the pull request workflow in [CONTRIBUTING.md](CONTRIBUTING.md) instead, with that contributor as the author.

- **Do not open pull requests and do not push.** Commit locally on `main` on David's machine; David reviews and pushes himself.
- **Author and committer: `David Bennehag <david.bennehag@gmail.com>`.**
- **No `Co-Authored-By:`, `Claude-Session:` or any other tool attribution** in commit messages, even if your tooling asks for it.
- Small, focused commits with an imperative subject line and a short body explaining what and why. Moves and refactors in separate commits from behavior changes.
- Binary files go through Git LFS (see `.gitattributes`). Never commit `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` or `.vs/`.

## Working with David

- David is an experienced C++ programmer who is new to Unreal Engine and Blender. Don't explain C++; do explain Unreal specifics (editor menus, settings, why an engine pattern is used) concretely.
- **Be critical of David's decisions.** Because he is new to Unreal, he relies on you to say when a choice is wrong or risky. Give an honest assessment with reasons and the better alternative before agreeing, even when he sounds sure; then follow his final call and record it.
- Before doing work that touches several systems, say which class will own the new responsibility.
- Game presentation preferences: 24-hour clock (06:40, 21:30), and properly dark nights.
- Don't name other games in docs, code comments or commit messages.
