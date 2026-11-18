# Architecture document audit

**Reviewed:** 2026-10-26 (the document), 2026-10-29 (the code against the document, then again after the clean-up)  
**Scope:** A critical review of [Architecture.md](Architecture.md) as guidance for a C++ / Unreal Engine 5.7 game project. The first review (A-01 to A-09) assesses the document's accuracy, completeness, internal consistency and usefulness as an implementation guide, and does not check the code. The second review (A-10 onward, [Code review findings](#code-review-findings-2026-10-29)) checks the code in `Source/Vestige/`, `Config/` and the project's Blueprints against the document's design and Unreal practice. Code-style findings from the same review are in [CodingGuidelinesAudit.md](CodingGuidelinesAudit.md) (C-09 onward).

## Executive assessment

**Overall: 6/10.** The document has a strong foundation for a small single-player prototype: ownership is explicit, lifecycle hazards are called out, presentation is meant to stay separate from game rules, and known gaps are not hidden. Those are meaningful strengths.

The score is held back by guidance that is too absolute or underspecified in areas where mistakes are costly: Unreal world and level lifetimes, save ownership and recovery, content identity, and the claims around future mod support. Some statements could lead a future contributor to implement behavior that the document itself does not intend. The architecture is therefore a useful direction-setting document, but it should not yet be treated as a reliable contract for persistence or mods.

Scores use a 1–10 scale: 1 means misleading or absent; 5 means usable but has material gaps; 10 means accurate, specific and actionable for this project's scope.

| Area | Score | Assessment |
|---|---:|---|
| Ownership and separation of concerns | 8/10 | Clear responsibilities and a useful ownership table. Some class/layer rules are stated as universal laws rather than design tests. |
| Unreal framework and lifecycle accuracy | 5/10 | Good warnings about initialization order, but “level” and “world” are conflated and actor auto-creation needs tighter lifecycle constraints. |
| Dependencies and event flow | 7/10 | Good direction-of-dependency rules. The day-completion guarantee needs a synchronous-work boundary. |
| Persistence and save safety | 4/10 | Good intent around owner-produced save data and versioning, but ownership, failure recovery and data-loss behavior are not sufficiently specified. |
| Content identity and mod extensibility | 5/10 | Data-driven direction is sound; stable IDs and plugin-based mod support are presented more confidently than the defined mechanisms justify. |
| Testability and verification | 5/10 | Testing principles exist in the coding guide, but the architecture decision log and roadmap disagree about whether CQTest is set up. |
| Performance and growth guidance | 6/10 | Event-driven defaults and measurement are good; the actor-count transition example is an arbitrary threshold and an overly direct optimization prescription. |
| Decision traceability and known gaps | 8/10 | The decision log and gap list are unusually useful. They would benefit from priority, risk and completion criteria. |

### Code conformance (2026-10-29)

**Overall: 6/10.** The code follows the ownership table closely, and the newest work is the best example of the design: crops and tools are discovered data assets with typed-in IDs and soft-loaded visuals, the toolbar's rules sit in a testable struct, and old saves keep loading. The score is held back by the known gaps, which are all still open (gameplay calls the HUD, the save subsystem writes other classes' state, no save version, an unreadable save gets overwritten), and by a few problems the gap list doesn't name: the tile highlight is debug drawing that disappears from shipping builds, bedtime is polled every frame, and the farming component is collecting jobs.

| Area | Score | Assessment |
|---|---:|---|
| Ownership and separation of concerns | 7/10 | Matches the ownership table. The farming component also draws the target highlight and checks the clock every frame, which aren't its jobs. |
| Dependency direction | 5/10 | Gameplay and saving call the HUD (known gap), and the save data header includes the tile actor. |
| Events instead of polling | 6/10 | New days are event driven; bedtime and the HUD are polled every frame. |
| Lifetimes and initialization | 6/10 | `Get`/`Find` and the one-tick load wait are known gaps; nothing unbinds in `EndPlay`. |
| Saving | 4/10 | Works for the prototype, but none of section 6's safety rules are implemented yet; all are tracked as known gaps and roadmap items. |
| Content and mod readiness | 7/10 | Crops and tools follow section 12. ID validation, duplicates and extension points are still missing (known gaps). |

## Findings

### A-01 — Unreal world lifetime is described as level lifetime

**Severity: High**

**Status: Resolved (2026-10-28).** Sections 2 and 3 now use world lifetimes throughout, explain the difference between streaming a sublevel, opening a map and seamless travel, describe the game mode as authoritative and server-only in multiplayer, and state the current assumption: one map, no streaming, `OpenLevel` only for a new game.

Sections 2 and 3 repeatedly describe framework objects and `UWorldSubsystem` as lasting “one level,” then say level loads destroy the world. Unreal's lifecycle boundary is generally the `UWorld`, not an individual streamed level. Streaming a level does not, by itself, imply that the world and its subsystems are recreated; map travel and seamless travel have different lifecycle behavior. `AGameModeBase` is also authoritative game-mode state, not a client-visible world service.

**Why it matters:** A reader could attach state to the wrong lifetime, incorrectly assume a subsystem will be recreated, or lose state during travel. This becomes especially risky once the game uses streamed levels, transitions, or separate menus.

**Recommended change:** Use “world” consistently for world-scoped actors and subsystems. Distinguish streamed-level changes from world/map travel, and state which travel mode the project currently assumes. Keep the existing warning that world `BeginPlay` ordering is not guaranteed.

### A-02 — World-scoped save coordination has no stated travel boundary

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 6 and the ownership table state the current scope (one world, one save slot, a new game deletes the slot and reopens the map) and the split to make before save slots, menus or travel: slot choice in a game-instance subsystem, the world snapshot in the world subsystem. Logged as a decision and an expected change.

**Section 13** assigns saving and loading to `UFarmSaveSubsystem`, a `UWorldSubsystem`. Section 16 explicitly includes deleting the save as part of “delete save, reload level”; the concern is not that the document claims reloading alone deletes persistent data. Rather, the document does not say whether this world-scoped service is only responsible for the current world's snapshot or also owns active-slot/profile selection, nor what happens if a save/load is in progress during world travel.

**Why it matters:** The current single-map prototype can reasonably keep this service world-scoped. If the game later adds map travel, menus or multiple slots, however, it will need an explicit boundary between world snapshot work and state that must survive world replacement.

**Recommended change:** State the current assumption (one world and one active save slot) and that “New Game” explicitly deletes or selects a different slot before loading. Before adding travel or multiple slots, separate world-scoped snapshot collection/application from persistent slot/profile ownership; put only the latter in a game-instance-lifetime owner if it must survive world replacement.

### A-03 — Save recovery rules risk hiding or overwriting user data

**Severity: High**

**Status: Resolved (2026-10-28).** Section 6 defines the outcome for an empty slot, a current, migratable, corrupt or newer save, and unknown content; an unreadable save is never overwritten without the player starting a new game; a failed write keeps the last good save; the mid-day loss window and synchronous saving are stated as accepted. Save tests are listed. The current code's overwrite of an unreadable save is recorded as a known gap and a roadmap item.

Section 6 says an old save should migrate “or fall back to a clean start with a message.” It does not distinguish a missing save, a corrupt save, an unsupported newer save, or a failed migration. It also does not say whether a fallback may later overwrite the original file. “Save at clear points” describes when the game saves, not what happens when the write fails or the game exits before the next day boundary.

**Why it matters:** Treating an unreadable or newer save as a clean start can turn a recoverable compatibility problem into permanent data loss. Saving only at day boundaries also means all progress since the previous boundary can be lost; that may be an intentional prototype trade-off, but it should be explicit.

**Recommended change:** Define observable outcomes for missing, valid, migratable, corrupt and newer-version saves. Preserve the original on failed load/migration, report errors, and do not overwrite it with a clean-start save without an explicit reset decision. Document the accepted loss window and whether saves are synchronous or asynchronous. Add round-trip, migration, failure and unknown-content tests to the save plan.

### A-04 — “Stable ID” has no rename or collision policy

**Severity: Medium**

**Status: Resolved (2026-10-28).** Sections 5 and 12 define one canonical persisted ID (a typed-in `ContentId`, `<Namespace>.<Name>`, that also builds the `FPrimaryAssetId`), its format and validation, a no-rename rule with redirects, duplicate and unknown ID handling, and the tests needed. Logged as a decision.

Sections 5 and 12 call `FPrimaryAssetId`, `FName` and gameplay tags stable identifiers and prescribe namespaced content IDs. These types are useful identifiers, but the document does not define which representation is canonical or how IDs survive asset renames, package changes, duplicate mod IDs, removed mods or conflicting content. An asset-derived ID is not automatically an immutable save identifier.

**Why it matters:** Saves and mod content may become unresolvable after a rename or package reorganization. A collision can make the same saved ID resolve differently depending on what content is loaded.

**Recommended change:** Pick one canonical persisted ID format and define its namespace and validation rules. Treat IDs separately from asset paths and display names. Document how renames are migrated or redirected, how duplicate IDs fail, and how unknown IDs are reported/skipped. Add tests for rename migration and duplicate/unknown IDs before relying on IDs in player saves.

### A-05 — Mod-friendly architecture is described too close to guaranteed mod support

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 12 now states "mod-oriented design, not mod support", explains that Unreal doesn't load plugins into packaged games on its own and that Game Features need deliberate activation, and adds a readiness checklist (discovery, scanning, activation, load order and ID conflicts, versions, saves without the mod, trust and consent, a documented API).

Section 12 calls content and Blueprint mods what the project supports, while also deferring loading mod plugins from a packaged build, load order, conflict handling and a mod kit. Asset Manager discovery and primary asset types are useful building blocks, but do not on their own make third-party plugin content discoverable, cooked, loadable or safe to activate. Game Feature plugins also require deliberate runtime activation and lifecycle design; they are not a general-purpose mod loader that works automatically.

**Why it matters:** Contributors may build against an implied mod API or promise compatibility before packaging, discovery, activation, dependency and save-compatibility behavior exists. Blueprint mods also execute trusted game logic, as the document correctly notes, so extension surfaces need an explicit trust boundary.

**Recommended change:** Describe the current status as **mod-oriented design, not supported mod loading**. Keep the data-driven rules as goals, and add a readiness checklist before claiming support: packaged plugin discovery/cooking, Asset Manager scan configuration, activation/deactivation, load order and ID conflicts, version compatibility, save behavior when a mod is absent, and user trust/consent.

### A-06 — The day-finished event is not a general completion barrier

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 15 states the synchronous-completion contract for `OnNewDay` listeners and the explicit completion barrier that replaces `OnNewDayFinished` if asynchronous day work is ever needed. Logged as a decision.

Section 15 says `OnNewDayFinished` ensures saving runs after every system has reacted, regardless of subscription order. That is true only for work completed synchronously by listeners during the preceding `OnNewDay` broadcast. A listener that starts a timer, latent action, async task or deferred spawn has not necessarily finished when that broadcast returns. The document does not define whether day listeners are required to complete synchronously.

**Why it matters:** If day advancement gains asynchronous work, the save can capture a partially advanced world while still appearing to honor the documented event order.

**Recommended change:** State the synchronous-completion contract explicitly if it is intentional. If any system must complete asynchronously, replace the second broadcast as a presumed barrier with an explicit completion coordinator/countdown or another mechanism that only signals completion after all required work reports done. Test the save snapshot against that contract.

### A-07 — The auto-spawn recommendation needs world and uniqueness constraints

**Severity: Medium**

**Status: Resolved (2026-10-28).** Section 2 limits find-or-spawn to required placed actors and sets rules: world-scoped and game-thread only, spawning only in running game worlds, `Get` as the only creator, warnings when it spawns or finds duplicates, and subsystems for services. The current `AFarmDayCycle::Get` is recorded as a known gap and a roadmap item.

Sections 2 and 3 recommend that a required placed actor find-or-spawn itself through a static `Get(World)` call. This can be a practical choice for `AFarmDayCycle`, which controls world actors and lighting, but the guidance does not specify when spawning is allowed, how duplicate instances are prevented, or how editor/preview worlds and teardown are handled.

**Why it matters:** A convenience lookup can silently turn a missing level dependency into an implicit runtime side effect. With streaming, editor previews or future multiple-world contexts, “find-or-spawn” can produce surprising ownership or duplicate actors if its world scope is not explicit.

**Recommended change:** Keep `Get` explicitly world-scoped and game-thread-only; reject invalid, editor-preview or tearing-down worlds as appropriate. Define one creation authority and a uniqueness rule. For required actors, also provide validation or a clear failure path so a bad level setup is not silently masked. Do not generalize this pattern to services that belong in a subsystem.

### A-08 — The CQTest decision is ahead of the project setup

**Severity: Medium**

**Status: Resolved (2026-10-28).** The decision log entry now reads "Adopt CQTest for game-rule tests; setup is pending" and points to the roadmap. The roadmap's save tests now include the tile save and restore ownership boundary.

Section 16 says “Tests use CQTest,” but the [Roadmap.md](Roadmap.md) Tests checklist still has “Set up CQTest in the project” unchecked. The [CodingGuidelines.md](CodingGuidelines.md) describes setup as work to do with the first test. This is a documentation-state mismatch, not a criticism of choosing CQTest.

**Why it matters:** The architecture decision log reads like a current implementation fact, while the roadmap says the framework is not yet installed/configured. Contributors may assume tests can be added and run now.

**Recommended change:** Rephrase the decision as “Adopt CQTest for game-rule tests; setup is pending,” then update the roadmap when the module/plugin dependency is actually configured and the first test runs. Include a test that exercises the key ownership boundary, such as tile state save/restore, not only isolated transitions.

### A-09 — The scale example prescribes a structural rewrite by actor count

**Severity: Low**

**Status: Resolved (2026-10-28).** Section 10 now keeps one actor per tile until profiling shows a cost, and presents grid-owned state arrays, instanced meshes and merged collision as options to weigh against the measured bottleneck, not a step triggered by tile count.

Section 10 contrasts an 8×8 field of tile actors with “fields of thousands” represented by an array of structs and instanced static meshes. The section also correctly says to measure before optimizing, but the specific threshold suggests actor count alone decides when to switch. Rendering, collision, ticking, component count, interaction frequency and target hardware all affect the trade-off; instanced meshes replace rendering representation, not automatically tile behavior or collision.

**Why it matters:** A premature rewrite could make tile rules and interaction harder to maintain without fixing the actual bottleneck.

**Recommended change:** Keep the current actor design until profiling demonstrates a bottleneck. Present data-oriented storage and instancing as options to evaluate against measured CPU, memory, collision and rendering costs, not as a threshold-based next step.

## Code review findings (2026-10-29)

Findings from checking the code against this document. Problems already listed in [Known gaps](Architecture.md#17-known-gaps) aren't repeated here; their status is in [Status of earlier findings and known gaps](#status-of-earlier-findings-and-known-gaps-2026-10-29).

### A-10 — The tile highlight is debug drawing in the farming component

**Severity: High**

**Status: Resolved (2026-10-29), 51819b4.** `UFarmingComponent` follows its owner's `TransformUpdated` and broadcasts `OnTargetTileChanged`; a new `UFarmTargetHighlightComponent` frames the tile with an instanced mesh that ships. The component no longer ticks.

The roadmap lists "Highlight on the tile the player is facing" as done. It is drawn by `UFarmingComponent::TickComponent`, which calls `DrawDebugBox` around the target tile every frame. Unreal compiles debug drawing out of shipping builds (`ENABLE_DRAW_DEBUG` is off there), so the highlight silently disappears from a packaged game. It also puts presentation in a game-rules component, against [section 7](Architecture.md#7-presentation-ui-audio-and-effects), and it is the reason the component ticks at all.

**Why it matters:** A finished feature will be missing from the first shipping build, with no error to point at it. Leaving it where it is also invites the next cosmetic (a tool preview, a "can't do that" flash) into the same tick.

**Recommended change:** Keep target finding in `UFarmingComponent`, which owns targeting: it exposes the target tile and broadcasts a change event (for example `OnTargetTileChanged`) when the tile in front of the farmer changes. A new cosmetic component on the farmer (for example `UFarmTargetHighlightComponent`) listens and shows a real highlight, such as a decal or an outline mesh, that ships. Checking the target on a short timer or when the farmer moves is enough; it doesn't need every frame. Add the new component to the ownership table.

### A-11 — Bedtime is polled every frame, and passing out depends on the grid

**Severity: Medium**

**Status: Resolved (2026-10-29), 758f81c and 9d77ee1.** `AFarmDayCycle::OnDayEndReached` fires once when the clock stops, and the day cycle stops ticking until the next day. Starting a day no longer needs a grid. Sleep and passing out moved to `UFarmRestComponent`.

`UFarmingComponent::TickComponent` asks `AFarmDayCycle::IsPastBedtime()` every frame. The day cycle owns the clock and already knows the moment it reaches `DayEndHour` (its `Tick` clamps there), but it broadcasts nothing. `UFarmingComponent::StartNextDay` also returns early when there is no grid, so in a level without a grid the farmer can't sleep, and at 02:00 the component calls `StartNextDay` again every frame while the clock stays stopped.

**Why it matters:** `AGENTS.md` rule 12 and [section 10](Architecture.md#10-performance-by-design) ask for events instead of polling. More importantly, milestone 2 replaces passing out with stamina and late-night rules (GameDesign G-03, G-04), and every new consumer of "the night is over" would add another poll. Starting a day doesn't need the grid: the grid listens to `OnNewDay` and simply isn't there to listen.

**Recommended change:** Give `AFarmDayCycle` an event such as `OnDayEndReached`, broadcast once when the clock reaches `DayEndHour`. `UFarmingComponent` binds to it and starts the next day. Drop the grid check from `StartNextDay`. With A-10 done too, `UFarmingComponent` no longer needs to tick. Record the event in the ownership table and the day-flow diagram.

### A-12 — The save data layer depends on the tile actor

**Severity: Low**

**Status: Resolved (2026-10-29), 7500292 and 9cdf0ac.** `EFarmTileState` and the rules live in `FarmTileState.h` (`FFarmTileState`), the save structs in `FarmSaveData.h`, and `AFarmTile` has `GetSaveData` and `RestoreFromSaveData`.

[Section 14](Architecture.md#14-dependencies) puts `UFarmSaveGame` in the data layer, below the world-state actors, and says lower layers never include higher ones. `FarmSaveGame.h` includes `FarmTile.h`, an actor header, only to get `EFarmTileState`. `FFarmTileSaveData` is defined by the save game rather than by the tile that should produce and restore it ([section 6](Architecture.md#6-saving)).

**Why it matters:** Every file that includes the save game also compiles the tile actor's header, and the dependency points the wrong way. It will matter more when the tile's rules move into a plain state struct (a known gap): the save data should depend on that struct, not on the actor.

**Recommended change:** When doing the roadmap's "each owner saves and restores its own state" item, move `EFarmTileState` and `FFarmTileSaveData` into a small header with no actor dependency (for example `FarmTileState.h`, which can later hold the tile-state struct), and have `AFarmTile` provide `GetSaveData()` and `RestoreFromSaveData()`.

### A-13 — `UFarmingComponent` is collecting jobs

**Severity: Low**

**Status: Resolved (2026-10-29), c87081e, 9d77ee1 and 51819b4.** The split is decided and built: sleep and the late-night rules belong to `UFarmRestComponent`, the highlight to `UFarmTargetHighlightComponent`, player messages to `UFarmFeedbackSubsystem`; inventory and money move to the player state with the inventory work (decision log). `UFarmingComponent` keeps the toolbar, seeds, targeting and tool use.

The component's row in the ownership table already lists the toolbar, seeds and the seed crop, the harvest count, targeting, tool use, sleeping and passing out. In code it also finds the grid, sends player messages to the HUD and draws the target highlight (A-10). Milestone 2 plans stamina, money, more items and late-night rules, and each has an obvious but wrong home here.

**Why it matters:** [Principle 1](Architecture.md#1-core-principles) is one job per class. A component that is the default home for "something the player does" will become the class the rest of the game reaches into, and the hardest one to test (see C-17).

**Recommended change:** No change yet beyond A-10 and A-11. Decide the split before adding stamina or money, and record it: inventory and money move to an inventory component on the player state ([Expected changes](Architecture.md#18-expected-changes)); stamina becomes its own component; sleeping and the late-night rules belong with whichever of the day cycle or a new rest component owns "the day is over". `UFarmingComponent` keeps the toolbar, targeting and tool use.

### A-14 — Key hints repeat the key bindings

**Severity: Low**

**Status: Resolved (2026-10-29), 6805d86 and 812ea57.** `AFarmerCharacter` creates its input once, labels each action and reports its keys from the mapping context; the HUD builds every hint from that.

`AFarmerCharacter::SetupPlayerInputComponent` binds E, N and F9, and `SFarmHudWidget::GetControlsText` writes "E Use tool | N Sleep | F9 New game" as a literal. Only the toolbar part of the hint is built from data. The HUD therefore knows the farmer's keys without asking the class that owns them.

**Why it matters:** Changing a key, or later letting the player rebind keys, leaves a wrong hint on screen. [Principle 2](Architecture.md#1-core-principles) is one source of truth.

**Recommended change:** Keep the bindings in one place that the HUD can read. Today that can be a small table of action names and keys owned by the character; when input moves to `IA_`/`IMC_` assets ([section 11](Architecture.md#11-patterns-borrowed-from-lyra)), the HUD can ask Enhanced Input for each action's mapped key, which also makes rebinding work.

### A-15 — Part B, the roadmap and the gap list lag the code in places

**Severity: Low**

**Status: Resolved (2026-10-29), in the commit that adds this line.** The dependency diagram is redrawn with every class and event and no wrong-way edges left, the roadmap's crop item matches the code, the tile gap is closed, and the 2026-10-25 CQTest entry points to the setup entry.

- The dependency diagram in [section 14](Architecture.md#14-dependencies) has no arrows for the edges the gap list names (farming component and save subsystem to the HUD), for the character starting a new game through the save subsystem, or for the HUD finding the farming component on the pawn.
- The tile gap says to separate rules from visuals "before writing tile tests"; tile tests now exist without that split.
- The decision log's 2026-10-25 entry still reads "setup is pending" for CQTest. A newer entry records the setup, which is how the log should work, but nothing points from the old entry to the new one.
- The roadmap's crop item lists "days per stage" and "radiation tolerance"; the code and the decision log have no per-stage days and call the field `ContaminationTolerance`.
- The roadmap's "Tile rule tests" item is unticked although five tile tests exist (see C-17). ("Tool data assets" was also unticked at review time; the assets and the tick landed in 3f16cfa the same day.)

**Why it matters:** Part B is meant to be the map a returning developer trusts. A diagram that hides the known wrong-way edges makes them easy to forget.

**Recommended change:** Draw the known-gap edges in the diagram as dashed lines labelled "known gap", and remove them as the gaps close. Reword the tile gap to "before the field grows a lot or tile rules get more complex". Align the roadmap's crop and test items with the code.

## Status of earlier findings and known gaps (2026-10-29)

*This section is the state at the code review. The state after the clean-up is in [Re-audit after the clean-up](#re-audit-after-the-clean-up-2026-10-29).*

A-01 to A-09 were documentation findings and stay resolved: the document still says what their status paragraphs describe. A-08's follow-up is done: CQTest is set up and the roadmap item is ticked.

The [known gaps](Architecture.md#17-known-gaps), checked against the code. All are still open; none of them has been made worse by the crop and tool work.

| Known gap | Status |
|---|---|
| Game logic calls the HUD | **Open.** `UFarmingComponent::ShowMessage` and `UFarmSaveSubsystem` (`AppendMessage`, `ShowMessage`) still call `AFarmHUD`. |
| The save subsystem reads and writes other classes' state | **Open.** It writes `Seeds` and `HarvestedCrops` directly and reads tile fields one by one. The tool now goes through `RestoreCurrentTool`, which is the right pattern. |
| Content IDs aren't checked | **Open.** No `IsDataValid`, duplicate check or redirect list. |
| No extension points | **Open.** Nothing is `BlueprintCallable` or Blueprint-assignable. |
| The toolbar is fixed per farmer Blueprint | **Open**, as designed for now. |
| Saves don't record a tile's crop | **Open.** Tracked by the `TODO` in `UFarmSaveSubsystem::LoadFarm`. |
| Tiles are saved by grid position | **Open.** |
| No save version number | **Open.** An empty `CurrentToolId` is the only marker of an older save. |
| An unreadable save gets overwritten | **Open.** `SaveFarm` runs on the next new day whether or not loading failed. |
| `AFarmDayCycle::Get` doesn't follow the find-or-spawn rules | **Open.** It is called from both `AFarmGrid::BeginPlay` and `UFarmingComponent::BeginPlay`, so whichever runs first spawns the fallback. |
| Load waits a tick | **Open.** |
| The grid is found with `TActorIterator` in several places | **Open.** Still in `UFarmingComponent::BeginPlay` and `UFarmSaveSubsystem::BindAndLoad`. |
| Tile rules and visuals live in the same class | **Open.** See A-15 for the wording. |

## Positive foundations to preserve

- The ownership table and “ask the owner to change its state” rule are strong; preserve them as systems are added.
- The initialization section correctly warns against relying on cross-actor `BeginPlay` order and discourages constructor-time world access.
- The distinction between game rules and presentation is sound, and the known-gap section clearly records current violations.
- The save-data direction—plain values/IDs, owner-provided serialization and restore without replaying gameplay—is a good target once the ownership boundary is implemented.
- The decision log and explicit “not adopted yet” entries prevent unnecessary framework adoption. Keep documenting choices, but distinguish decisions, current implementation and future intent.

Seen in the code on 2026-10-29:

- Crops and tools follow the mod rules: discovered by the Asset Manager, typed-in `ContentId`s that build the primary asset ID, soft-referenced meshes in a `Visuals` bundle loaded at play start, and no list of content in code.
- Each content and data change was recorded in the decision log in the same commit, including why `EFarmToolAction` is an enum and tools aren't.
- Old saves keep loading across the tool change through a small, tested mapping, and the restore goes through the owner's own function (`RestoreCurrentTool`), the pattern section 6 asks for.
- `AFarmTile::RestoreState` refuses impossible states (a planted tile without a crop), in line with principle 7.
- The farmer's input reaches the toolbar only through `SelectToolbarSlot`, and the held mesh follows `OnToolChanged`, so commands go down and events come up as principle 8 says.

## Recommended correction order

1. [x] Correct world-versus-level lifetime wording and the save subsystem/new-game description (A-01, A-02).
2. [x] Define save load/write failure behavior and the accepted save-loss window (A-03).
3. [x] Choose a canonical persisted content ID and rename/collision policy before adding more content or mods (A-04).
4. [x] Recast mod support as a future capability with explicit readiness requirements (A-05).
5. [x] Specify the synchronous day-event contract and constrain find-or-spawn behavior (A-06, A-07).
6. [x] Align the CQTest decision log with the roadmap, then revisit the actor-scale example (A-08, A-09).

From the 2026-10-29 code review:

7. [x] Replace the debug tile highlight with a shipping cosmetic component fed by a target-changed event (A-10).
8. [x] Broadcast the end of the day from the day cycle instead of polling it, and let sleeping work without a grid (A-11).
9. [x] Decide how `UFarmingComponent` splits before stamina, money or late-night rules are added (A-13).
10. [x] Move the tile's save types out of the actor header with the owner-saves-itself roadmap item (A-12).
11. [x] Give key hints one source of truth (A-14), and bring the diagram, roadmap and gap wording up to date (A-15).

## Re-audit after the clean-up (2026-10-29)

Every finding from the code review (A-10 to A-15, and C-09 to C-18 in [CodingGuidelinesAudit.md](CodingGuidelinesAudit.md)) and every known gap was worked through on `main`, then the whole of `Source/Vestige` was checked again against [Architecture.md](Architecture.md) and the review checklist in [CodingGuidelines.md](CodingGuidelines.md#17-review-checklist). The code was built (`VestigeEditor` Development, a non-unity build without precompiled headers, and the `Vestige` Shipping game target, all without warnings) and the 58 `Vestige` tests pass. An existing version 0 save was loaded headless and upgraded correctly.

### Code conformance after the clean-up

**Overall: 9/10.** Every known gap is closed or recorded as a deliberate decision. What keeps it from 10 is work that needs the editor or a measurement: the content moves, re-saving the template Blueprints, and the first performance baseline.

| Area | Score | Assessment |
|---|---:|---|
| Ownership and separation of concerns | 9/10 | One job per class. Sleep, the highlight and player messages have their own owners; the farming component keeps tools, seeds and targeting. |
| Dependency direction | 10/10 | No wrong-way edges: game code raises messages through `UFarmFeedbackSubsystem`, and the save data layer includes no actor. |
| Events instead of polling | 9/10 | Only the day cycle ticks, and only while the clock runs. Bound HUD getters read finished text; the clock line is rebuilt only when the shown minute changes. |
| Lifetimes and initialization | 9/10 | Every binding is removed in `EndPlay` or `Deinitialize`. Loading waits for the world's begin-play broadcast instead of a tick. |
| Saving | 9/10 | Everything in [section 6](Architecture.md#6-saving) is built and tested. Saving stays synchronous, which is fine while saves are small. |
| Content and mod readiness | 8/10 | IDs are validated and duplicates reported. Extension points and the ID redirect list are deferred on purpose (decision log). |

### Status of the known gaps

| Known gap at the code review | Status |
|---|---|
| Game logic calls the HUD | **Closed**, c87081e: `UFarmFeedbackSubsystem` |
| The save subsystem reads and writes other classes' state | **Closed**, 812ea57 and 9cdf0ac: each owner has `GetSaveData` and `RestoreFromSaveData` |
| Content IDs aren't checked | **Closed**, 139e88d: `IsDataValid` and `UFarmAssetManager`. The redirect list is a deferral (decision log) |
| No extension points | **Decision**, 2026-10-29: added with the systems they expose |
| The toolbar is fixed per farmer Blueprint | **Decision**, 2026-10-29: intentional until items exist |
| Saves don't record a tile's crop | **Closed**, 9cdf0ac |
| Tiles are saved by grid position | **Closed**, 9cdf0ac |
| No save version number | **Closed**, 9cdf0ac |
| An unreadable save gets overwritten | **Closed**, 9cdf0ac: two alternating slots; saving turns off |
| `AFarmDayCycle::Get` doesn't follow the find-or-spawn rules | **Closed**, 7105c61 |
| Load waits a tick | **Closed**, 9cdf0ac (the grid's `OnTilesReady` came in 7105c61) |
| The grid is found with `TActorIterator` in several places | **Closed**, 7105c61: `AFarmGrid::Find` on `FarmActorLookup` |
| Tile rules and visuals live in the same class | **Closed**, 7500292: `FFarmTileState` |
| The farming component ticks every frame (added with A-10, A-11) | **Closed**, 758f81c and 51819b4 |
| The HUD searches the world every frame (C-09) | **Closed**, 7105c61 and 812ea57 |
| No trace scopes on new-day, load and save code | **Closed**, 9cdf0ac and e12cd18 |
| Rendering settings aren't set for a target | **Closed**, 42f3a44 |

### Decisions taken instead of a change

Each is in the [decision log](Architecture.md#16-decision-log) with its reason.

- **The default performance level stays Maximum.** On desktop, Scalable only switches off auto-exposure, motion blur and anti-aliasing; the dark nights depend on auto-exposure, and cost is set by the scalability levels.
- **`LoadWithVisuals` stays synchronous.** It runs only at play start (the farmer's `BeginPlay` and loading a save), which the performance rules allow.
- **The grid's `OnTilesReady` doesn't drive loading.** It doesn't say the farmer's toolbar is loaded; the world's begin-play broadcast covers both.
- **`EFarmTool` stays**, confined to the version 0 save upgrade that needs it.
- **Crops and the held tool keep their shadows.** Guide section 12.5 allows it when they visibly need them, and turning them off would change the look without a measurement.
- **The day cycle's light search stays.** `FindLights` uses `TActorIterator` once in `BeginPlay` to find the level's sun and sky light when none is set, which the lookup rule allows.
- **Deferred:** Blueprint extension points, the content ID redirect list, inventory and money on the player state, the per-Blueprint toolbar, and maintaining Epic's template code.

### Still open

- Editor steps, listed in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-save the template Blueprints and `L_Farm` off the old module names, move `BP_FarmTile` and the materials with redirectors fixed, and regenerate the Android File Server token if it was ever used.
- A project material for the tile highlight (`M_TileHighlight`): done 2026-10-31, after the engine material drew as the default grey material in Unreal Engine 5.8 ([Architecture.md, decision log, 2026-10-31](Architecture.md#16-decision-log)).
- The first performance baseline on the target hardware ([Architecture.md, section 10](Architecture.md#10-performance-by-design)).
- Whether to split `Source/Vestige` into feature folders now that there are 20 of our own headers ([CodingGuidelines.md, section 14.1](CodingGuidelines.md#141-source)). The maintainer's call.

### Play in Editor checklist (2026-10-29)

The checks for this clean-up moved to [PlayTest.md, Pending checks](PlayTest.md#2-pending-checks), the one place for manual testing in the editor.

## Review limits

The first review (A-01 to A-09) is a document audit, not a code audit or a certification against every Epic recommendation. The codebase was not traced to confirm each implementation statement. The CQTest status was cross-checked against the project roadmap and coding guidelines; other findings evaluate the claims and omissions in the architecture document itself. Revisit the audit when save migration, packaged mod loading, or world travel becomes an implemented feature.

The code review (A-10 onward) read every file in `Source/` and `Config/` on `main` at commit `d9f10d7` (2026-10-28, tools as data assets) and traced each known gap to the code. The maintainer's PC wasn't reachable, so uncommitted local work (the tool data assets themselves) wasn't seen. Blueprints and data assets were checked only through the class and asset references stored in the `.uasset` files, not opened in the editor. Nothing was compiled or run: the cloud environment has no Unreal Engine. The shipping-build claim in A-10 follows from how the engine defines `ENABLE_DRAW_DEBUG`; confirm it by packaging a Shipping build.
