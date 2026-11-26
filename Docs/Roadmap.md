# Project Vestige roadmap

Everything we know we still have to build, in the order we plan to build it. Finished items are in [RoadmapArchive.md](RoadmapArchive.md), under the same headings.

- [GameDesign.md](GameDesign.md) says what the game is and why. This roadmap only tracks the work, and links to the design for details.
- [RoadmapSuggestions.md](RoadmapSuggestions.md) holds exploratory roadmap proposals that are not
  approved items yet.
- [Architecture.md](Architecture.md) says how the code fits together. Many items in the foundations section come from its known gaps and expected changes.
- [Lore.md](Spoilers/Lore.md) holds the story, characters and names.

**Keep this document up to date.** In the commit that finishes an item, tick it and move it, with its sub-items, under the same heading in [RoadmapArchive.md](RoadmapArchive.md), so this file holds only open work. An open item whose parts are partly done stays here, with a "Done" line under each finished part, until all of it is done. When new work turns up, add it under the right milestone. When the plan changes, move items rather than deleting them, and record the reason in the design or architecture decision log.

Legend: `[x]` done, `[ ]` to do. Items marked *(decide)* need a decision before work starts.

Contents

1. [Where we are](#1-where-we-are)
2. [Milestone 1: playable prototype (done)](#2-milestone-1-playable-prototype-done)
3. [Housekeeping](#3-housekeeping)
4. [Milestone 2: vertical slice](#4-milestone-2-vertical-slice)
5. [Milestone 3 and later](#5-milestone-3-and-later)
6. [Before a public release](#6-before-a-public-release)
7. [Open decisions](#7-open-decisions)

---

## 1. Where we are

- **Done:** milestone 1, the playable prototype, and most of the farming core of milestone 2: crops and the season, planting rules, water and the well, withering, contamination with a mutated crop, selling and the seed shop.
- **Now:** the rest of milestone 2, the one-season vertical slice. What is left is the caretaker robot, the sprinkler, dirty water, the screens a first-time player needs, and the slice play-test that ends the milestone.
- **Next up**, in order (reviewed 2026-11-24, @DunderGG said yes; the hotbar moved before the sprinkler and the feel pass and the day-one test added on 2026-11-26, from [RoadmapSuggestions.md](RoadmapSuggestions.md) items 1, 2, 16 and 17):
  1. The open play-tests of contamination and the seed shop (the housekeeping quick fixes before them were done on 2026-11-25).
  2. Hob's relationship track, and how the player sees his opinion change, with his short lines on events ([4.4](#44-the-caretaker-robot)).
  3. The hotbar and the bag on screen ([4.6](#46-interface-and-input)), before the sprinkler, so the player buys and places it with an inventory they can see (2026-11-26).
  4. [4.5](#45-automation-and-selling): placing objects on the farm grid, and the sprinkler, bought at the seed shop.
  5. Dirty water and its source ([4.2](#42-farming-depth)).
  6. Lay out the home farm ([4.7](#47-starting-and-testing-the-slice)): the farmhouse with the bed inside, the service yard, Hob's spot and the dirty water source, as a blockout, once every slice prop exists. The screenshot art pass's ground dressing comes after it, so it doesn't dress a layout that still moves; its meshes and sky don't wait for it.
  7. What a first-time player needs to see: a dry crop that looks thirsty, the mutated ripe meshes and item descriptions ([4.2](#42-farming-depth)), and the feel pass on the four farming actions ([4.6](#46-interface-and-input)).
  8. The HUD layout pass ([4.6](#46-interface-and-input)), laid out for handheld PCs and for translation, as @DunderGG decided on 2026-11-26 ([GameDesign.md, section 14](GameDesign.md#14-decision-log)).
  9. [4.7](#47-starting-and-testing-the-slice): the title screen and the day-1 save.
  10. A day-one test with one person who hasn't seen the game ([4.7](#47-starting-and-testing-the-slice)), so Hob's tutorial teaches what that person actually missed.
  11. Hob's tutorial and his reminders of crops at risk ([4.4](#44-the-caretaker-robot), [4.2](#42-farming-depth)), which need Hob and dialogue.
  12. The slice play-test with players who haven't seen the game ([4.7](#47-starting-and-testing-the-slice)).

  Polish that isn't on this path (the sun and the moon, the per-minute clock, held meshes, automated play-tests, the performance agent) waits until after it unless it is cheap to do alongside.

  The screenshot art pass ([4.6](#46-interface-and-input)) comes before the first devlog post, outside the numbered order: the dispenser mesh, the bed and the sky depend on nothing else and can come after item 1, and the ground dressing comes after the home farm's layout (2026-11-25).

## 2. Milestone 1: playable prototype (done)

A third-person farmer on a small field who can till, plant, water and harvest, with a day and night cycle, a HUD and saving.

All its items are done and listed in [RoadmapArchive.md](RoadmapArchive.md#2-milestone-1-playable-prototype-done).

## 3. Housekeeping

Work outside the milestones' features: the tools, the tests and the rules.

- [ ] Rename the two event handlers named for when they run, `UFarmingComponent::HandleItemInHandChanged` and `AVestigeHUD::HandleInputMethodChanged`, for what they do ([CodingGuidelines.md, section 2.3](CodingGuidelines.md#23-case-and-wording)), in a small commit of its own (code review of the contamination slice, 2026-11-22)
- [ ] A docs checker, `Tools/check_docs.py` (standard-library Python, so it runs anywhere, cloud threads included), run by `Tools/Verify-Vestige.ps1` (@DunderGG said yes, 2026-11-26):
   - Every relative link in the repository's Markdown files leads to a file that exists, and every `#section` link to a heading or `<a id>` there. A first run on 2026-11-26 found four broken links in 88 files: `Docs/Systems/14-Audio.md` links `CONTRIBUTING.md`, `LICENSING.md` and `CREDITS.md` one folder too shallow (`../` instead of `../../`), and `Docs/RoadmapSuggestions.md` links `GameDesign.md#farming`, which has no such heading. Fix those four with it. The audit files' links to the old `FarmSim` names are history and stay as they are, so the checker needs a short list of files it skips
   - Every file under `Art/` and `Content/Vestige/` matches a row of the path table in `LICENSING.md`, so no asset is left without a record of who or what made it (asked by @DunderGG, 2026-11-23, instead of a README per `Art/` folder, which would repeat the table in many places and drift). It reads the same files as the link check, so it is the same script. To decide through the architect and the reviewer: how the table's patterns are read
- [ ] A second GitHub Actions workflow, `.github/workflows/repo-checks.yml`, beside the whitespace check, for what can be checked without Unreal (@DunderGG said yes, 2026-11-26). @DunderGG pushes to `main` himself, so it reports after a push and can't stop a commit: each check also runs from `Tools/Verify-Vestige.ps1`, so a failure shows before the push, and the workflow is the gate for contributors' pull requests. Four jobs, each seconds long:
   - The docs checker above, once it exists (links, section links, the `LICENSING.md` path table and the SPDX headers, which are all present on 2026-11-26)
   - The Python tests: `python -B -m unittest discover -s EditorScripts/Tests` (24 tests, all passing on 2026-11-26), and any later tests of `Tools/` and `Art/Blender/` scripts that run without Blender or the Editor
   - clang-format on the changed lines only (`git clang-format --diff` against the push's or pull request's base), since 8 of 218 source files aren't clean on 2026-11-26; pin the clang-format version to the one the agents use on @DunderGG's PC, so two versions don't disagree. Reformatting the 8 files is a separate commit, if wanted
   - Every file matching a `filter=lfs` pattern in `.gitattributes` is stored as an LFS pointer (all are on 2026-11-26), since a binary committed outside LFS stays in the public history unless history is rewritten
   - Not in CI: building `VestigeEditor` and the `Vestige` suite (they need Windows with Unreal 5.8; GitHub's large runners cost money, and a self-hosted runner on @DunderGG's PC would run strangers' pull request code from a public repository), and a Python copy of `Vestige.Architecture.IncludeDirection` (two copies of the layer order would drift)
- [ ] Check every static mesh in `Content/Vestige/` automatically against its triangle budget and for missing collision, so a mistake in an asset nobody re-checks by hand is caught (asked by @DunderGG, 2026-11-12). To decide through the architect and the reviewer: an editor data validator (the Data Validation plugin's `UEditorValidatorBase`, which runs when an asset is saved and on Validate Assets) or a `Vestige` automation test, where the budgets per kind of asset live (crop, tool, building, from [CONTRIBUTING.md, Budgets](../CONTRIBUTING.md#budgets)) and how a mesh's kind is known, how Nanite meshes are counted, and which meshes need no collision (crops). Cheap to add to the same check: textures over 1024, small props that cast shadows, and any asset or map that references the store-asset folders `.gitignore` keeps out of the repository (`Content/Fab/`, `Content/Megascans/`...), which would break in a fresh clone (art direction review, 2026-11-13). A validator needs an editor-only module or dependency, since `Vestige` is a runtime module; an automation test doesn't
- [ ] Automate the play-tests, so the maintainer only judges feel and fun (asked by @DunderGG, 2026-11-13; the first step, the test author checking the game off-screen itself before writing a manual check, is in its instructions). To design through the architect and the reviewer: functional tests in a test map (Unreal's `AFunctionalTest`) that drive input, sleep and save and check the result; a script that runs the game off-screen (`-RenderOffScreen`) and takes screenshots at set moments for an agent to look at; Unreal's screenshot comparison against reference images approved once, which needs a run with a GPU (not `-nullrhi`) and stable lighting and time; an automatic performance run against the budget; and later Gauntlet for packaged builds. Then go through the open checks in `PlayTest.md` and move each one that can be automated
- [ ] Make the agent workflow cheaper over time (asked by @DunderGG, 2026-11-13; the first steps, a small-task path, Sonnet for the test author, filtered logs and diff-first reviews, are in `AGENTS.md`, `CLAUDE.md` and the agents' files):
   - Measure what a task costs: the main session notes each agent's token count (Claude Code reports it when the agent finishes) in its final reply, so the expensive steps show up before anything else is changed. Done 2026-11-19: `Tools/measure_agent_tokens.py` reads Claude Code's transcripts and shows each session's tokens and estimated cost, and with `--session` splits one session by role (main session, architect, design review, implementer, test author, code review...), so nobody has to note the counts by hand
   - Find what fills the main session's context before its first step: at the start of the next roadmap session, @DunderGG types `/context` once (an agent can't run it) and the biggest parts are noted here. The crops session started at about 130,000 tokens, which over its 348 turns was about half of the 92 million it read
   - The implementer runs on Sonnet as a trial from 2026-11-19. The test author on Sonnet was checked that day in the session transcripts: 0 to 2 Low findings per task, never a Medium or High one, at most one fix round, and a few compile errors in its own tests that it fixed itself. The implementer was then the largest token user per task (10 to 21 million cached tokens read). After the next task, compare its findings and fix rounds with its Opus record (0 or 1 Low finding and at most one fix round per task), and set it back to `opus` if it gets a Medium or High finding or needs more than one fix round. The artist stays on Opus until the trial is judged
   - Move finished items out of `Roadmap.md` into a `RoadmapArchive.md`, as `PlayTest.md` does with `PlayTestArchive.md` (`Roadmap.md` is about 21,000 tokens, mostly ticked items). Done 2026-11-22: the 156 ticked items are in [RoadmapArchive.md](RoadmapArchive.md) under the same headings, and every heading stays here so links to a section still work. Sizes, as UTF-8 bytes / 4: `Roadmap.md` 26,500 to 10,700, and nobody needs to read the archive. The decision log stays whole until searching it gets slow or noisy, as the next line says
   - Move replaced and old entries of `DecisionLog.md` (about 30,000 tokens) to an archive file if searching it gets slow or noisy
   - Keep `AGENTS.md` and `CLAUDE.md` short (about 5,500 tokens together): they load into every agent and the main session, so text that only one role needs belongs in that role's file
   - After the `/context` check and a tracker run (`python Tools/measure_agent_tokens.py --last 5 --html`), pick from these by the numbers (2026-11-26, @DunderGG chose to measure first): (1) move the text only the main session, the architect and the reviewer use (`AGENTS.md` "How work is done", `CLAUDE.md` "Specialist subagents", about 4,700 tokens) into a workflow document those roles are told to read, since today it loads into every agent's every turn (estimated 1.9 million cached tokens, about 5% of a task); (2) set `effort` per agent ([WORKFLOW_AUDIT.md](../.claude/WORKFLOW_AUDIT.md), fix 7), lower first for the test author and the game designer, judged by the tracker's split by token kind. Rejected: cutting the implementer's full read of `CodingGuidelines.md` (a fix round costs more), reading `Architecture.md` by section (the reviewer needs most of it) and Sonnet for the reviewer or the architect
   - Watch the largest system guides (Inventory and the Gameplay Ability System are about 6,000 to 7,500 tokens) and split a guide that keeps growing
- [ ] Rules from the [code review](CodeReview.md#project-rules-that-are-themselves-wrong) (2026-11-04), each *(decide)* by @DunderGG, then changed in its document. Five are still open: decide them in one sitting, listed in [section 7](#7-open-decisions) until then (roadmap review, 2026-11-24):
   - Replace the strict linear layer order with folders that depend on `Core/` and the features they declare, and plan a `VestigeCore` module once a boundary settles ([M3](CodeReview.md#m3--the-strict-linear-layer-order-is-the-wrong-shape))
   - Cut `Architecture.md` to principles and a one-page system map, keep ownership in the class comments, and log only hard-to-undo decisions ([M8](CodeReview.md#m8--the-documents-outweigh-the-code), rule 5). Done 2026-11-11 and 2026-11-12, as @DunderGG chose it ("yes to all" to the recommended options): one ownership list, section 13 as a one-page map with what each class doesn't own (the detail lives in the class comments and the guides), and the same decision log scope with new entries of about 100 words. Logging only hard-to-undo decisions was rejected because it would take away the maintainer's way to overrule an agent's choice ([Architecture.md, decision log](Architecture.md#16-decision-log), 2026-11-11)
   - Performance as a frame budget per system and a packaged-build profile at every milestone, with micro rules only for per-tile and per-frame code (rule 1)
   - Drop "change structure only when a profile says so" for structural choices whose cost is known up front (rule 2). Done 2026-11-05 with C2: [Architecture.md, section 10](Architecture.md#design-rules) now says to decide structure up front when its cost is known
   - Let a system be prototyped and played before its `Systems.md` guide; keep guide approval for adopting engine frameworks (rule 3)
   - Build what is right for the game even when it needs an editor asset; @DunderGG makes the asset or the agent writes the steps (rule 6)
   - One class prefix, `Vestige`, for new types, and no more renames of existing `Farm` classes ([L1](CodeReview.md#low))

## 4. Milestone 2: vertical slice

**Goal:** one full 28-day season that proves the farming core and a first relationship with the caretaker robot. The town, humans, exploration, combat and survival needs are tested in later milestones. See [GameDesign.md section 13](GameDesign.md#13-scope-and-milestones).

**Done when** a player can start a new game, meet the caretaker robot, grow three or four crops (one of them mutated) in soil with water and radiation to manage, run out of stamina, pay for staying up late, set up a sprinkler, sell the harvest, and play through to the end of the season, and playtests meet the experience criteria in [GameDesign.md section 13](GameDesign.md#next-vertical-slice).

### 4.0 Structure from the code review

The fixes from the [code review](CodeReview.md) of 2026-11-04, in the order to do them. C1 and C2 come before [4.2](#42-farming-depth), because water and radiation on every tile build on the world structure and the tile representation. Each item names its finding; tick the item and add a status line under the finding in the same commit.

When working nearby:

- [ ] Blueprint tool abilities, H1's modding half: make `CanUseOn` and `UseOn` Blueprint-implementable, make the field's rules callable from Blueprints, and turn the tool's `Ability` into a soft class reference in its bundle, when mod support is designed ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system), tools as abilities)
- [ ] A cost effect per tool (`CostEffect` on the tool data asset, falling back to the game data's `GE_StaminaCost`), when a tool costs something other than stamina (found with the `GE_` assets, 2026-11-07)
- [ ] Load only the crops of the seeds the player carries, if crops ever number in the dozens: the content catalogue loads every crop with its meshes once per game today (since 2026-11-08). Measure first (found with H2, 2026-11-07)
- [ ] A held tool use resumes after a jump only when the farmer walks again: landing in water or in navigation walking doesn't resume it (releasing E still ends it). Revisit when the farmer can swim or robots use tools
- [ ] Write saves with `AsyncSaveGameToSlot` once they grow ([L4](CodeReview.md#low)). First measure saving and loading on the 60 by 60 benchmark field: each tile is about five tagged properties with their names as text, so a large field is a large record. Saving only the tiles that differ from grass is the likely fix, since tiles carry their coordinates. `AsyncSaveGameToSlot` still builds the bytes on the game thread
- [ ] The content catalogue sees content added while a game runs (a mod's game feature plugin turned on mid-game) by listening for new primary assets, when mod support is designed; until then it sees it at the next game (found with M5, 2026-11-08)
- [ ] A devlog capture kit, so screenshots and clips need no photo mode (2026-11-26): a cheat command in `UVestigeCheatManager` that hides and shows the HUD, and a short note in `Docs/` on taking shots (Unreal's built-in `HighResShot` console command) and on a season's time-lapse of the field (`SkipDay` with a fixed camera, recorded with Movie Render Queue or a screen recorder). A real photo mode stays a later decision ([RoadmapSuggestions.md](RoadmapSuggestions.md) item 18)
- [ ] Remove the template's touch controls when the character is reparented ([M6](CodeReview.md#m6--the-template-code-is-load-bearing)): nothing shows them since 2026-11-06, but `BP_PlayerCharacter` still implements `BPI_TouchInterface`, so `Content/Input/Touch/` stays until then
   - The template's `IA_Move`, `IA_Look`, `IA_MouseLook` and `IA_Jump` have no player-mappable names and are bound through the template's own properties; when the template goes they get mappable names (their input tags), join `DA_InputConfig_Player` and are bound by tag, and their contexts move to `Content/Vestige/Input` (found with H6, 2026-11-07)
   - The gamepad layout is provisional: interact (B today) likely moves to A when jumping moves with the template's replacement (H6, 2026-11-07)
- [ ] An automated test that the pawn's mapping contexts don't stack when it is possessed again, if a test map with a local player can run headless (`FMapTestSpawner` on an empty test map, never `L_Farm`, editor builds only); a play-test check until then (found with H6, 2026-11-07)
- [ ] Ability input actions in the input config (abilities activated by input tag), when the first key maps straight to an ability with no owner logic in between ([Systems.md, section 5](Systems.md#5-input); decided with H6, 2026-11-07)

### 4.1 Foundations

The code changes that the slice's content depends on. Doing them first means adding a crop, tool or item later is a new data asset, not new code. Details in [Architecture.md sections 12, 17 and 18](Architecture.md#17-known-gaps).

**Content as data**

- [ ] An ID redirect list, once an ID first has to change (deferred, see the decision log)

**Editor tooling**

- [ ] Implement and validate the first generic, read-only level survey: inspect only the currently open Editor map and write a human-readable report plus versioned JSON under ignored `Saved/LevelReports/`; test it on `L_Farm` and a structurally different or empty map. The technical design is in [Systems.md, section 23](Systems.md#23-editor-scripting); the broader proposals and deferred tools are in [Docs/Levels/Scripts/README.md](Levels/Scripts/README.md).

**Communication and extension points**

- [ ] Harvests by anything but the player's tools (robots, automation) reach `On Crop Harvested` too: when the first other harvester is built, move the event to the owner that sees every harvest (a change mod Blueprints would have to follow, so cheapest before mods exist). Likely due with Hob, who shows his trust by helping on his own initiative ([GameDesign.md, section 8](GameDesign.md#relationships))
- [ ] Blueprint-callable main actions and Blueprint-readable state (a tile's state, the clock, inventory counts), as extension points, once a mod or content Blueprint first needs them ([Architecture.md, section 12](Architecture.md#12-designing-for-mods), rule 4)
- [ ] A way to choose between an interactable's options (a menu, or a key per option), when the first object offers two actions at once; until then the interact key uses the first ([Systems.md, section 7](Systems.md#7-interaction))
- [ ] A marker or outline on the focused interactable in the world, as a presentation component, if the HUD prompt alone doesn't make clear what F will use

**Tests**

- [ ] A redirected ID test, with the redirect list

**Performance** ([Architecture.md section 10](Architecture.md#10-performance-by-design))

- First performance baseline: moved to [4.0](#40-structure-from-the-code-review) (code review, 2026-11-04)
- [ ] Add a performance analyst agent (`vestige-performance`) by the end of milestone 2, when real art and bigger fields arrive: it profiles the packaged build with Unreal Insights and CSV captures, compares the frame against the budget in [Architecture.md, section 10](Architecture.md#10-performance-by-design) and [PerformanceBaseline.md](PerformanceBaseline.md), and finds what costs the most ([CodeReview.md, H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target)). Made like the other agents, with @DunderGG's yes on its instructions (asked by @DunderGG, 2026-11-12)

### 4.2 Farming depth

- [ ] A dry crop looks thirsty (paler, a little drooped) while its tile is dry, so the player notices a crop that won't grow. Needed for the slice play-test's first goal ([GameDesign.md, section 6](GameDesign.md#tiles), decided by @DunderGG 2026-11-20)
- [ ] Rename the soil's Blender material `M_Soil` in `Art/Blender/make_farm_assets.py` (`SM_Soil_Tilled`'s slot still carries the deleted material's name) at the next re-export, and reimport with no materials made (art review, 2026-11-20)
- Replacing the well splash: moved to [section 6](#6-before-a-public-release) with the rest of the sound work (@DunderGG, 2026-11-22)
- [ ] Mutated ripe crops that read at a glance: the mutated ripe meshes the [contamination guide](Systems/27-Contamination.md) plans (`MutatedRipeMesh`, one per spring crop) in place of the pink tint alone, striking enough (oversized, twisted or faintly glowing) to read in a greyscale screenshot from the game camera, so they work for colour-blind players and in devlog images. How they look is @DunderGG's art-direction call, prepared by the artist with options (2026-11-26, [RoadmapSuggestions.md](RoadmapSuggestions.md) item 16)
- [ ] Dirty water: easy to get, but raises tile radiation. It is the slice's clean versus dirty choice, one of the play-test goals ([GameDesign.md, section 13](GameDesign.md#next-vertical-slice))
   - Decide the dirty water source with @DunderGG, open in [GameDesign.md, section 15](GameDesign.md#15-open-questions); assumed until then: unlimited, a prop of its own, its water in the contamination cue
   - The source's mesh by the artist, and the prop in `L_Farm`
- [ ] The caretaker reminds the player of crops at risk in the season's last days: "at risk" when the nights a crop still needs equal the days left, "lost" when they are more ([GameDesign.md, section 6](GameDesign.md#6-farming); game designer, 2026-11-20)
- [ ] Rename `UFarmGameplayAbility_Harvest`, `HarvestToolId` and `UFarmingComponent::Harvest` now that the hand also clears withered crops, with `[CoreRedirects]` and a test, in a commit of its own (design review, 2026-11-20)
- [ ] A description field on content items, a design for the architect (a data asset field, with where the player reads it): the writer's Ready descriptions for the cleaning crop's pod and seeds and the three mutated crops wait for it in [Spoilers/Items/Contamination.md](Spoilers/Items/Contamination.md), which hold the names only today (2026-11-22)
- [ ] A test for the "Welcome back" message's values (`Vestige.Feedback.Save.Loaded`, season and day of the season); the save subsystem builds that report privately, so the test needs a hook (code review, 2026-11-14)

### 4.3 Stamina and late nights

- [ ] Starting stamina from data instead of the constant in `UVestigeStaminaSet` (`VestigeStamina::StartingMaxStamina`), once a second kind of character (a robot) needs other numbers or tuning needs it without a build: for example a starting-values effect granted by the ability set, or Epic's attribute table (`UAttributeSet::InitFromMetaDataTable`). The global game data (`DA_GameData`) can name a starting-values effect
- [ ] When tools get animations, a use's time comes from its montage, played at half speed at zero stamina, instead of `UseSeconds` alone (Systems.md, section 9)
- [ ] Hostile robots at night wait until combat exists ([milestone 3](#combat-and-the-wasteland)). When they come, the midnight warning says "it's dangerous out now" as [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) has it
- [ ] No sleeping before an earliest bedtime, 18:00 as @DunderGG suggests (2026-11-07), so the player can't skip days one after another *(decide: the hour, and whether the bed hides Sleep before then or offers it and says it's too early)*
   - The rule belongs in `UVestigeRestComponent` with the other late-night hours (an `EarliestSleepHour` setting beside `LateWarningHour` and `LateNightHour`), and `AVestigeBed` asks it in `GetInteractionOptions`, so the bed keeps no rules of its own
   - Times after midnight count as late, not early: the clock's time of day runs past 24 until the day ends at 06:00
   - The `SkipDay` cheat calls `Sleep` too and must skip the rule. The rest tests that sleep in the morning move the clock past the hour first, and a new test covers sleeping too early
   - Add the rule to [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) and its decision log, since it limits "your pace"

### 4.4 The caretaker robot

- [ ] Hob's Blender model, replacing the placeholder body (`BP_Hob`'s cubes, built 2026-11-26) and keeping the cyan eyes material `M_HobEyes` for the eye slot
- [ ] One relationship track with Hob, who starts as a dutiful servant rather than a sceptic; trust grows through talking, gifts and working together, and the slice arc is in [Hob's character sheet](Spoilers/Characters/Hob.md#vertical-slice-arc)
   - How the player sees his opinion change, one of the play-test goals: his lines change and he helps on his own initiative ([GameDesign.md, section 8](GameDesign.md#relationships)), with no number on screen unless the play-test shows it is needed. Designed with the game designer and the writer (roadmap review, 2026-11-24)
- [ ] Hob's short lines on events, said once each in passing with no dialogue box (the player's first mutated crop, watering in the rain once there is rain, still awake past midnight, the first sale): they make him react to the world with no new system, using the dialogue data above. The lines are the writer's; which events, and how often he may speak, are designed with the game designer (2026-11-26, [RoadmapSuggestions.md](RoadmapSuggestions.md) item 16)
- [ ] Hob guides the player through the first days as a tutorial
   - A new game opens with a fade-in on the farm and a few lines from Hob, not the cryo-pod scene, which stays in [Story](#story): a level, a sequence and writing for one minute is too much before the slice play-test (2026-11-26)

### 4.5 Automation and selling

- [ ] Placing objects on the farm grid
- [ ] A sprinkler that waters nearby tiles each morning from the farm's water store
   - How the player gets it: recommended in [GameDesign.md, section 15](GameDesign.md#15-open-questions), bought at the seed shop for about 500 coins, repeatable, as a line in `DA_Shop_Farm`. Decide with @DunderGG, with its range (3 by 3 recommended)
- The seed dispensers' real mesh: moved into the screenshot art pass in [4.6](#46-interface-and-input) (2026-11-25)

### 4.6 Interface and input

Needed for the slice play-test, so a first-time player sees what they carry:

- [ ] The hotbar and the bag on screen, built in UMG with C++ base classes: the hotbar as a row of slots with the selected one marked (its rules are built since 2026-11-07, [4.0](#40-structure-from-the-code-review), H2), and an inventory screen for moving items between slots
   - Item icons come with it: a soft texture on `UVestigeItemData` in an `Icons` bundle of its own, which the content catalogue never loads, preloaded by the row and the screen with a handle each owns while it shows them. Design there the empty or unknown list (no handle), icons already in memory (the callback still comes a frame later) and a callback for a replaced handle ([Systems.md, section 2](Systems.md#2-content-and-the-asset-manager), the content catalogue; M5, 2026-11-08)
- [ ] A layout pass on the HUD once the vertical slice's features are in: the status line now carries the date, the clock, the item in hand with a seed's days to ripen, and the stamina, and @DunderGG expects the interface to need a rework anyway (play-test note, 2026-11-20). Plan it with the hotbar and the bag on screen, and design it first with the game designer
   - Laid out for handheld PCs and for translation (@DunderGG, 2026-11-26): every screen reads at 1280x800 and 16:10 with the smallest text at least 12 pixels tall there, every menu works with a gamepad alone, room for text a third longer than English, no text in fixed-size boxes or images, and a font set with fallbacks for Cyrillic, Chinese, Japanese and Korean. English only ships for now; the rules are in [GameDesign.md, section 14](GameDesign.md#14-decision-log)

Polish, after the slice's features unless cheap alongside them:

- [ ] Holding F to harvest along a row of ripe crops, as holding E repeats a tool (found with harvesting with F, 2026-11-07)
- [ ] Tests for the HUD's other bindings (the prompt, the money and the stamina lines: binding on a new pawn, unbinding when the pawn changes and when the HUD ends), which have none; they can use the test hooks `AVestigeHUD::SetHudWidgetForTests` and `FollowPawnForTests` added for the tile line (code review, 2026-11-22)
- [ ] Test the dialogue box's typing over time: move the characters-shown count out of `UVestigeDialogueWidget::NativeTick` into a function a test can call with a time step (code review of Hob's dialogue, 2026-11-26)
- [ ] A marker on what F would use (an outline on the bed, a highlight on the ripe tile), if the prompt and the tile frame aren't enough in play
- [ ] A held mesh for items that aren't tools (a turnip in the hand), with the meshes for the new crops
- Input actions as assets: moved to [4.0](#40-structure-from-the-code-review) and decided as now (code review, 2026-11-04)
- [ ] A feel pass on the four farming actions, after Hob's dialogue (2026-11-26, [RoadmapSuggestions.md](RoadmapSuggestions.md) item 17): they fill most of a day, so how they feel is most of how the game feels, and they are what clips of the game show
   - An animation per action (till, plant, water, harvest): a montage played by the tool ability in place of its timed wait, timed so the effect starts when the tool meets the soil; it is also the montage that [4.3](#43-stamina-and-late-nights)'s use time comes from
   - A short effect per action (a puff of soil, a splash, seeds dropping, a burst of leaves) as Niagara systems, and a sound per action, all through one gameplay cue per action, as [Systems.md, section 15](Systems.md#15-visual-effects-and-feedback) says: game code never spawns an effect. The sounds are the exception to the sound deferral in [section 6](#6-before-a-public-release); each gets a listening check
   - The morning payout as a moment: the HUD's coin count ticks up from the old total to the new one with a coin sound, alongside "The bin paid N." in the morning message
   - The ripe-crop glow below is built with it, since it is the same effects work
   - Camera shake and gamepad rumble wait until the play-test asks for them
   - The slice play-test's sheet asks whether the actions feel satisfying
- [ ] Ripe crops show that they are ready to harvest, for example small glowing particles hovering around the crop (@DunderGG, 2026-11-07). The field draws its tiles as instanced meshes with no actor per tile ([4.0](#40-structure-from-the-code-review), C2), so not a particle system per crop: one Niagara system per field fed the ripe tiles' positions when the field changes, or a glow in the crop material driven by per-instance custom data, as the tiles' colour and wetness are. Measure on the 20 by 20 benchmark field of ripe turnips. Design it in [Systems.md, section 15](Systems.md#15-visual-effects-and-feedback) first
- [ ] Meshes for the new crops, the sprinkler, the shipping bin, the water source and the bed
   - 2026-11-14: the new crops are done, and the bed, shipping bin, well and sprinkler are built as scripts (`Art/Blender/make_<asset>.py`, FBX in `Art/Export/`: `SM_Bed` 694 triangles, `SM_ShippingBin` 466, `SM_Well` 434, `SM_Sprinkler` 228) on new shared helpers in `Art/Blender/vestige_art/`, reviewed by the art reviewer, on the recommended options of the open questions in [GameDesign.md, section 15](GameDesign.md#15-open-questions) (colour cues, sizes, looks), which @DunderGG can overrule. Not imported yet: the steps are in [Art/README.md, Importing into Unreal](../Art/README.md#importing-into-unreal) (`M_Prop` as the parent of the `MI_Prop_*` instances, Nanite on as Epic recommends with virtual shadow maps and no LODs, the sprinkler's cast shadow off on the mesh, and `BP_Bed`'s `BedMesh` set to scale 1 and location 0, since the placeholder scale of 2, 1, 0.5 would make the mesh 4 m long), and the well's `Stone` colour still needs a look against `L_Farm`'s ground. Still to build: the rain collector, purifier and dirty water source, and the placement ghosts that use the same meshes
   - 2026-11-20: `SM_Well` is imported with `M_Prop` and `MI_Prop_*`, and `BP_Well` stands in `L_Farm` (clean water, 4.2), with a rust crust on the pump from the art review
- [ ] A short art pass so screenshots for a devlog look like the game, not a test map, without building the full level yet (@DunderGG said yes, 2026-11-25). It follows the art direction in [GameDesign.md, section 12](GameDesign.md#12-presentation): one stylized style, low-poly first passes improved in place with the same IDs, sizes and pivots, made by the artist as Blender scripts, and no store-licensed assets (Fab Standard License, Megascans). Each mesh goes through the art review, and anything placed in `L_Farm` through the code review
   - The seed dispensers' real mesh: the artist's `SM_ShopDispenser` (0.9 by 0.9 by 1.5 m, pivot on the floor at its centre, front towards +X) with hull `UCX_SM_ShopDispenser_00` and a window material per seed so the three tell apart at a glance; then rerun `EditorScripts/SetUpSeedShop.py` (it sets the mesh at scale 1 and location 0 in place of the stand-in cube) and re-check the interaction box numbers ([Systems.md, section 26](Systems/26-Shops.md))
   - The other stand-ins the camera sees: import `SM_Bed` with `EditorScripts/ImportProp.py` and set `BP_Bed`'s `BedMesh` as the meshes item above says, and check that `L_Farm` shows `SM_ShippingBin` (imported 2026-11-22) with no cube left in its place. The sprinkler's mesh comes with the sprinkler ([4.5](#45-automation-and-selling))
   - A simple sky: tune the sky atmosphere, height fog and clouds in `L_Farm` to the washed-out, hazy wasteland look, with no new code. The sun and the moon below stay a separate item
   - Ground dressing around the field (dead grass, rocks, a broken fence, a ruin or two towards the horizon), placed after the home farm's layout so nothing is placed twice, and instanced so it stays inside the frame budget ([Architecture.md, section 10](Architecture.md#10-performance-by-design))
- [ ] Prop import as a committed editor script: the well's Interchange import (Nanite with a 100% fallback, UCX collision only, `MI_Prop_*` by slot name, the cast shadow check) ran from a script outside the repository, and the bin, sprinkler and bed need the same steps; make it `EditorScripts/ImportProp.py` with the asset as an argument, and with it move `SM_WateringCan` onto `MI_Prop_PaintTeal` and `MI_Prop_Brass` so each colour lives in one material. The well's rust crust reads as a neat painted collar from the game camera: a ragged top edge or a lopsided patch when the well gets another pass (art reviewer, 2026-11-20)
   - 2026-11-22: `EditorScripts/ImportProp.py <SM_Name>` is in and imported `SM_ShippingBin` (art review approved). Still to do: moving `SM_WateringCan` onto `MI_Prop_*`, tests in `EditorScripts/Tests/`, and running it on the bed and the sprinkler when they are imported
- [ ] Move `make_farm_assets.py` onto the shared helpers in `Art/Blender/vestige_art/` (one script per asset, a `--check-only` run, `--export-dir`, the budget check), keeping every existing asset's geometry identical, and add what the crops and tools need (tubes, `SOCKET_` empties, their pivot and collision rules); the old script rewrites every tracked FBX on each run (art reviewer, 2026-11-14). With it, move `--save-blend`'s default file out of `Art/Blender/` (into `Saved/`), since a scripted mesh's `.blend` isn't committed, and drop `FarmAssets.blend` once nothing writes it (reviewer, 2026-11-15)
- [ ] A sun and a moon instead of one light that is both: at sunset today `AVestigeSkyLighting` jumps the one directional light from the western horizon to the eastern one and from warm to blue in a single frame, so the shadows and the sky snap at 20:00 (seen by @DunderGG, 2026-11-06). Instead the sun carries on below the horizon for a real dusk in the sky atmosphere, and a second, dim directional light for the moon (the atmosphere's second sun light) fades in as the sun goes and crosses the sky through the night. Only the brighter of the two casts shadows, so the frame costs about what it does now; measure it against the baseline ([Architecture.md, section 10](Architecture.md#measuring)). Needs a moon light placed in `L_Farm` and tests of the fading
- [ ] The HUD clock shows every game minute instead of 10-minute steps (@DunderGG, 2026-11-07). At 40 real seconds per game hour the shown time changes about every 0.67 seconds
   - Cheap: the HUD reads the time each frame and rebuilds the status line only when the shown time changes, so the change is `VestigeHudClock::StepMinutes` in `VestigeHUD.cpp` set to 1, a short text rebuilt about 1.5 times a second. Since [M7](CodeReview.md#m7--game-time-events-run-on-real-time) (2026-11-11) the clock broadcasts `OnMinuteStarted`, so with this item the HUD sets the status line from that event and the day events instead of from `AVestigeHUD::Tick`, and stops ticking
   - The 10-minute step was chosen so the clock doesn't flicker every game minute, so check in play that the moving minutes don't distract. Update the clock line in [PlayTest.md](PlayTest.md)

### 4.7 Starting and testing the slice

What the slice's "done when" needs besides its features: a player who can start a game without the console, and the play-test that judges it (roadmap review, 2026-11-24).

- [ ] A small title screen: Continue, New game (asking for confirmation before it replaces a save) and Quit, built with Common UI ([4.0](#40-structure-from-the-code-review), H7). Today a new game is only the `NewGame` console command. The settings and save slots stay in [section 6](#6-before-a-public-release)
- [ ] Start a new game with a save: `NewGame` deletes both slots, so quitting before the first night leaves no save at all. Write a day-1 save once the new map has begun play instead (found in the save registry review, 2026-11-06)
- [ ] Lay out the home farm in `L_Farm` as the [home farm plan](Levels/HomeFarm/README.md) says: floor and a blocking boundary with a visible edge, a farmhouse with a small interior and `BP_Bed` moved inside (@DunderGG, 2026-11-25), the service yard east of the field with the dirty water source nearer than the well, Hob's spot by the door (move `BP_Hob` there from its stand-in place north of the field, and change Hob's `Vestige.Level` test to check they stand west of the field by the door) and the Player Start facing the field, timed against the plan's walking targets and checked with `stat unit`. Blockout shapes only; meshes and scenery are a separate art pass after it. A level, not a system: no new class, the bed's rule unchanged
- [ ] A day-one test before the slice play-test: one person who hasn't seen the game plays a packaged build from the title screen to the first sleep, with nobody helping, once the title screen, the HUD layout and Hob's dialogue exist. It shows what the tutorial must teach before it is written. The sheet records what they did, not what they say they did: found the seeds and the item in hand, noticed dry and contaminated soil, noticed Hob, went to bed (2026-11-26, [RoadmapSuggestions.md](RoadmapSuggestions.md) item 2)
- [ ] The slice play-test, which ends the milestone: a packaged build, two or three people who haven't seen the game, a sheet with one line per goal in [GameDesign.md, section 13](GameDesign.md#next-vertical-slice) (what they did, where they got stuck, without help from the person watching), and a round of fixes from what it finds, each a roadmap item
   - The end-of-season recap ([section 5](#seasons-and-weather)) stays later, as @DunderGG chose on 2026-11-14; it is cheap once the title screen and Common UI exist, if the play-test shows the season's end falls flat

## 5. Milestone 3 and later

Roughly in the order from [GameDesign.md section 13](GameDesign.md#13-scope-and-milestones). Each group becomes its own milestone with a short plan when we get to it.

### The farmer

- [ ] A player character more appealing than the Unreal template's mannequin, in the game's style (@DunderGG, 2026-11-25): a concept with the game designer, then a rigged mesh by the artist and its animations (idle, walk, run, jump, tool uses). A large job, so it comes after the slice play-test. A cheap stopgap before then, if screenshots need it: a restyle of the mannequin (our materials and colours, a hat or overalls as attached meshes) that keeps its skeleton and animations

### Seasons and weather

- [ ] All four seasons (spring, summer, autumn, winter) of 28 days each
- [ ] Crops that don't fit the new season die when it changes
- [ ] More crops for each season
- [ ] Weather: clear, rain that waters crops, dust storms, and radiation storms that raise tile radiation
- [ ] A forecast for the next day, read by the robot weather broadcaster
- [ ] One festival per season in town
- [ ] Crops that regrow, or span several seasons
- [ ] An end-of-season recap screen when a season ends (@DunderGG, 2026-11-14)
- [ ] A title card that shows the new season on its first morning, taking over from the morning message as the season's announcement (@DunderGG, 2026-11-14)

### Energy and the land

- [ ] Simulate a best-case and a worst-case season for land health and energy before building them ([GameDesign.md section 6](GameDesign.md#land-health-and-the-haze))
- [ ] Solar panels that charge power cells, with output depending on time of day, weather and haze
- [ ] Machines that draw charge and stop when it runs out
- [ ] Land health for each map, raised by healthy farming and lowered by pollution
- [ ] The haze: thinner with healthy land, giving more solar power and a brighter sky
- [ ] An easy-to-read land health meter
- [ ] "Dirty" shortcuts (dirty water, polluting machines) that are faster now and costlier later

### The robot town

- [ ] Separate maps with loading between them: the farm, the town, later the wasteland zones. Opened with `OpenLevel` as [Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go) decides: an owner for travel in the game instance, zones and hubs as data assets naming their maps, and a loading screen
- [ ] The robot town map
- [ ] Add a level designer agent (`vestige-level-designer`) with the second map: it lays out maps from the design (paths, landmarks, where things are placed, sight lines from the game camera), within the performance budget and the content rules. Made like the other agents, with @DunderGG's yes on its instructions (asked by @DunderGG, 2026-11-12)
- [ ] Townsfolk with schedules, likes and dislikes, and their own backstories (starting with the administrator, the clinic robot, the mail carrier, the engineer and the weather broadcaster)
- [ ] Gifts, requests and personal events
- [ ] Romance, dating and marriage with robots and humans; a partner moves to the farm
- [ ] Recruiting befriended robots to work on the farm
- [ ] Town restoration, the main story: repair the water pump, power, clinic and market, each unlocking something new
- [ ] Shops
   - A proper store building to visit replaces the farm's seed dispensers from the vertical slice (@DunderGG, 2026-11-22, [GameDesign.md section 9](GameDesign.md#economy))

### Progression

- [ ] Skills that level up by doing: farming, engineering, scavenging, combat
- [ ] Tool tiers: rusty, pre-war and high-tech, unlocked through the tech tree
- [ ] The tech tree, as data assets, with branches for automation, tools, water and soil machines, energy and robot upgrades
- [ ] Automation from sprinklers to robot farmhands to a fully automated farm

### Building and crafting

- [ ] Buildings and machines on the farm grid: greenhouse, water purifier, rain collector, robot charging station, storage
- [ ] Crafting stations and recipes, as data assets
- [ ] Decorating the house

### Survival needs

- [ ] Hunger, thirst and radiation sickness, kept in check by eating crops and drinking clean water
- [ ] Neglected needs lower stamina and work speed, but never kill the player
- [ ] The late-night drain rate as an attribute that gameplay effects can modify, built when the first such modifier is designed. Example: eating a healthy meal during the day applies a `GE_WellFed` effect, lasting until the next morning, that drains 30% less that night. Today the rest component sets a minute's share of its `LateNightDrainPerGameHour` setting on each game minute's drain, so nothing else can change the rate. The fix: the rate becomes an attribute (for example `LateNightDrainRate` on `UVestigeStaminaSet`, starting at 10 per game hour), and `GE_LateNightDrain`'s magnitude comes from a small magnitude calculation class (`UGameplayModMagnitudeCalculation`) that reads the current rate on every application and takes a minute's share of it, as the rest component does now. Meals, perks and robot upgrades then each become a `GE_` asset with a modifier on the rate, and stack with no new code. The quicker alternative, a `Vestige.State.WellFed` tag that the rest component scales its rate by, fits one or two cases but needs code for each new one (@DunderGG, 2026-11-07; [Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system))
- [ ] A setting to switch survival needs off, with nothing else depending on them

### Combat and the wasteland

- [ ] Simple real-time melee combat, starting with a wrench; ranged weapons later
- [ ] Hostile robots on the farm at night that attack the player, never crops or buildings
- [ ] Losing a fight: wake at home with a penalty
- [ ] The first wasteland zone
- [ ] Scavenging for materials
- [ ] More zones, each with a reason in the history: a ruined city, a research facility, a Cinderium mine, a dam

### Story

- [ ] First job for the writer: a story style guide in `Docs/Spoilers/` (the tone and where humour may land, a glossary of in-world terms, the sleeper's voice and the choices that show them growing), answering [LoreAudit.md](Spoilers/LoreAudit.md) L-09 and L-10
- [ ] The family mystery: letters, recordings and Hob's returning memories
- [ ] The humans, about four to six, each found as a story moment
- [ ] The parents' sealed message, delivered when the farm is whole again
- [ ] The opening: waking from the cryo pod under the farmhouse. Not in the slice, which opens with a fade-in and Hob's lines ([4.4](#44-the-caretaker-robot), 2026-11-26)

### Later still

- [ ] Animals: mutated livestock
- [ ] Other camera angles, such as a higher angled view
- [ ] Mod support: base content as a Game Feature plugin, loading mods from a folder, load order, a mod kit, and letting mods override base content
- [ ] Co-op multiplayer, which needs its own design and changes to the architecture
- [ ] Investigate AI-generated voice acting, once there is dialogue to voice. Lines are generated during development and shipped as ordinary audio files, never generated while the game runs. Candidates: Kokoro (free, runs locally, Apache-2.0) or Piper (free, local, but each voice has its own license) for placeholder voices; ElevenLabs or Azure AI Speech (paid, the most expressive) or a human actor for the final cast. Not Coqui XTTS (its model license forbids commercial use) or the Windows built-in voices. A batch script reads a line sheet (line ID, speaker, text, direction), names the files by line ID and normalizes their loudness. Before any generated audio is committed, check that the tool's terms allow the clips to be published under CC BY-NC-SA 4.0 as well as sold in builds, record the tool and any performer's consent in [LICENSING.md](../LICENSING.md) and [CREDITS.md](../CREDITS.md), and disclose the AI voices in each store's AI survey. Choosing the tool needs the maintainer's yes, since it touches licensing and may cost money (the maintainer, 2026-11-15)
- [ ] Gamepads that aren't Xbox-style (Switch-style pads such as HORI's, the Stadia controller and other plain HID pads) through Unreal's GameInput plugin, which maps a raw HID pad by its USB vendor and product ID. Until then only XInput pads work on Windows, as in most PC games, and Steam Input covers the rest for players. A nice to have, not needed yet (@DunderGG, 2026-11-08, after the gamepad key hints)

### Open problems
> Anything minor we have been putting off.

## 6. Before a public release

- [ ] When localization starts, run the text gather once and re-enter in the editor any display name it misses: the potato's and cabbage's crop and item names were set from editor Python, and it isn't known whether those texts got localization keys like the ones typed in the Details panel (code review, 2026-11-13)
- [ ] Raise the art to release quality in the game's one stylized style ([GameDesign.md, section 12](GameDesign.md#12-presentation)): improve the low-poly first pass in place, keeping content IDs, sizes and pivots. Paying artists for it needs @DunderGG's yes on the money and a contract, checked by a lawyer, that gives him the copyright (or an exclusive license) and accepts public release of the sources
- [ ] Decide with the final art whether Epic gets hardware ray tracing back (`r.RayTracing`, off since 2026-11-04 while the game is developed). It costs about 50 MB of shaders and longer cooks; measure Epic on the target and on a high-end PC first ([Architecture.md, decision log](Architecture.md#16-decision-log))
- [ ] Replace the Epic template code and content (character, animations, input) with our own
- [ ] Measure the performance baseline on target-class hardware (an RTX 2060 or RX 5600 XT PC with a 6-core CPU and 16 GB, [Architecture.md, section 10](Architecture.md#budget)) when one is available; until then the target's numbers are estimates from @DunderGG's PC, which has about three times its GPU ([CodeReview.md, H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
- [ ] Scale the sun's step with the shadow quality: a console variable for the step, set per `ShadowQuality` level in `Config/DefaultScalability.ini` (0.05 at High and Epic, larger at Low and Medium, where small jumps are the price of cheaper shadows), with `SunStepDegrees` on the day cycle as the level's default. Part of the graphics settings, not an option of its own (@DunderGG's idea, 2026-11-05)
- [ ] Main menu, settings and save slots, growing from the slice's title screen ([4.7](#47-starting-and-testing-the-slice)). Its New game asks for confirmation before it replaces a save ([CodeReview.md, H5](CodeReview.md#h5--f9-deletes-both-saves-with-one-key-in-a-shipped-build))
   - Key rebinding in the settings, with Enhanced Input's user settings ([Systems.md, section 17](Systems.md#17-settings)): `bEnableUserSettings` on, the screen calls `SaveSettings()` after each remap, and the key hints ask Enhanced Input for the player's current key instead of the assets' default mappings and are rebuilt on a remap event ([Systems.md, section 5](Systems.md#5-input); found with H6, 2026-11-07)
- [ ] Add a UI designer agent (`vestige-ui-designer`) with the first menu: it designs screens and the HUD for the player (layout, readability at a distance, gamepad navigation, accessibility, the pre-war interface style in [GameDesign.md, section 12](GameDesign.md#12-presentation)) before the architect designs them in UMG and Common UI. Made like the other agents, with @DunderGG's yes on its instructions (asked by @DunderGG, 2026-11-12)
- Moving the test keys F9 and N into a cheat manager: moved to [4.0](#40-structure-from-the-code-review), because F9 deletes the saves (code review, 2026-11-04)
- [ ] Turn save compatibility on at the first public build: a version per saved owner, with upgrades from then on ([CodeReview.md, H4](CodeReview.md#h4--save-upgrade-code-for-a-game-nobody-else-has-played))
- [ ] Music and sound effects. Sound polish waits until the game's basic functionality is in, so milestone 2 builds the season loop first and the HUD's text carries the feedback meanwhile (@DunderGG, 2026-11-22). The audio set-up and the agent stay, so a sound can still go in early where a feature needs one. The first exception: the four farming actions and the morning payout get their sounds in the feel pass ([4.6](#46-interface-and-input), 2026-11-26)
   - Replace the well splash with one that sounds like water into a can: the script-made one sounds artificial and has a metallic cling at the start (@DunderGG's play-test, 2026-11-21). Options: @DunderGG's own recording, a CC0 library sound (license checked and recorded in [LICENSING.md](../LICENSING.md) and [CREDITS.md](../CREDITS.md)), or a better synthesis script; same import through `EditorScripts/SetUpAudio.py`. Moved here from [4.2](#42-farming-depth) on 2026-11-22
- [ ] Settle how many feedback messages the HUD shows at once (Project Settings > Game > Vestige HUD; five while the game is being built, 2026-10-30)
- [ ] Final names for the game, the world, the town, the coin, the caretaker and the robot makers
- [ ] A lawyer's check of the licensing setup
- [ ] Packaged builds and store pages
- [ ] Add a release engineer agent (`vestige-release`) before the first public build: it packages and checks builds (cook warnings, Shipping-only problems, save compatibility, the licensing files that must ship with the build) and keeps the release checklist. Made like the other agents, with @DunderGG's yes on its instructions (asked by @DunderGG, 2026-11-12)

## 7. Open decisions

Collected from the other documents. Each one is settled in its own document and decision log.

- **Project rules from the code review:** the five open *(decide)* rules in [section 3](#3-housekeeping) (the layer order, the performance budget, prototyping before a guide, building with editor assets, one class prefix), to decide in one sitting. ([CodeReview.md](CodeReview.md#project-rules-that-are-themselves-wrong))
- **Land health:** one value per map or varying across it, how fast it changes, and whether damage spreads. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Energy balance:** how much charge panels give and machines use. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Mutated crops:** which effects are good and which are risky, and how often normal crops mutate. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Knock-out penalty:** what exactly is lost. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Multiplayer:** shared farm or separate farms, shared clock and sleep. ([GameDesign.md](GameDesign.md#15-open-questions))
- **A pet:** whether there is one, and whether it is a robot or a mutated animal. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Fishing:** out, after the first release, or part of the first release's gathering loop. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Names:** the game, the world, the town, the coin, the caretaker, the robot makers. ([Lore.md](Spoilers/Lore.md#14-open-questions))
- **Story:** the sibling's survival path, role and whether the story ever says who struck first. ([Lore.md](Spoilers/Lore.md#14-open-questions))
