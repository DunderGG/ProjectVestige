# 21. Logging

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** built (2026-11-02, [Roadmap 4.1](../Roadmap.md#41-foundations)) as designed below, approved by David on 2026-11-01: the categories in the table, `VESTIGE_LOG`, and every line in our code moved to them. Chosen on 2026-10-30: we build on Unreal's logging and write no logging tool of our own, which would rebuild what the engine has and lose the Output Log's filters and colours, the log files, crash reports and the `log` console command.

**References:** Epic's "Logging in Unreal Engine" page (log categories, verbosity levels, `UE_LOG`, the `log` console command and `[Core.Log]`), "Structured Logging" (`UE_LOGFMT`) and "Visual Logger". The engine source: `Logging/LogMacros.h` (`DECLARE_LOG_CATEGORY_EXTERN`, compile-time verbosity), `Logging/StructuredLog.h` (`UE_LOGFMT` checks the category's verbosity before it evaluates its fields) and `OutputLogSettings.h` (Category Colorization Mode). Lyra's `LyraLogChannels.h`: a few categories per area (`LogLyra`, `LogLyraExperience`, `LogLyraAbilitySystem`, `LogLyraTeams`) declared together, plus `LogLyraCheat` and `LogLyraGamePhase` declared next to their systems, and `GetClientServerContextString(this)` passed into lines that need to say where they came from.

**What we take from Lyra and what we skip:**

- **Take:** one category per system, named with the project prefix, each `Log` by default and `All` at compile time so its tracing can be turned on while the game runs; the context of the line passed in from the calling object.
- **Skip:** the client and server context string. We have no networking; the sender's name and function take its place.
- **Change:** Lyra's lines use `UE_LOG` with printf formats. We use `UE_LOGFMT` underneath our macro: its fields are typed, so the printf mistakes (an `FString` without `*`, `%d` for an `int64`) can't happen, and the fields stay separate for tools that read structured logs.

**Rules:**

- **One category per system,** not per class or per file, so a filter shows one system's whole story. Every line in our code (outside `Template/`) uses one of these:

  | Category | System | Declared in |
  | --- | --- | --- |
  | `LogVestige` | Level setup that belongs to no one system (`VestigeActorLookup`), and Epic's template code | `Core/VestigeLog.h` |
  | `LogVestigeFeedback` | Every feedback report for the player (its tag and values), written by `UVestigeFeedbackSubsystem::Report`, and the HUD's warning for a report without a response | `Core/VestigeLog.h` |
  | `LogVestigeAbilitySystem` | Ability sets, stamina, abilities | `AbilitySystem/VestigeAbilitySystemLog.h` |
  | `LogVestigeContent` | Loading content, content IDs, the Asset Manager's checks | `Content/VestigeContentLog.h` |
  | `LogVestigeTime` | The clock and the sky lighting | `Time/VestigeTimeLog.h` |
  | `LogVestigeInventory` | Items and the inventory | `Inventory/VestigeInventoryLog.h` |
  | `LogVestigeInput` | The input config and the player's bindings | `Player/VestigeInputLog.h` |
  | `LogFarm` | The farm: grid, tiles, crops, tools | `Farming/FarmLog.h` |
  | `LogVestigeSave` | Saving and loading | `Core/VestigeSaveLog.h` |

- **Names follow the type prefixes** (AGENTS rule 2): `LogVestige<System>`, and `LogFarm` for the farm, which is one system today (`LogFarm<System>` if it ever needs splitting). A new system gets its category with its first line (`LogVestigeInteraction`, `LogVestigeRest`, `LogVestigeHud`), not before, in the folder of the code that uses it, and this table gets a row.
- **Declared in the layer's own log header,** `<Layer>/<Prefix><System>Log.h` with `DECLARE_LOG_CATEGORY_EXTERN(LogVestigeSave, Log, All);`, and defined in the matching `.cpp` with `DEFINE_LOG_CATEGORY`. The layer's log header includes `Core/VestigeLog.h`, so a file includes only its own layer's log header to get both its category and the macro, and the include direction test keeps holding. `Core/Vestige.h` keeps including `Core/VestigeLog.h`, so the template code compiles unchanged.
- **Every line goes through `VESTIGE_LOG(Sender, Category, Verbosity, Format, ...)`** in `Core/VestigeLog.h`, a thin macro over `UE_LOGFMT`. The sender comes first, as in Epic's `UE_VLOG`. It puts the sender's name and the function in front of the message:

  ```cpp
  VESTIGE_LOG(this, LogFarm, Warning, "The Harvest Tool Id ({ToolId}) names no tool, so nothing can be harvested.", HarvestToolId.ToString());
  VESTIGE_LOG(this, LogVestigeSave, Error, "Couldn't write the save to slot {Slot}. The previous save is kept.", SlotName);
  ```

  prints

  ```
  LogFarm: Warning: [BP_PlayerCharacter_C_0.Farming] UFarmingComponent::BeginPlay: The Harvest Tool Id (VestigeTool:Base.Hand) names no tool, so nothing can be harvested.
  LogVestigeSave: Error: [VestigeSaveSubsystem_0] UVestigeSaveSubsystem::SaveGame: Couldn't write the save to slot Farm. The previous save is kept.
  ```

   - The sender is any `UObject`: `this` in members; in a static or free function the object the line is about (the world for `VestigeActorLookup::FindSingle`), or `nullptr`. Its name comes from `VestigeLog::GetSenderName`: an actor's or subsystem's own name (`FarmGrid_1`), a component's owner and name (`BP_PlayerCharacter_C_0.FarmingComponent`, since a component's name alone isn't unique), `None` for `nullptr`.
   - The function is the compiler's `__FUNCTION__`, which on MSVC includes the class (`UFarmingComponent::BeginPlay`).
   - The format is a plain string literal (no `TEXT()`), with fields in braces named after what they hold (`{Slot}`, `{ItemId}`) and given in order after it. Named fields can't be mixed with the macro's own, so they aren't used.
   - Field values are numbers, `bool`, `FString`, `FName` and the engine's ID types. Pass an `FText` as `.ToString()`, an `FIntPoint` or vector as `.ToString()`, and an object as `GetNameSafe(Object)`.
   - `UE_LOG`, `UE_LOGFMT` and their conditional forms aren't called directly outside `Template/` and the macro. A source test checks it.
- **The message** is a full sentence that says what happened and what follows from it: "The save has a crop {CropId} that no longer exists, so tile {Tile} is empty." It doesn't repeat the sender's name, which the macro already prints.
- **Verbosity:**

  | Verbosity | Use it for | Today's examples |
  | --- | --- | --- |
  | `Fatal` | Never. It crashes the game; use `check` for programmer errors ([CodingGuidelines.md, section 10](../CodingGuidelines.md#10-errors-logging-and-assertions)) | |
  | `Error` | Broken state someone must fix: the game carries on, but something is lost or turned off. Fails an automation test that logs it, unless the test expects it | Duplicate content IDs, a save that couldn't be written or read, two grids in a level |
  | `Warning` | A setup or content problem the game works around: a missing setting, an unknown ID in a save | A Harvest Tool Id that names no tool, an unknown item ID, no room for saved items |
  | `Display` | Rare: a line for someone reading the console window of a packaged game, where `Log` lines don't show | None yet. Later: which mods were loaded |
  | `Log` | Notable one-off events, a handful per day of play | A save loaded, a backup made, item definitions loaded, a feedback report |
  | `Verbose` | Tracing one action: a tool use, an interaction, a tile changing state. Off by default | None yet |
  | `VeryVerbose` | Tracing inside loops: each tile on a new day, each slot when loading. Off by default | None yet |

- **Volume:** nothing is logged every frame or on every tick of a timer that always runs (the interaction scan), at any level, not even `Verbose`: a suppressed line still costs a check, and turning tracing on must leave the game playable. At `Log` and above, a line per player action is already too much; that is `Verbose`. Arguments are evaluated only when the line is printed, so put the work (`ToString`, `GetNameSafe`) in the macro's arguments rather than in locals built before it.
- **Turning a category up or down:**
   - While playing, in the console (the backtick key, or the Output Log's command box): `log LogFarm Verbose`, `log LogVestigeSave Warning`, `log list` to see every category, `log reset` to go back.
   - From the start of a run: the command line `-LogCmds="LogFarm Verbose, LogVestigeSave Verbose"`.
   - For good: a line under `[Core.Log]` in an `Engine.ini`, such as `LogFarm=Verbose`. Personal settings go in `Saved/Config/WindowsEditor/Engine.ini`, which git ignores; `Config/DefaultEngine.ini` gets a `[Core.Log]` line only for a decision everyone shares, recorded in the decision log.
   - Shipping builds compile logging out (our `Vestige.Target.cs` doesn't set `bUseLoggingInShipping`), so the lines cost nothing there.
- **Reading our lines in the editor's Output Log** (Window > Output Log): the Filters menu has Categories, where we tick ours (it lists only the categories that have written a line since the editor started, so a system that only warns, like `LogFarm`, is missing until it does), and the verbosity filters (Messages, Warnings, Errors); the search box filters by text, so `LogVestige` finds every Vestige category. Editor Preferences > General > Output Log > Category Colorization Mode gives each category its own colour (David uses Colorize Whole Line, which leaves warnings and errors in their own yellow and red). Both are per-user editor settings, nothing committed; [PlayTest.md](../PlayTest.md) says how to set them. The full log of each run is in `Saved/Logs/Vestige.log`.
- **Reading a line.** In the log file and the console window a line looks like this (the Output Log window leaves out the first two parts, unless its Output Log Window Timestamp Mode, on the same preferences page, adds a time):

  ```
  [2026.11.02-19.56.43:858][647]LogFarm: Warning: [BP_PlayerCharacter_C_0.Farming] UFarmingComponent::ReportNothingInHand: The item SomeMod.Scythe names the tool VestigeTool:SomeMod.Scythe, which doesn't exist, so it can't be used.
  ```

  | Part | Written by | What it is |
  | --- | --- | --- |
  | `[2026.11.02-19.56.43:858]` | The engine | When: date, then time to the millisecond, in UTC (the engine's default, `LogTimes=True` in `BaseEngine.ini`), so an hour behind Swedish winter time |
  | `[647]` | The engine | The frame the line was written in, counted modulo 1000, so it wraps from 999 to 0. Lines with the same number come from the same frame; the gap between two numbers is how many frames passed |
  | `LogFarm:` | The engine | The category: which system wrote it |
  | `Warning:` | The engine | The verbosity. Left out for `Log` lines |
  | `[BP_PlayerCharacter_C_0.Farming]` | `VESTIGE_LOG` | The sender: the object that wrote the line (here the `Farming` component on the player character), the level's world for code that has no object (`[L_Farm]`), or `None` |
  | `UFarmingComponent::ReportNothingInHand:` | `VESTIGE_LOG` | The C++ function the line comes from, with its class |
  | The rest | The line itself | The message, with its fields (`SomeMod.Scythe`) filled in |
- **The Visual Logger** (Tools > Debug > Visual Logger, `UE_VLOG` and its shape macros) records lines and shapes per actor over time and plays them back on a timeline. It is for gameplay that moves: the robots' AI gets Visual Logger output with its Gameplay Debugger categories when it is built ([section 19](19-DebuggingAndCheats.md)). The farm doesn't need it today; its rules are tested instead.
- **Logs aren't feedback.** A line in the log is for developers and modders. What the player must know is reported as a feedback tag through `UVestigeFeedbackSubsystem::Report` ([CodingGuidelines.md, section 10](../CodingGuidelines.md#10-errors-logging-and-assertions), [section 15](15-EffectsAndFeedback.md)), which also logs it to `LogVestigeFeedback`.
- **Tests** (`Vestige.Logging.*`, in `Tests/VestigeLogTests.cpp` on a test-only category): a line through the macro is exactly `[sender] function: message` with its fields filled in (captured with an output device on `GLog`); a component's sender name is its owner's name and its own, an actor's is its own, and `nullptr` gives `None`; nothing in the macro's arguments is evaluated while the category is turned down. `Vestige.Architecture.LogsUseTheSenderMacro` scans our source, as the include direction test does, and fails on a direct `UE_LOG`, `UE_CLOG`, `UE_LOGFMT` or `UE_CLOGFMT` outside `Template/`, `Tests/` and `Core/VestigeLog.h`.

**Pitfalls:**

- One category for everything: the Output Log's category filter can't show one system's story.
- A category per class: a filter for one system means ticking a dozen boxes, and the colours stop meaning anything.
- `Warning` or `Error` for something that is normal play (no seeds left, the inventory full). Those are player feedback, maybe a `Verbose` line.
- `Error` in code that tests run on purpose: the test fails unless it declares the error it expects.
- Logging inside `Tick`, a timer that always runs, or a Slate binding.
- An on-screen debug message instead of a log line (AGENTS rule 8).
