# Coding guidelines audit

**Reviewed:** 2026-10-26 (the guide), 2026-10-29 (the code against the guide, then again after the clean-up)  
**Scope:** A critical review of [CodingGuidelines.md](CodingGuidelines.md) as practical C++ and Unreal Engine 5.7 guidance for FarmSim. This assesses correctness, consistency, clarity and fit for this project. The first review (C-01 to C-08) did not check the code. The second review (C-09 onward, [Code review findings](#code-review-findings-2026-10-29)) checks the code in `Source/Vestige/`, `Config/` and the project's Blueprints against the guide and general Unreal C++ practice.

## Executive assessment

**Overall: 6/10.** The guide is well organized, concrete, and tailored to the project's Unreal workflow. Its emphasis on ownership, UObject lifetime, IWYU, formatting, tests and explicit error handling gives a solo developer a useful review checklist.

The main weakness is that several rules are absolute where Unreal and C++ need context, and some rules contradict either their own examples or [Architecture.md](Architecture.md). The biggest risks are in pointer ownership language, bool-returning action naming, test setup status, and guidance that could encourage presentation dependencies. The guide is a good local style baseline, but some rules should be clarified before treating it as a source of truth.

Scores use a 1–10 scale: 1 means misleading or absent; 5 means usable but has material gaps; 10 means accurate, specific and actionable for this project's scope.

| Area | Score | Assessment |
|---|---:|---|
| C++ and Unreal correctness | 6/10 | Many sound defaults, but pointer, loading and framework rules need qualifications. |
| Internal consistency | 5/10 | A few contradictions between naming rules and examples, plus a cross-document presentation mismatch. |
| Memory, reflection and lifetime guidance | 6/10 | Strong awareness of GC and weak references, but the blanket `TObjectPtr` rule obscures reference semantics. |
| Naming and formatting | 7/10 | Specific and mostly enforceable; some naming rules are too rigid for action APIs. |
| Build and module guidance | 5/10 | IWYU is good, but the proposed folder/include-path migration is over-prescriptive. |
| Testing and verification | 4/10 | Useful test intent, but CQTest is described as the standard before setup is complete. |
| Error handling and diagnostics | 6/10 | Good distinction between programmer errors and runtime failures; HUD advice needs to respect dependency direction. |
| Maintainability and usability | 7/10 | Strong layout and checklist; numeric thresholds and absolutes need rationale or softer language. |

### Code conformance (2026-10-29)

**Overall: 7/10.** The code is small, readable and mostly follows the guide: every class has one job and a header comment, names and prefixes are consistent, UObject references use the right pointer types, content uses soft references with asset bundles, and the newest code (`FFarmToolbar`, `UFarmContentData`) is tested and cleanly separated. The gaps are concentrated in a few places: the three known deviations from section 16 are still open, the code isn't clean under the repo's own `.clang-format`, player-facing text isn't `FText`, the HUD searches the level every frame, and tests cover the tile and toolbar but not the farming component, clock or saving.

| Area | Score | Assessment |
|---|---:|---|
| Naming, headers and comments | 8/10 | Prefixes, `Farm` prefix, SPDX headers and class comments are consistent. A few engine overrides are public and some action results are undocumented. |
| Formatting | 6/10 | Hand-formatted code is close to the rules, but eight files change under clang-format and three lines exceed 140 characters. |
| Memory, pointers and lifetime | 8/10 | Strong, weak and soft references are used as section 7 describes. Delegate bindings and spawned tiles are never cleaned up in `EndPlay`. |
| Encapsulation | 5/10 | Seed and harvest counts and many tunables are still public mutable members, as section 16 already records. |
| Functions, constants and duplication | 7/10 | Functions are short with early returns. Magic numbers remain in visual code, and key hints duplicate the key bindings. |
| Errors, logging and player text | 6/10 | Good warnings for content mistakes. Player-facing strings are `FString`, and gameplay still calls the HUD. |
| Performance and ticking | 6/10 | Ticking is off by default, but the HUD searches the level every frame and the farming component polls the clock and draws debug shapes every frame. |
| Tests | 6/10 | CQTest is set up and 13 tests pass. The farming component, clock and save round trip have none. |

## Findings

### C-01 — The bool naming rule conflicts with the `Till()` API

**Severity: Medium**

**Status: Resolved (2026-10-28).** [CodingGuidelines.md](CodingGuidelines.md) section 2.3 now separates state queries, which are named as questions, from actions that can fail, which keep their verb name and return `bool` for success, documenting what `false` means. `Till()` is the example, consistent with the code.

Section 2.3 says functions returning `bool` ask a question, with examples such as `IsRipe()` and `HasMessage()`. But the formatting example uses `AFarmTile::Till()` returning `bool` to indicate whether an action succeeded, and section 2.4 also uses the action-oriented `Till()` name.

**Why it matters:** The rule gives conflicting direction for common gameplay APIs that both perform an action and report success. A contributor could produce awkward names such as `DidTillSucceed()` or incorrectly change a useful result type.

**Recommended change:** Distinguish predicates from command results. Use question names for state queries (`IsRipe`); allow action names to return success, or prefer a consistent `TryTill` convention where the return value means the action may fail. Keep the documented choice consistent across examples and code.

### C-02 — “Every UObject member is TObjectPtr” blurs strong, weak and soft references

**Severity: High**

**Status: Resolved (2026-10-28).** Sections 6 and 7 now describe pointer types by meaning: strong `UPROPERTY() TObjectPtr` references keep a target alive, non-owning references stay `TWeakObjectPtr`, and deferred assets stay `TSoftObjectPtr`/`TSoftClassPtr`, with a table of when to use each. The review checklist and [AGENTS.md](../AGENTS.md) rule 5 match.

Section 6 says every `UObject*` member must be a `UPROPERTY()` holding `TObjectPtr<>`. Section 7 separately recommends `TWeakObjectPtr` for non-owning references and `TSoftObjectPtr`/`TSoftClassPtr` for deferred assets. These are different reference semantics, not interchangeable spelling choices.

**Why it matters:** `TObjectPtr` in a reflected property is a strong GC-visible reference. Applying it to a reference that should be weak can keep an object reachable longer than intended. Conversely, a soft asset reference has different loading behavior and should not be converted to a hard reference merely to satisfy the blanket rule.

**Recommended change:** Say that strong UObject references held by a UObject and intended to participate in GC reachability should normally be `UPROPERTY() TObjectPtr<T>`. Explicitly retain weak and soft references where their semantics are needed, with reflection annotations when appropriate. Describe ownership/reachability intent, not just pointer syntax.

### C-03 — CQTest is prescribed, but the project setup is still marked pending

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 13 now calls CQTest the chosen framework with setup pending, states what is missing from `FarmSim.Build.cs` and `FarmSim.uproject`, and requires the first test's commit to record the verified setup and command and tick the roadmap item. [AGENTS.md](../AGENTS.md) rule 13 says the same.

Section 13 says to use CQTest for all new tests and gives setup instructions. The [Roadmap.md](Roadmap.md) checklist still marks “Set up CQTest in the project” as incomplete; the current [FarmSim.Build.cs](../Source/FarmSim/FarmSim.Build.cs) has no CQTest dependency, and [FarmSim.uproject](../FarmSim.uproject) does not enable the CQTest plugin.

**Why it matters:** The guide promises a test workflow that may not be available in a clean checkout. “Enable the plugin if the editor reports the module as missing” also leaves the required configuration dependent on an error rather than stating a verified setup.

**Recommended change:** Until setup and a sample test are verified, describe CQTest as the selected framework with setup pending. Once configured, record the exact plugin/module requirements and a command that has been run successfully. Keep the roadmap and guide synchronized.

### C-04 — Error feedback guidance can be read as coupling gameplay to the HUD

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 10 now requires player-facing failures to reach the player through a presentation-neutral feedback event that the HUD listens to; game code logs and returns or broadcasts the failure and never calls the HUD. The direct `AFarmHUD::ShowMessage` calls are named as a known gap, not a pattern. The pointer to `ShowMessage` as the feedback route was removed.

Section 10 says failures the player should know about, such as a failed save, “are reported on the HUD.” [Architecture.md](Architecture.md) says game rules must not depend on presentation and already lists direct `ShowMessage` calls as a known gap.

**Why it matters:** The coding guide could be read as authorizing exactly the dependency that the architecture document says should be removed. This also limits reuse of the save/gameplay code in contexts without the current HUD.

**Recommended change:** Require the failure to be reported to the player, but route it through a presentation-neutral event or feedback service. Let the HUD subscribe and display it; the save subsystem should log or return the failure independently of any widget.

### C-05 — The source-folder migration prescribes `PublicIncludePaths` too broadly

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 14.1 now triggers the folder split on navigation pain rather than a file count, uses includes relative to the module root (`Farming/FarmTile.h`, which the existing `"FarmSim"` include path already supports) and says not to add feature folders to `PublicIncludePaths`; a `Public/`/`Private/` split is reserved for headers another module actually needs.

Section 14.1 says to add feature folders to `PublicIncludePaths` after the source is split into folders. Feature-folder paths are not automatically public API. Within a module, includes can use module-relative paths, and only headers intended for consumers of other modules should be exposed as public includes.

**Why it matters:** Publishing every feature directory can make private headers easy to include across module boundaries, weaken encapsulation and increase accidental compile-time coupling. The arbitrary “about 20 files” trigger also makes a structural change depend on file count rather than navigation pain or ownership boundaries.

**Recommended change:** Make folder organization a response to discoverability and feature boundaries, not a fixed file threshold. Use explicit module-relative includes or the appropriate private include paths for implementation-only headers. Expose only deliberate public headers to other modules.

### C-06 — “Use UE types, not the standard library” is broader than the stated reasons justify

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 9 now makes UE types the default and requires them at reflection, save and engine API boundaries, while allowing standard-library facilities in private `.cpp` implementation code. `FMath` is the normal choice for math rather than a ban on `<cmath>`.

Section 9 rules out `std::` containers categorically and says to use `FMath` instead of `<cmath>`. Unreal types are the right default for reflected properties, engine APIs, serialization and allocator-aware engine containers, but standard C++ library facilities remain useful and valid in non-reflected implementation details. The guide does not distinguish those cases.

**Why it matters:** An absolute ban may lead to inferior or less familiar implementations, or developers may assume standard-library utilities are unsupported by the toolchain. It also makes the guide more restrictive than necessary without identifying a concrete project risk.

**Recommended change:** Prefer Unreal types at Unreal API/reflection boundaries and for data that integrates with engine containers or serialization. Permit standard-library facilities in private implementation code when they are appropriate and supported by the project's compiler settings. Keep `FMath` as the normal choice for engine math types, not as a blanket prohibition on `<cmath>`.

### C-07 — The synchronous asset-load prohibition is too absolute

**Severity: Low**

**Status: Resolved (2026-10-28).** Section 12 now names the concern (game-thread hitches), asks for async loading of large or slow assets during interactive play, and allows synchronous loads during startup or behind a loading screen.

Section 12 says not to load synchronously during gameplay. Avoiding blocking loads during active play is a good performance rule, but “during gameplay” can include a deliberate loading screen or initialization phase where synchronous loading may be an acceptable, bounded trade-off.

**Why it matters:** Read literally, the rule can force unnecessary async-loading machinery even when the player is not interacting and the load is intentionally blocking. It also does not state the actual concern: game-thread stalls and hitching.

**Recommended change:** Prohibit unbounded or latency-sensitive synchronous loads during interactive play. Prefer async loading for assets that may be large or unpredictably slow; allow explicit synchronous loads in controlled startup/loading flows when their cost and behavior are understood.

### C-08 — Several numeric rules are presented as universal thresholds

**Severity: Low**

**Status: Resolved (2026-10-28).** Sections 8 and 14.1 now treat function length and file count as review prompts: split a function when it does more than one thing or is hard to follow, and reorganize folders when related code becomes hard to find.

Sections 8 and 14 prescribe a split at about 40 function lines and a folder restructure at about 20 project-owned files. These can be useful prompts, but neither number is a reliable quality threshold. A 41-line cohesive function may be clearer than a fragmented one, and 20 files may still be easy to navigate.

**Why it matters:** Numeric targets can encourage mechanical refactoring or create churn without improving comprehension. The guide's stated goal is readability, so the criteria should be based on responsibility and discoverability.

**Recommended change:** Retain these as review prompts rather than pass/fail rules: split when a function has multiple responsibilities or is hard to understand; organize folders when finding and changing related code becomes difficult.

## Code review findings (2026-10-29)

Findings from checking the code against the guide. Design-level findings from the same review are in [ArchitectureAudit.md](ArchitectureAudit.md) (A-10 onward). Problems already listed in the guide's [known deviations](CodingGuidelines.md#16-known-deviations-to-clean-up) or the architecture's [known gaps](Architecture.md#17-known-gaps) are not repeated as findings; their status is in [Status of earlier findings and known deviations](#status-of-earlier-findings-and-known-deviations-2026-10-29).

### C-09 — The HUD searches the level every frame

**Severity: Medium**

**Status: Resolved (2026-10-29), 7105c61 and 812ea57.** The HUD keeps weak references, updated on possession, and builds its texts from `OnToolChanged`, `OnInventoryChanged` and `OnFeedback`. The bound getters only read finished text; the clock line is rebuilt only when the shown minute changes.

`SFarmHudWidget` binds its texts as Slate attributes, which Slate evaluates every frame. `GetStatusText` calls `AFarmHUD::GetDayCycle`, which calls `AFarmDayCycle::Find`, a `TActorIterator` search of the world. `GetStatusText`, `GetInventoryText` and `GetControlsText` each call `AFarmHUD::GetFarming`, which runs `FindComponentByClass` on the pawn. Section 12 says not to search the level every frame.

**Why it matters:** The cost is small today, but the HUD is the pattern the next widgets (inventory, shop, relationships) will copy, and each one would add its own per-frame searches. It also means the HUD silently shows nothing if the day cycle is missing, rather than noticing once.

**Recommended change:** Let `AFarmHUD` find the day cycle and the farming component once (at `BeginPlay`, and again when the controller possesses a new pawn) and keep them in `TWeakObjectPtr`s that the widget reads. Better still, as the HUD grows, have the widget update its texts from events (`OnNewDay`, `OnToolChanged`, an inventory change event) instead of polling. Bound attributes are fine for values that really change every frame, such as the clock.

### C-10 — Player-facing text is `FString`, not `FText`

**Severity: Medium**

**Status: Resolved (2026-10-29), c87081e, 6805d86 and 812ea57.** Every player-facing string is `LOCTEXT`, sentences are `FText::FormatNamed` with named arguments, messages arrive one per line through `UFarmFeedbackSubsystem` instead of `AppendMessage`, and the clock and counts use `FText::AsNumber`.

Section 9 says `FText` is for anything the player reads. All feedback messages are `TEXT("...")` literals passed as `FString` (`UFarmingComponent::UseTool`, `UFarmSaveSubsystem::SaveFarm` and `LoadFarm`), `AFarmHUD::ShowMessage` and `AppendMessage` take and store `FString`, and `SFarmHudWidget` builds its texts with `FString::Printf` and wraps them in `FText::FromString`. The tool's localizable `DisplayName` is turned back into a string with `ToString()` before display.

**Why it matters:** None of this text can be gathered for translation, and the 24-hour clock and number formats can't follow the player's culture. Converting later means touching every message, and `AppendMessage` concatenates sentences, which doesn't translate (word order differs between languages).

**Recommended change:** Do it together with the feedback event from the [known gaps](Architecture.md#17-known-gaps): the event carries an `FText`, messages are written with `LOCTEXT`/`NSLOCTEXT` and `FText::Format`, and the HUD formats its lines with `FText::Format` and `FText::AsNumber`. Replace `AppendMessage` with a list of messages the HUD shows one after another, or one message per event.

### C-11 — The code isn't clean under the repo's `.clang-format`, and the formatter disagrees with the written rules

**Severity: Medium**

**Status: Resolved (2026-10-29), c1b0a9e and f1e2343.** The reference formatter is the clang-format 22.1 that ships with Visual Studio. The Slate blocks have off/on markers, `.clang-format` knows CQTest's `TEST_CLASS`, the nested ternary is gone, and every one of our files is clean under the formatter.

Running clang-format 18 with the repo's `.clang-format` changes eight of our files: `FarmHudWidget.cpp` (the Slate layout, 104 lines), `FarmingComponent.cpp`, `FarmToolbar.cpp`, `FarmTile.cpp`, `FarmDayCycle.cpp`, `FarmerCharacter.cpp`, `FarmHudWidget.h` and `Tests/FarmTileTests.cpp`. Three lines are over the 140-character limit (`FarmingComponent.h:85`, `FarmTile.cpp:38`, `FarmerCharacter.cpp:75`). Some of the differences are the formatter fighting the guide:

- Section 3 points to the nested ternary in `AFarmTile::RefreshSoil` as the model for breaking a ternary. The formatter re-aligns it with runs of spaces, against the tabs rule.
- The formatter re-indents lambda bodies (`FFarmToolbar::SelectTool`, the timer lambda in `UFarmingComponent::BeginPlay`) differently from how they are written.
- `FarmHudWidget.h` still lacks the `// clang-format off` / `on` the guide asks for around the Slate arguments, and `FarmHudWidget.cpp` has no such markers around the `ChildSlot` layout, which the formatter flattens.

Visual Studio bundles its own clang-format version, so results may differ slightly from version 18, but the over-long lines and the Slate layout differ under any version.

**Why it matters:** Rule 4 in `AGENTS.md` says to format changed files. While the files aren't already clean, formatting one for a small change rewrites unrelated lines and mixes a reformat into a behavior commit, which section 3 forbids.

**Recommended change:** Record which clang-format version the project uses (the one Visual Studio ships). Wrap the Slate layout in `FarmHudWidget.cpp` and the arguments in `FarmHudWidget.h` in `// clang-format off` / `on`. Replace the nested ternary in `RefreshSoil` with an `if`/`else` (clearer anyway), or pick a different example in section 3. Then format all of our files once in a formatting-only commit, so later changes only show their own lines.

### C-12 — Magic numbers remain, and some values are defined twice

**Severity: Low**

**Status: Resolved (2026-10-29), 13964d9, 812ea57 and 9cdf0ac.** The listed literals are named constants, `TimeOfDay` starts from `DayStartHour`, the save no longer repeats the start hour, and the key hints come from the bindings.

Section 8 asks for named constants. Unnamed values remain mostly in visual code:

- `AFarmTile::RefreshCrop`: the placeholder crop's height formula (`0.15f + 0.25f * GrowthDays`), width (`0.3f`) and half height (`50.f`); `TEXT("Color")` is written twice as a material parameter name.
- `AFarmDayCycle::UpdateLighting`: the lowest sun elevation (`5.f`), the fade rates (`3.f`, `2.f`, `6.f`), `180.f` and `24.f`.
- `AFarmGrid`: the engine shapes' 100-unit size (`TileSize / 100.f`, also assumed in `FarmTile.cpp`) and the bounds box height (`10.f`).
- `AFarmerCharacter`: the `hand_r` socket name and the mapping context priority `1`.
- `SFarmHudWidget`: font sizes and paddings, while the colors already have a `FarmHudStyle` namespace.

Some values are defined twice: `AFarmDayCycle::TimeOfDay` and `UFarmSaveGame::TimeOfDay` both start at `6.f`, repeating `DayStartHour`; the HUD's `"E Use tool | N Sleep | F9 New game"` repeats the keys bound in `AFarmerCharacter::SetupPlayerInputComponent`.

**Why it matters:** Tuning the look means hunting for literals, and a key or start-hour change has to be made in two places; missing one shows the player a wrong hint.

**Recommended change:** Move the visual numbers into the existing `FarmTilePlaceholder` and `FarmHudStyle` namespaces and a new `FarmDayCycleLighting` namespace, or into tunable properties where a designer would want them. Initialize `TimeOfDay` from `DayStartHour`. For the key hints, see [A-14](ArchitectureAudit.md#a-14--key-hints-repeat-the-key-bindings).

### C-13 — Header layout and API comments miss a few rules

**Severity: Low**

**Status: Resolved (2026-10-29), 13964d9, 4e3bac0 and c87081e.** The overrides are protected, each tile action documents what `false` means (now on `FFarmTileState`), the missing comments are written, "level" reads "world", and the farming component's comment no longer describes calling the HUD.

- Section 5.2 keeps engine overrides `protected` unless outside callers need them. `AFarmDayCycle::Tick`, `UFarmingComponent::TickComponent` and `AFarmGrid::OnConstruction` are public.
- Section 2.3 says an action that can fail documents what `false` means. `AFarmTile` covers `Till`, `Water` and `Harvest` with one `//` comment (not `/** */`, so IDE tooltips don't show it), and `UFarmingComponent::UseTool` and `Sleep` have no comment.
- `EFarmTileState`, `FFarmTileSaveData`, the day cycle's `SunriseHour`, `SunsetHour`, `SunriseColor` and `MoonColor`, and the grid's `Width` and `Height` have no `/** */` comment.
- Header comments still say "level" where [Architecture.md](Architecture.md) now says "world": `AFarmDayCycle::Get`/`Find` ("the level's day cycle") and `UFarmSaveSubsystem` ("a fresh one comes with every level load").
- `UFarmingComponent`'s class comment says it "reports what happened on the player's AFarmHUD", describing the known gap as its job.

**Why it matters:** Small inconsistencies, but the header comments are what a returning developer and the planned mod API read first.

**Recommended change:** Move the three overrides to `protected`, give each tile action its own `/** */` saying what `false` means, and correct the wording when the files are next touched. Drop the HUD sentence from `UFarmingComponent`'s comment when the feedback event replaces the calls.

### C-14 — Bindings and spawned actors aren't cleaned up

**Severity: Low**

**Status: Resolved (2026-10-29), 758f81c, 9d77ee1, a04b349, 6805d86 and 9cdf0ac.** Every binding is removed in `EndPlay` or `Deinitialize`, the grid destroys its tiles when it alone ends, and the farmer creates its input once and removes its mapping context on `UnPossessed`.

[Architecture.md, section 4](Architecture.md#4-how-systems-talk-to-each-other) says to unbind in `EndPlay` / `Deinitialize`. Nothing does: `AFarmGrid` binds `OnNewDay`, `UFarmSaveSubsystem` binds `OnNewDayFinished` and `AFarmerCharacter` binds `OnToolChanged`, and none of them has an `EndPlay` or `Deinitialize` that removes the binding. `AFarmGrid` spawns its tiles but doesn't destroy them in `EndPlay`. `AFarmerCharacter::SetupPlayerInputComponent` creates new input actions and adds the mapping context again each time it runs (each possession) without removing the old context.

**Why it matters:** `AddUObject` makes a stale binding harmless today, and with one world and no streaming the tiles die with the world. These become real bugs as soon as the grid can be streamed out or replaced, or the farmer is possessed again (respawn, cutscene): orphaned tiles stay in the world, and the input mapping is added twice.

**Recommended change:** Keep the `FDelegateHandle` from each `AddUObject` and remove it in `EndPlay` / `Deinitialize` (or call `RemoveAll(this)`). Destroy the grid's tiles in `AFarmGrid::EndPlay`. In the farmer, create the input actions once and remove the mapping context in `PawnClientRestart`/`UnPossessed`, or move the context to an `IMC_` asset when input moves to assets.

### C-15 — Includes, module dependencies and small hygiene

**Severity: Low**

**Status: Resolved (2026-10-29), f1e2343, fba8700, 4e3bac0 and 13964d9.** `FarmerCharacter.h` forward declares, the include block is fixed, the UI modules are private dependencies, style names use `TEXT`, and every file ends with a newline.

- `FarmerCharacter.h` includes `FarmingComponent.h` though a forward declaration is enough for a `TObjectPtr<UFarmingComponent>` member (section 5.3).
- `FarmHudWidget.cpp` has a stray blank line splitting its alphabetical include block.
- `Vestige.Build.cs` lists `Slate`, `SlateCore` and `UMG` as public dependencies, though no public header exposes their types (`SFarmHudWidget` is used only inside the module); section 5.3 prefers private dependencies. It also keeps the template's commented-out lines.
- Slate style lookups use plain string literals (`"WhiteBrush"`, `"Bold"`, `"Regular"`) instead of `TEXT(...)` (section 9).
- `FarmHudWidget.cpp`, `Vestige.h` and `Vestige.cpp` have no final newline.

**Why it matters:** Each is minor; together they make the next clang-format or IWYU pass noisier than it needs to be.

**Recommended change:** Fix them in the formatting-only commit from C-11, or when each file is next touched.

### C-16 — The template classes don't follow the guide, and the guide doesn't say whether they should

**Severity: Low**

**Status: Resolved (2026-10-29), f1e2343 and the decision log.** Section 2.2 says the template files keep Epic's style, aren't maintained to the guide and are deleted when unused.

`AVestigeCharacter` and `AVestigePlayerController` are Epic's template code. They use raw `UObject*` members in `UPROPERTY`s (`UInputAction* JumpAction`, `TArray<UInputMappingContext*>`), `} else {`, multiple `public:`/`protected:` sections, a second log category (`LogTemplateCharacter`) and a 247-character line in `VestigeCharacter.cpp`. Section 2.2 only says they keep their names.

**Why it matters:** An agent following the hard rules literally would "fix" these files in passing and mix Epic's code with ours, or copy their style into new code because it compiles.

**Recommended change:** Say in section 2.2 or 16 that the template files keep Epic's style until we change them for a reason, and that a file we change substantially is brought up to the guide (raw `UPROPERTY` pointers become `TObjectPtr`, its log calls use `LogVestige`) in its own commit.

### C-17 — Tests cover the tile and toolbar, not the rest of the rules

**Severity: Medium**

**Status: Resolved (2026-10-29).** The tests went from 13 to 58: the day clock, the rest component, targeting and the highlight, feedback, key hints, saving (load decision, upgrade, round trips by coordinates, unknown content), content IDs, tile refusals on `FFarmTileState`, the farming component's rules on the real farmer Blueprint, and the rendering settings. Tool IDs are set with `SetContentIdForTests` instead of reflection by name.

CQTest works and the 13 tests are well named and namespaced as section 13 asks. Section 13's "what to test" list is only partly covered:

- **Tile:** refusals are untested (tilling non-grass, watering grass, harvesting an unripe crop, planting twice), as is `RestoreState` clamping the growth days.
- **Farming component:** nothing. Planting uses a seed, an empty seed count refuses, harvesting adds the crop's yield, an empty slot does nothing.
- **Day clock:** nothing. `StartNextDay` advances the day and resets the clock, `IsPastBedtime`, and `RestoreClock` clamping without broadcasting.
- **Saving:** only the old-tool mapping. No save and load round trip.

`FarmToolbarTests` sets the private `ContentId` with `FindFProperty` by its string name, so renaming the property breaks the test at run time, not at compile time. The roadmap's "Tile rule tests" item is still unticked although five tile tests exist.

**Why it matters:** The farming component and the clock carry the rules milestone 2 is about to change (stamina, late nights, seasons). They are the riskiest code to change without tests.

**Recommended change:** Add the missing tile refusal tests and tick the roadmap item. Test the clock with a spawned `AFarmDayCycle` (no lights needed: `UpdateLighting` returns early without a sun). For the farming component, either spawn a grid and farmer in the test world or pull its rules into a testable struct like `FFarmToolbar`. Give `UFarmContentData` a test-only way to set the ID (a `protected` setter used by a test subclass, or `#if WITH_DEV_AUTOMATION_TESTS` friend) instead of reflection by name.

### C-18 — Config and content leftovers from the template

**Severity: Low**

**Status: Resolved where it is safe without the editor (2026-10-29), 231c01a.** The ThirdPersonCPP settings, duplicate keys, the Android File Server token and the VR axis mappings are gone. Re-saving the template Blueprints and `L_Farm` and moving `BP_FarmTile` and the materials need the editor; they are housekeeping items in the roadmap with the exact steps.

- `DefaultEditor.ini` (`SimpleMapName`) and `DefaultEditorPerProjectUserSettings.ini` point at `/Game/ThirdPersonCPP`, which doesn't exist.
- `DefaultEngine.ini` repeats `ExtendDefaultLuminanceRange` and `DefaultGraphicsRHI`, and keeps an Android File Server `SecurityToken` (only used to connect to a USB Android device, and excluded from shipping, but worth regenerating or removing before the repository goes public).
- `DefaultInput.ini` keeps the template's legacy axis mappings for VR controllers and `bEnableMotionControls=True`, which the game doesn't use.
- `BP_ThirdPersonGameMode` and `BP_ThirdPersonPlayerController` are still saved against `/Script/TP_ThirdPerson` and load only through the `ActiveClassRedirects` in `DefaultEngine.ini`.
- Section 14.2 says our content is split by type. `BP_FarmTile` sits at the root of `Content/Farm/`, and the `M_` materials live in `Content/Farm/Meshes/`.

**Why it matters:** None of this breaks the game. It is noise for a new contributor, and the redirects can never be removed while assets still depend on them.

**Recommended change:** In one housekeeping commit: remove the dead `ThirdPersonCPP` settings and duplicate keys, regenerate or remove the token, and trim the unused axis mappings. Open and resave the two template Blueprints so they reference `/Script/Vestige`. Move `BP_FarmTile` to `Content/Farm/Blueprints/` and the materials to `Content/Farm/Materials/` in the editor (so references are fixed up), then fix up redirectors.

## Status of earlier findings and known deviations (2026-10-29)

*This section is the state at the code review. The state after the clean-up is in [Re-audit after the clean-up](#re-audit-after-the-clean-up-2026-10-29).*

C-01 to C-08 were documentation findings and stay resolved; the guide still says what their status paragraphs describe. Two follow-ups:

- **C-03:** CQTest is now set up as section 13 describes (developer-tools-only dependency, no plugin), the first tests run, and the roadmap item is ticked. The finding is fully closed.
- **C-04:** the guide is right, but the code it names as a known gap is unchanged: `UFarmingComponent` and `UFarmSaveSubsystem` still call `AFarmHUD::ShowMessage` and `AppendMessage`.

The guide's [known deviations](CodingGuidelines.md#16-known-deviations-to-clean-up), checked against the code: (at the time of the code review; all three are fixed now, see below)

| Deviation | Status |
|---|---|
| `UFarmingComponent::Seeds` and `HarvestedCrops` are public and written by the save subsystem | **Open.** Also read directly by `SFarmHudWidget::GetInventoryText`. |
| Tunables on `AFarmTile`, `AFarmGrid` and `AFarmDayCycle` are public | **Open.** The same holds for `UFarmingComponent::ReachDistance` and `AFarmHUD::MessageDuration`, which the list doesn't name. The newer classes (`UFarmContentData`, `UFarmCropData`, `UFarmToolData`) follow the rule. |
| `FarmHudWidget.h` needs `// clang-format off` / `on` | **Open.** See C-11. |

## Positive foundations to preserve

- The ownership guidance and links to the architecture document make code placement a deliberate decision rather than a convenience.
- The warnings about UObject lifetime, `IsValid`, safe delegate binding and not capturing `this` in long-lived lambdas address common Unreal failure modes.
- The IWYU and forward-declaration guidance supports faster, more reliable builds when applied with Unreal Header Tool requirements in mind.
- The guide treats game-rule tests and bug-regression tests as normal work, rather than optional polish.
- The error-handling section distinguishes programmer invariants (`check`/`ensure`) from recoverable runtime conditions and designer mistakes.
- The review checklist and known-deviation list are practical tools for a small project, provided they stay synchronized with the actual build and architecture.

Seen in the code on 2026-10-29:

- `FFarmToolbar` keeps rules in a plain struct the component owns, so they're tested without a level. It is the model for the tile-state struct and for testing the farming component.
- `UFarmContentData` puts the content ID, primary asset ID and bundle loading in one place, and the newest classes (`UFarmContentData`, `UFarmCropData`, `UFarmToolData`) follow the encapsulation rule with private `AllowPrivateAccess` tunables and getters.
- Content mistakes degrade with a clear warning (a missing crop or tool asset, an empty toolbar, a restored planted tile without a crop) instead of crashing.
- Lambdas that can outlive their object capture a `TWeakObjectPtr`, as section 9 asks.
- Every one of our source files carries the SPDX header, and every one of our classes has a class comment.

## Recommended correction order

1. [x] Clarify strong, weak and soft UObject reference rules (C-02; resolved 2026-10-28).
2. [x] Resolve the bool naming contradiction and HUD feedback dependency (C-01, C-04; resolved 2026-10-28).
3. [x] Reconcile CQTest documentation with the actual project configuration (C-03; resolved 2026-10-28). The roadmap item stays open until the first test runs.
4. [x] Tighten module include-path guidance before source folders are reorganized (C-05; resolved 2026-10-28).
5. [x] Qualify the standard-library and synchronous-load rules; soften numeric thresholds (C-06 through C-08; resolved 2026-10-28).

From the 2026-10-29 code review:

6. [x] Make the code clean under `.clang-format` in one formatting-only commit, after settling the formatter version and the ternary and Slate exceptions (C-11, C-15).
7. [x] Add the missing tile, clock and farming tests, and tick the roadmap's tile test item (C-17).
8. [x] Cache the HUD's references instead of searching every frame (C-09).
9. [x] Move player-facing text to `FText` together with the feedback event (C-10).
10. [x] Clean up bindings and spawned tiles in `EndPlay` / `Deinitialize` (C-14).
11. [x] Name the remaining magic numbers, fix the header details and decide the template code rule (C-12, C-13, C-16).
12. [x] Clear the template leftovers in config and content (C-18). The editor-only steps are in the roadmap.

## Re-audit after the clean-up (2026-10-29)

Every code review finding above was worked through, then all of `Source/Vestige` was checked again against the guide and its [review checklist](CodingGuidelines.md#17-review-checklist): formatting under the reference clang-format, includes (a non-unity build without precompiled headers compiles every file on its own), `UObject` pointer types, encapsulation, magic numbers, ticking and per-frame work, trace scopes, comments and SPDX headers. The editor target, the non-unity build and the Shipping game target build without warnings, and the 58 `Vestige` tests pass.

**Code conformance: 9/10.** The guide's [known deviations](CodingGuidelines.md#16-known-deviations-to-clean-up) list is empty: the counts became private with a save struct (812ea57), the tunables are private (a04b349, with `ReachDistance` in 812ea57 and the HUD's settings in c87081e), and the Slate markers are in (f1e2343). The last fix of the re-audit turned off overlap events on meshes nothing overlaps (6c45ece).

Left as they are, on purpose, and recorded in [Architecture.md, decision log](Architecture.md#16-decision-log) or the [architecture re-audit](ArchitectureAudit.md#re-audit-after-the-clean-up-2026-10-29): `UFarmSaveGame` keeps public members because it is a plain data container; crops and the held tool keep their shadows; the day cycle searches for its lights once in `BeginPlay`; and Epic's template files aren't brought up to the guide.

## Review limits

The first review (C-01 to C-08) is a documentation audit, not a compiler or static-analysis audit. Project configuration was checked for the CQTest setup claim, and the architecture document was checked for the HUD dependency rule. No C++ build, tests or formatter were run because no code changed. Those findings assess the coding guide as guidance and do not claim that every rule is violated in the implementation.

The code review (C-09 onward) read every file in `Source/` and `Config/` on `main` at commit `d9f10d7` (2026-10-28, tools as data assets). @DunderGG's PC wasn't reachable, so uncommitted local work (the tool data assets themselves) wasn't seen. clang-format 18.1 was run with the repo's `.clang-format`; Visual Studio's bundled version may differ in details. Blueprints and data assets were checked only through the asset and class references stored in the `.uasset` files, not opened in the editor, so their graphs weren't reviewed. Nothing was compiled and no tests were run: the cloud environment has no Unreal Engine.
