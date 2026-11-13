# Project Vestige agent instructions

Instructions for AI coding agents working in this repository. Humans can read it too; it is a short summary of how the project works and the rules to follow.

## Project

- A cozy 3D farming and life-sim game, currently a small playable prototype.
- Unreal Engine 5.8, C++ first, Blueprints for content. Art made in Blender.
- Single game module `Vestige` in `Source/Vestige/`, built from the Third Person C++ template, with its code in folders by architecture layer. Include our headers by folder path (`#include "Farming/FarmGrid.h"`).
- Main level: `/Game/Maps/L_Farm`. Our assets live under `Content/Vestige/`. Blender sources in `Art/Blender/`, exports in `Art/Export/`.
- Developed by one person (David) on Windows, in `C:\Unreal Projects\Vestige`. Open the editor via `Vestige.uproject`.

## Read first

- **[Docs/CodingGuidelines.md](Docs/CodingGuidelines.md)** is the coding standard: how code looks. It is based on Epic's C++ Coding Standard and clean code principles.
- **[Docs/Architecture.md](Docs/Architecture.md)** is the design: principles, where things go in Unreal's framework, who owns what, how systems talk. Its decision log is **[Docs/DecisionLog.md](Docs/DecisionLog.md)**: every design choice with its reasons.
- **[Docs/Systems.md](Docs/Systems.md)**: the index of the system guides, best practice per game system (saving, content, input, UI, the Gameplay Ability System, AI, inventory...) from Epic's documentation and the Lyra sample, one file per system in `Docs/Systems/`. Read the guide for every system a change touches.
- **[Docs/Roadmap.md](Docs/Roadmap.md)**: what's done and what's next. Tick items in the same commit that finishes them.
- **[Docs/PlayTest.md](Docs/PlayTest.md)**: what David checks by hand in the editor: pending checks for recent changes and the smoke test.

- **[LICENSING.md](LICENSING.md)**: code is GPL-3.0-or-later with an Unreal Engine linking permission, assets are CC BY-NC-SA 4.0, Epic template material is under the UE EULA. New source files start with the SPDX license header shown there. Never add third-party code or assets without checking they are under a license CONTRIBUTING.md accepts, and record their source and license in CREDITS.md and the path table in LICENSING.md.
- **[CONTRIBUTING.md](CONTRIBUTING.md)** covers setup, art and audio pipelines, and the workflow for outside contributors.

Use [Docs/CodingGuidelines.md](Docs/CodingGuidelines.md) and [Docs/Architecture.md](Docs/Architecture.md) as the baseline for every change. Also follow the relevant system guide in [Docs/Systems.md](Docs/Systems.md), update [Docs/Roadmap.md](Docs/Roadmap.md) when completing a listed item, and add required manual checks to [Docs/PlayTest.md](Docs/PlayTest.md). The hard rules below summarize the requirements most often missed; the linked documents provide the full guidance. Several of them are long, so read the sections a change needs, found by their headings, rather than whole files.

## Hard rules

1. **Ownership.** Every actor, component and subsystem has one job. Put new logic, components and settings on the class that owns that responsibility, never on whichever class is convenient. If no class fits, create a new actor, component or subsystem and say so. The owners are listed once, in `Docs/Architecture.md`, section 13, one row per class with what it doesn't own: read it before deciding where something goes, and give a new owner its row there.
2. **Epic naming.** `A`/`U`/`S`/`F`/`E`/`I`/`T` type prefixes, `b` for booleans, PascalCase everywhere, and the `Vestige` project prefix on every new type (`AVestigePlayerState`, `FOnVestigeInventoryChanged`). Types about the farm itself (tiles, grid, crops, farming actions) use `Farm` instead (`AFarmTile`, `EFarmTileState`). The general types were renamed from `Farm` on 2026-11-01; a rename of a type that assets or saves name needs `[CoreRedirects]` and a test (guidelines section 2.2).
3. **No shadowing.** The build treats MSVC C4458 as an error. Never give a local or parameter the name of a member of the class or any engine base class (`Mesh` in a character, `Owner`, `Controller`, `Tags`...).
4. **Formatting.** Tabs, braces on their own lines, braces on every block. Format changed files with the repo's `.clang-format`; don't reformat untouched files.
5. **UObject pointers.** Strong references are `UPROPERTY() TObjectPtr<T>`; non-owning references are `TWeakObjectPtr<T>`, assets loaded on demand are `TSoftObjectPtr<T>` (guidelines section 7). Check pointers that can be null. No `new`/`delete` on UObjects.
6. **Encapsulation.** Runtime state is private with getters. Don't add public mutable data members. Expose to Blueprints only when a Blueprint needs it or as a deliberate, commented modding extension point.
7. **Communicate through delegates and small functions**, not by reaching into another class's state.
8. **Keep it simple.** Small functions, early returns, named constants instead of magic numbers, no duplicated logic, no commented-out code, no new debug-only on-screen messages. Don't add Lyra machinery (Experiences, Game Features, Common UI) or replication unless asked. The Gameplay Ability System is adopted, starting with stamina, and is built only as `Docs/Systems.md` section 9 describes.
9. **Dependencies point one way.** Game rules never call UI, audio or effects; presentation reads state and listens to events. No circular dependencies between classes. The source folders are layers (`Core`, `AbilitySystem`, `Interaction`, `Content`, `Time`, `Inventory`, `Farming`, `Save`, `Player`, `UI`, `Game`, lowest first, with Epic's `Template` outside the layers): a lower folder never includes a higher one, new files go in the folder of their layer, and the test `Vestige.Architecture.IncludeDirection` enforces it (guidelines section 14.1).
10. **Record design decisions.** A change that adds a system, moves a responsibility or makes a design choice updates `Docs/Architecture.md` (Part B) and the decision log in `Docs/DecisionLog.md` in the same commit. A log entry is about 100 words: the decision, why, and each rejected option in a line, linking the system's guide for the detail. A choice with no real alternative (a Details category, a file name) goes in the commit message only.
11. **Design for mods.** Content (crops, tools, items) is data assets found through the Asset Manager, never hard-coded lists or enums. Content has namespaced IDs (`Base.Parsnip`). Saves store IDs and skip unknown content. See `Docs/Architecture.md`, section 12.
12. **Performance is a requirement.** Ticking off by default; use events and timers instead of per-frame polling. Nothing in `Tick`, timers or UI bindings searches the world, loads assets, spawns, creates `UObject`s or rebuilds unchanged strings. Content is soft-referenced and never loaded synchronously during play. No debug drawing as a feature. Expensive loops and new-day work get a `TRACE_CPUPROFILER_EVENT_SCOPE`. Optimizations that cost readability need a measurement. Rules in `Docs/CodingGuidelines.md` section 12, budget in `Docs/Architecture.md` section 10.
13. **Follow the system's guide.** Each game system follows its guide in `Docs/Systems/`. A system without a guide gets one first, written from Epic's documentation and Lyra's source and approved before any code (by the reviewer for technical choices, by David for game design; see "How work is done"). Adopting an engine framework (Gameplay Ability System, Common UI, StateTree, Mass) is a decision log entry.
14. **Tests for rules.** Game rule changes get CQTest tests in `Source/Vestige/Tests/`, named `Vestige.<Area>.<Behavior>`. A bug fix gets a test that would have caught it. How to write and run them is in guidelines section 13.

## How work is done

Every change to code or content goes through specialist roles, in this order. "The user" is the person the agents work for: usually David, the maintainer, and sometimes an outside contributor. Most people working on the project, David included, are new to game development, so the roles do the technical judging and the user is asked only about game feel and game design.

0. **Game design: the game designer,** when the task adds or changes how the game plays, unless `Docs/GameDesign.md` and the user's own words already settle everything it needs. Checks that `Docs/GameDesign.md` covers the feature, and prepares each open game feel or design question with options and a recommendation for the user. The user decides, except what the designer may decide itself within the maintainer's limits ("What the user is asked"); the designer records both in `Docs/GameDesign.md`, and the architect designs from them.
1. **Design: the architect.** Names the owner of each new responsibility and its layer, writes or updates the system's guide in `Docs/Systems/`, the `Docs/Architecture.md` changes (ownership table) and the `Docs/DecisionLog.md` entry, and plans the tests and the manual checks.
2. **Design review: the reviewer.** A senior Unreal C++ reviewer, independent of the author, checks the design against Unreal practice, the engine source, Lyra and these rules. The architect answers every finding and the reviewer reviews again, until it approves. When the user is David, the reviewer's approval counts as his on technical choices, within the limits below (decision log, 2026-11-11).
3. **Build: the implementer.** Writes the C++ the approved design calls for and builds it clean. A design that turns out not to work goes back to steps 1 and 2; the implementer doesn't redesign.
4. **Tests: the test author.** Writes the CQTest tests for every changed rule, builds, runs the whole `Vestige` suite, checks what it can itself by running the game off-screen (the log, screenshots it looks at), and adds to `Docs/PlayTest.md` only what still needs a person, saying why.
5. **Code review: the reviewer** again, on the diff, the tests and the docs. Findings go back to the implementer or the test author, and the reviewer re-reviews until it approves.
6. **Commit: the main session.** Checks that `VestigeEditor` built clean and the whole `Vestige` suite passed after the last source change (reading the log, not a summary; if a fix round changed code, build and run them again), runs the checklist in `Docs/CodingGuidelines.md` section 17, and commits only this task's files. The reply to the user lists the technical decisions taken, a line each, so they can question any of them.

**The main session owns the loose ends:** it passes each step what it needs (including the planned commit subject, for `Docs/PlayTest.md`), updates `Docs/Architecture.md` when the implementer had to deviate from the design, turns review findings left for later into `Docs/Roadmap.md` items, and has the game designer record the maintainer's design answers and its own bounded decisions in `Docs/GameDesign.md` (an unanswered question goes into its section 15, and the code built on the recommended option is kept cheap to change). Steps that run the Unreal editor or the build (`Build.bat`, `UnrealEditor-Cmd`) run one at a time, never in parallel: they lock the module and the packages they load.

**Art** is made by the artist (Blender models, export, import, licensing records, preview renders) and reviewed by the art reviewer, a technical artist, who looks at the renders, measures the mesh and reads the scripts, and reviews again after every fix until it approves. Materials and textures are art too. Any change to code, a data asset, a Blueprint, a map or config that uses the art goes through the reviewer as usual, even when the artist's script made it. A change that needs both art and code runs the artist and the art review alongside steps 3 to 5, except for their editor steps.

**A bug fix** starts with the test author writing a test that fails because of the bug, then the implementer fixes it, then the code review.

**Smaller changes** skip what they don't need. A change that doesn't change how the game plays, or whose design is already settled, skips step 0. A change that makes no design choice (no new owner, interface, data, save format or engine feature) skips steps 1 and 2, but still gets tests when a rule changes and the code review. A **small task** (no design choice, a few lines in one or two files, such as a tuning value the user already chose, a fix in one function or a stale doc line) is one agent doing the change and one code review: the implementer for code, the test author for tests, the main session for docs. A bug fix still starts with its failing test. When the change touches a rule, the test author adds or updates its test and runs the suite (or the implementer reports that no rule changed); if no agent ran the suite, the main session runs it before committing, and adds any manual check the change needs. Changes to lore, story and other writing that isn't a rule or a design, and typo fixes, skip the flow. When in doubt, run the full flow.

**What the user is asked:** game feel and game design: how something plays, looks, sounds or reads to the player, tuning numbers that set the feel, player-facing texts, controls, art direction, scope and priorities. Each question comes with the options and a recommendation, prepared by the game designer, and work continues on the recommended option while the user decides. A design question raised at a later step (by the architect, a reviewer or the artist) goes to the step-0 game designer if it ran, continued rather than started again; otherwise the main session puts it to the user as written and records the answer in `Docs/GameDesign.md` in a line. Within limits the maintainer set (2026-11-13), the game designer decides some of these itself and records them; the main session copies its list into the reply to the user, so the maintainer can overrule any of them. The limits: one tuning value per decision, within about 25% of the value the maintainer last set or approved (moving several values of one loop at once goes to the user); values for a new item within the range of its siblings; controls by common convention (the platform's, Lyra's defaults, what games of this kind usually use); functional interface text (labels, prompts, system messages); and small fixes that keep `Docs/GameDesign.md` consistent. For a contributor, these too go into the pull request. Writing with a voice (dialogue, item descriptions, lore, names) always goes to the user. Technical choices are made by the architect and the reviewer and recorded in the decision log with their reasons, so the maintainer can overrule one later.

**What still needs the maintainer's yes** (David's), however technical, because agents shouldn't approve changes to their own rules or take risks he can't see:

- changes to what these hard rules, `Docs/CodingGuidelines.md`, this workflow or the agents' instructions require. Recording what the docs already ask agents to record isn't one: a new test pitfall, a known deviation with its roadmap item, the ownership table in `Docs/Architecture.md` section 13 kept in step with the code;
- licensing, credit and anything legal (`LICENSING.md`, `CONTRIBUTING.md`, third-party or AI-made material);
- anything that costs money or adds an account or service;
- anything that can't be undone: deleting assets or files without a copy, breaking save files once players have them, and pushing to the project's repository (he pushes himself).

**Who the user is:** treat the user as a contributor unless they say they are the maintainer or `git config user.email` is his (the address in "Git" below). Never set the commit author or committer yourself; they come from the user's own git config.

**Working for a contributor,** the flow is the same, but nothing in it stands in for the maintainer: the reviewer's approval is the review before the pull request, game design choices and the items above are raised in the pull request, and the git rules are those in `CONTRIBUTING.md`, not the ones below. Agents never open the pull request, never tick its contributor terms (a legal agreement only the contributor can make) and never claim a play-test they didn't do. They give the contributor a draft description, the points to raise and the manual checks, and the contributor pushes to their own fork or branch.

## Building and testing

- C++ can only be compiled on a Windows PC with Unreal Engine 5.8 installed. Cloud containers have no engine.
- If you can build (for example in a session on the user's PC), build the `VestigeEditor` target (Development Editor, Win64) before committing and fix every error and warning you introduced.
- Run the `Vestige` automation tests when you can build.
- If you can't build, say clearly that the change is uncompiled.
- **A change that needs a manual check** adds it to `Docs/PlayTest.md`, section "Pending checks", in the same commit: a dated heading, the commit and what to look for. Manual checks are kept few: only what neither an automated test nor an agent's off-screen check (the log, screenshots) can judge, such as feel, fun, input on real hardware and a new look the first time. Game rules are always tested automatically, never by a manual check only.
- Live Coding can't add new `UPROPERTY`s, `UFUNCTION`s or classes, and a build can't run while the editor has the project open. Ask the user to close the editor before such a change. When you built after your last source change, tell them the editor is safe to open; ask them to rebuild only when you couldn't build.

## Git

These rules are for agents working for David, the maintainer. If you are helping another contributor, follow the pull request workflow in [CONTRIBUTING.md](CONTRIBUTING.md) instead, with that contributor as the author.

- **Do not open pull requests and do not push.** Commit locally on `main` on David's machine; David reviews and pushes himself.
- **Author and committer: `David Bennehag <david.bennehag@gmail.com>`**, as his git config already sets them.
- **No `Co-Authored-By:`, `Claude-Session:` or any other tool attribution** in commit messages, even if your tooling asks for it.
- Small, focused commits with an imperative subject line and a short body explaining what and why. Moves and refactors in separate commits from behavior changes.
- Binary files go through Git LFS (see `.gitattributes`). Never commit `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` or `.vs/`.

## Working with David

When the user is David, the maintainer. Working for a contributor, apply the same skepticism to their ideas.

- David is an experienced C++ programmer who is new to Unreal Engine and Blender. Don't explain C++; do explain Unreal specifics (editor menus, settings, why an engine pattern is used) concretely.
- **Be skeptical of David's suggestions, ideas and comments** (his words, 2026-11-11: he has no game development experience and needs agents that don't just agree). Treat each one as a proposal to check, not an instruction to carry out: question everything you think is wrong, risky or not how Unreal games are normally built, give the reasons and the better way forward, and say plainly when something he takes for granted is a mistake. Agreeing is fine only when you checked and he is right; say what you checked. Don't soften a disagreement because he sounds sure, and don't invent objections to look critical. Once he has heard the case and still decides, follow his call and record it (in the decision log for a design choice).
- Before doing work that touches several systems, say which class will own the new responsibility.
- Game presentation preferences: 24-hour clock (06:40, 21:30), and properly dark nights.
- Don't name other games in docs, code comments or commit messages.
