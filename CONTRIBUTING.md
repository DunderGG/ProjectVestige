# Contributing to Project Vestige

Thanks for wanting to help. Project Vestige is a cozy 3D farming and life-sim game, built with Unreal Engine 5 and Blender. It is an early prototype made by one developer, so there is plenty to do and plenty of room to shape it.

This guide covers how to get the project running, how it is organized, and how to contribute code, art, audio and ideas.

> **Licensing in short:** code is GPL-3.0-or-later with a permission for Unreal Engine, assets are CC BY-NC-SA 4.0 (no commercial use except by the maintainer), and the maintainer (David, [@DunderGG](https://github.com/DunderGG)) sells official builds of the game whose code stays under the GPL. Contributors are credited. Read [Licensing and credit](#11-licensing-and-credit) before your first contribution.

Contents

1. [Ways to help](#1-ways-to-help)
2. [Getting set up](#2-getting-set-up)
3. [Finding your way around](#3-finding-your-way-around)
4. [Contributing code](#4-contributing-code)
5. [Contributing art](#5-contributing-art)
6. [Contributing audio](#6-contributing-audio)
7. [Adding content the data-driven way](#7-adding-content-the-data-driven-way)
8. [Ideas, bugs and feedback](#8-ideas-bugs-and-feedback)
9. [Git workflow and pull requests](#9-git-workflow-and-pull-requests)
10. [Working with AI tools](#10-working-with-ai-tools)
11. [Licensing and credit](#11-licensing-and-credit)
12. [Be kind](#12-be-kind)

---

## 1. Ways to help

| Area | Examples | Skills |
|---|---|---|
| **Gameplay code** | New tools, crops that need special rules, an inventory, shops, NPCs | C++, Unreal Engine |
| **3D art** | Crops at each growth stage, tools, buildings, fences, trees, props | Blender, low-poly modeling |
| **Materials and textures** | Soil, grass, wood, stylized materials | Unreal materials, texturing |
| **Animation** | Tool swings, watering, harvesting | Blender or Unreal animation |
| **UI** | HUD, inventory, menus, icons | Unreal UMG/Slate, 2D art |
| **Audio** | Footsteps, tool sounds, ambience, music | Sound design, composing |
| **Content and balancing** | New crops and items defined as data, prices, growth times | Unreal editor, no code needed |
| **Testing** | Playing builds and reporting bugs, writing automated tests | Patience, C++ for tests |
| **Documentation** | Fixing unclear docs, writing how-tos | Writing |
| **Mods** | Building content as a mod, to test that the game is moddable | Unreal editor, Blueprints |

Not sure where you fit? Open an issue and say what you'd like to do.

## 2. Getting set up

**[Docs/Setup.md](Docs/Setup.md)** walks you through it step by step on a fresh Windows PC: Git and Git LFS, cloning, Unreal Engine 5.8, Visual Studio with the components the engine needs, the first build, the automated tests, the formatter, Blender, and fixes for common first-build problems.

In short, you need Windows 10 or 11, Git with **Git LFS**, Unreal Engine **5.8** (never convert the project to another version), and Visual Studio 2026 with the components in `.vsconfig`. Visual Studio is needed even for content work, because the editor compiles the project's C++ module the first time it opens it.

### Controls

| Key | Action |
|---|---|
| WASD, mouse | Move, look |
| Space | Jump |
| 1 / 2 / 3 / 4 | Hoe / seeds / watering can / hand |
| E | Use the current tool on the tile in front of you |
| F | Use what is in front of you (the bed sleeps to the next day, and the game saves) |

Till the grass, plant seeds, water them each day, sleep, and harvest when the crop is ripe.

## 3. Finding your way around

```
ProjectVestige/
    Vestige.uproject        The Unreal project
    Source/Vestige/         Game C++ code (one module, in folders by layer; see CodingGuidelines.md 14.1)
    Content/                Unreal assets
        Farm/               Our game assets (meshes, materials, Blueprints)
        Maps/               Levels (L_Farm is the main one)
        Characters/, ThirdPerson/, Input/
                            From Epic's Third Person template
    Art/
        Blender/            Blender source files and scripts
        Export/             FBX files exported from Blender, ready to import
    Config/                 Engine and project settings
    Docs/                   Project documentation
```

Read these before making larger changes:

- **[Docs/Architecture.md](Docs/Architecture.md)**: how the game is structured, which class owns what, the design rules, and why decisions were made. Its *Known gaps* and *Expected changes* sections are a good list of things that need doing.
- **[Docs/CodingGuidelines.md](Docs/CodingGuidelines.md)**: how code should look, naming for code and assets, testing.
- **[Docs/Systems.md](Docs/Systems.md)**: how each game system is built, following Epic's recommendations and the Lyra sample. Read the guide for the system you're working on.

## 4. Contributing code

1. **Talk first for anything bigger than a small fix.** Open an issue that describes the change and which class or system would own it. It saves you from building something that doesn't fit.
2. **Follow the architecture.** The most important rule: every actor, component and subsystem has one job, and new logic goes on the class whose job it is. If nothing fits, propose a new class. See [Architecture.md](Docs/Architecture.md).
3. **Follow the coding guidelines.** Epic's naming (`AFarm...`, `UFarm...`, `bIsSomething`), tabs and braces on their own lines, small functions, no magic numbers. The repo's `.clang-format` handles formatting: enable *ClangFormat support* in Visual Studio and format the files you change.
4. **Keep it fast.** Performance is a requirement: no ticking without a reason, no per-frame polling or searching, soft references for content and no loading during play. See [Performance](Docs/CodingGuidelines.md#12-performance) and the [budget](Docs/Architecture.md#10-performance-by-design).
5. **Build for mods.** Content (crops, tools, items) is data, not hard-coded lists or enums. See [Designing for mods](Docs/Architecture.md#12-designing-for-mods).
6. **Test it.** Game rule changes come with automated tests (CQTest, in `Source/Vestige/Tests/`). Run the `Vestige` tests from the editor's Test Automation window, and play the change in the editor, using the checks in [Docs/PlayTest.md](Docs/PlayTest.md). If your change needs a manual check, add it to that file's "Pending checks" in the same pull request.
7. **No new warnings.** The build treats some warnings as errors, for example a local variable that hides a member (`Mesh` inside a character class).
8. **Update the docs.** If your change adds a system, moves a responsibility or makes a design choice, update `Docs/Architecture.md` (the ownership table) and add an entry to `Docs/DecisionLog.md` in the same pull request.

## 5. Contributing art

### Style

Low-poly and stylized, with simple shapes and flat or lightly textured colors. Look at the existing crops and the watering can in `Content/Vestige/Meshes` for reference. Readability from the game camera matters more than detail.

### From Blender to Unreal

1. **Scale:** Blender in metric with unit scale 1.0, so 1 Blender meter is 1 Unreal meter (100 Unreal units). A farm tile is **1 m × 1 m**.
2. **Orientation:** Z up, and the front of the object facing **+X**.
3. **Origins (pivots)** are where the object touches the world or is held:
   - Soil and ground pieces: bottom center.
   - Crops: center of the base, on the soil surface.
   - Held tools: the point where the hand grips.
4. **Apply transforms** (rotation and scale) before exporting.
5. **Export FBX** to `Art/Export/` with the same name the asset will have in Unreal, using *Forward: X*, *Up: Z* and *Apply Scalings: FBX Units Scale*.
6. **Save the `.blend` source** in `Art/Blender/`. Every imported mesh should have its source in the repo so others can change it.
7. **Import** into `Content/Vestige/` in the folder for its type (`Meshes`, `Materials`, `Textures`...).

`Art/Blender/make_farm_assets.py` generates the current crop and tool meshes from a script and is a working example of these settings.

### Naming

Follow Epic's [recommended asset naming](https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects): `SM_` static mesh, `SK_` skeletal mesh, `M_` material, `MI_` material instance, `T_` texture, `BP_` Blueprint and so on. Variants go at the end: `SM_Crop_Stage0`, `SM_Crop_Ripe`. The full table is in [CodingGuidelines.md](Docs/CodingGuidelines.md#142-content-and-asset-naming).

### Budgets

Keep things light so big farms stay fast: a crop or small prop in the hundreds of triangles, a tool under about 2,000, a building under about 10,000. Textures 1024 × 1024 or smaller unless there's a reason. Going over one needs a reason in the pull request. Small props and crops also don't cast dynamic shadows unless they visibly need to, and swaying plants keep their movement subtle and switch it off at a distance. The reasons are in [Architecture.md, Performance by design](Docs/Architecture.md#10-performance-by-design).

## 6. Contributing audio

- Deliver **WAV**, 16-bit or 24-bit, 44.1 or 48 kHz. Mono for sounds placed in the world, stereo for music and ambience.
- Name sound waves `S_<Name>` and sound cues `SC_<Name>`, and put them in `Content/Vestige/Audio/`.
- Keep the source project files (DAW sessions, raw recordings) out of the repo unless they're needed to edit the sound later. Ask first if they are large.

## 7. Adding content the data-driven way

The goal is that adding a crop, tool or item means adding data, not changing C++. That's also how mods will add content.

Each crop, tool and item is a **data asset**: an asset in the Content Browser whose fields describe it. The game finds them through Unreal's *Asset Manager*, which scans set folders at startup, so the code holds no list of crops to edit. The design is in [Architecture.md, Designing for mods](Docs/Architecture.md#12-designing-for-mods).

### Adding a crop

1. **Make the meshes**, one per growth stage, from the day it's planted to ripe: a crop that takes 3 days has 4 meshes. Follow [Contributing art](#5-contributing-art), with the origin at the center of the base on the soil surface, and name them after the crop: `SM_Crop_Carrot_Stage0` ... `SM_Crop_Carrot_Ripe`. Import them into `Content/Vestige/Meshes`.
2. **Create the data asset.** In the Content Browser, open `Content/Vestige/Crops`, right-click and choose *Miscellaneous > Data Asset*, pick **Farm Crop Data**, and name it `DA_Crop_Carrot`. It has to be in `Content/Vestige/Crops` (or a folder inside it): that is the folder the Asset Manager scans for crops (`Config/DefaultGame.ini`).
3. **Fill in its fields** (double-click the asset):

   | Field | What to enter |
   |---|---|
   | Content Id | `Base.Carrot`: the namespace `Base` for the game's own content, a dot, and the crop's name, each starting with a capital letter, letters and digits only. Saves store this ID, so it **never changes** once the crop is in a release. It doesn't follow the asset's name, so renaming or moving the asset later is safe |
   | Display Name | The name the player sees, such as *Carrot* |
   | Days To Mature | Watered days until it can be harvested. It grows one stage per watered day |
   | Growth Stage Meshes | The meshes from step 1, in order, one more than *Days To Mature* |
   | Harvest Item Id | The item a harvest gives, such as `VestigeItem:Base.Carrot`. Make that item first, as in [Adding an item](#adding-an-item); prices belong to it |
   | Yield | How many of that item one harvest gives |
   | Seasons, Water Need, Contamination Tolerance | Fill in what you intend. The game doesn't use them yet; they are there so crops don't need changing when seasons and contamination arrive |

4. **Save it and check it.** Right-click the `Content/Vestige/Crops` folder and choose *Validate Assets*: a missing or badly formed Content Id, or a Harvest Item Id that names no item, shows as an error. Restart the editor and check the Output Log for an "is used by more than one asset" error, which means another asset already has your ID.
5. **Try it in the game.** Seeds are planted from the item in hand, so make a seed item for your crop first ([Adding an item](#adding-an-item)): besides its **Farm Seed Fragment** naming your crop, it needs a **Vestige Tool Fragment** whose *Tool Id* is `VestigeTool:Base.Seeds`, the Seeds tool every seed item uses. Then open `Content/Vestige/Blueprints/BP_PlayerState`, select its **Inventory** component and add your seeds under **Starting Items**; the first nine slots are the hotbar. Start a new game with them: while playing, open the console and type `NewGame` ([PlayTest.md](Docs/PlayTest.md#1-before-you-test) says how to open it). Till a tile with the hoe, press the number key of your seeds' slot (or scroll to it) and press E on the tilled tile. Change the starting items back before you commit, unless your pull request is meant to change them.
6. **Open a pull request** with the data asset, the meshes, their `.blend` source and a screenshot of each growth stage in the game.

### Adding a tool

Tools work the same way: a **Vestige Tool Data** asset named `DA_Tool_<Name>` in `Content/Vestige/Tools`, with a Content Id such as `Base.Sickle`, a held mesh (origin where the hand grips), and an *Ability* chosen from the existing tool abilities (`FarmGameplayAbility_Till`, `_Plant`, `_Water`, `_Harvest`), which is what the tool does. The player holds a tool as an item: make an item for it ([Adding an item](#adding-an-item)), such as `DA_Item_Sickle` with *Max Stack Size* 1, with a **Vestige Tool Fragment** whose *Tool Id* names your tool (`VestigeTool:Base.Sickle`). To try it, add that item under **Starting Items** on the **Inventory** component of `Content/Vestige/Blueprints/BP_PlayerState` and start a new game, as in step 5 above; the first nine slots are the hotbar, on the keys 1 to 9. A tool that does something new is a new tool ability in C++ (a subclass of `UVestigeGameplayAbility_UseTool`, [Systems.md, section 9](Docs/Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system)), so open an issue first.

### Adding an item

Items are what the player carries: seeds, harvested crops, materials. Each is a **Vestige Item Data** asset named `DA_Item_<Name>` in `Content/Vestige/Items`, with a Content Id such as `Base.Carrot` (an item and a crop may share a name; they are different kinds of content), a display name, *Max Stack Size* (1 for items that don't stack) and *Sell Price*. What only some items have is a **fragment**: under *Fragments*, press + and pick a fragment class. A seed item has a **Farm Seed Fragment** whose *Crop Id* names the crop it plants (`FarmCrop:Base.Carrot`) and a **Vestige Tool Fragment** naming the Seeds tool (`VestigeTool:Base.Seeds`), which is what plants it. So a new crop usually comes with two items, its seeds and its harvest. Validate the `Content/Vestige/Items` folder afterwards as in step 4 above. The player carries items in the inventory on the player state; the starting items are set on `BP_PlayerState`. A kind of item that needs a new fragment needs C++, so open an issue first.

### Mods

Mods will add content the same way, with their own namespace instead of `Base` (`MyMod.BlueCorn`). How a mod is packaged and loaded isn't built yet; see [Architecture.md, Designing for mods](Docs/Architecture.md#12-designing-for-mods).

## 8. Ideas, bugs and feedback

- **Bugs:** open an issue with what you did, what you expected, what happened, and the engine version. Attach the log from `Saved/Logs/Vestige.log` and a screenshot if it helps.
- **Ideas:** open an issue describing the idea and why it would make the game better. Small, concrete ideas are easiest to act on.
- **Questions:** open an issue too. If something in the docs confused you, that's a bug in the docs.
- **Security problems:** report them privately, not as an issue. See [SECURITY.md](SECURITY.md).

## 9. Git workflow and pull requests

1. Fork the repository, or create a branch if you have write access. Name the branch after the change: `add-fence-meshes`, `fix-watering-twice`.
2. Make **small, focused commits** with clear messages: an imperative subject line under about 70 characters ("Add fence meshes"), a blank line, then what changed and why.
3. Keep moves and renames in their own commits, separate from changes in behavior.
4. **Never commit** `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/` or generated `.sln` and `.slnx` files. `.gitignore` handles this; don't force-add them.
5. **Binary files go through Git LFS.** `.gitattributes` covers the common types. If you add a new binary file type, add it to `.gitattributes` in the same commit.
6. **Avoid editing the same binary asset as someone else.** `.uasset` and `.umap` files can't be merged. Say in your issue which assets and levels you'll be changing, especially `L_Farm`.
7. Open a **pull request** against `main`. Describe what it changes and how you tested it, and include screenshots or a short clip for anything visual.
8. The maintainer reviews every pull request. Expect questions and requests for changes; they're about keeping the project consistent, not about you.

Before opening a pull request, check:

- [ ] It builds, and the `Vestige` tests pass.
- [ ] You played it in the editor and it works.
- [ ] Code follows the [coding guidelines](Docs/CodingGuidelines.md#17-review-checklist) and the [architecture](Docs/Architecture.md).
- [ ] Assets are named correctly, and their Blender sources are included.
- [ ] Docs are updated if the design changed.
- [ ] Third-party material is listed with its source and license (see [Third-party material](#third-party-material)), and new source files start with the license header from [LICENSING.md](LICENSING.md#license-notices-in-files).
- [ ] You agree to the [contributor terms](#contributor-terms).

## 10. Working with AI tools

You're welcome to use AI coding assistants. [AGENTS.md](AGENTS.md) (and `CLAUDE.md`, which loads it) gives them the project's rules, and its section "How work is done" splits the work into roles: a game designer, an architect, a reviewer, an implementer, a test author, an artist and an art reviewer. With Claude Code these roles are ready-made subagents in `.claude/agents/`; [.claude/README.md](.claude/README.md) shows how they work together. Their review is not the maintainer's: it happens before your pull request, which is still reviewed as usual. You are still responsible for what you submit: read and understand every change, build it and test it before opening a pull request.

## 11. Licensing and credit

Project Vestige's code is licensed under the **GPL-3.0-or-later** with an [additional permission](LICENSE-EXCEPTION.txt) that allows linking with Unreal Engine and platform SDKs. Its assets and documentation are under **CC BY-NC-SA 4.0**, which doesn't allow commercial use. The maintainer sells official builds of the game, and their code stays under the GPL. The full picture, and which license applies to which folder, is in [LICENSING.md](LICENSING.md).

### Contributor terms

These terms are version 2, dated 2026-10-28. They apply to every pull request to this repository. Contributions are accepted only as pull requests; please don't send code or assets by email, chat or issue attachment. Ideas and suggestions in issues and discussions are welcome, and aren't contributions under these terms. If one turns into code or art, it comes in as a pull request.

When you open a pull request, you agree that:

1. **You have the right to submit it.** The contribution is your own work, or you have the right to submit it under these terms (for example, your employer allows it). You have listed every third-party part with its source and license, as described under [Third-party material](#third-party-material).
2. **It is licensed to everyone** under the license for the part of the repository it goes into: GPL-3.0-or-later with the additional permission in [LICENSE-EXCEPTION.txt](LICENSE-EXCEPTION.txt) for code, and CC BY-NC-SA 4.0 for assets, documentation and everything else. [LICENSING.md](LICENSING.md#which-license-applies-where) says which is which.
3. **The maintainer may use it in official builds.** You also grant David Bennehag, the maintainer, a perpetual, worldwide, non-exclusive, royalty-free, irrevocable license to use, copy, modify and distribute your contribution as part of official Project Vestige builds, including commercial builds sold on game stores and consoles. This includes accepting the terms a store or platform sets for publishing there, and letting people who work on official builds for him (for example a porting studio or a publisher) do the same, under the same limits.
4. **Official builds stay GPL.** The grant in point 3 doesn't let anyone distribute your code under closed-source terms. The Project Vestige code in every official build stays under the GPL with the additional permission, and its source is published in this repository. The grant only adds what CC BY-NC-SA doesn't allow: commercial use of your assets in official builds.
5. **You keep the copyright** to your work and may use it elsewhere however you like.

Tick the contributor terms box in the pull request template. Pull requests are merged only when the box is ticked, and the pull request is the record of your agreement and of the terms version you agreed to. If you submitted something you didn't have the right to submit, say so in the pull request or an issue, and it will be removed.

### Credit

Add yourself to [CREDITS.md](CREDITS.md) in your first pull request. Contributors are also listed in the game's credits.

### Third-party material

Only submit code and assets you made yourself, or third-party material under one of these licenses:

| Kind | Accepted | Not accepted |
|---|---|---|
| Code | MIT, BSD-2-Clause, BSD-3-Clause, Apache-2.0, zlib, BSL-1.0, public domain or CC0 | Any GPL, LGPL or AGPL code (its authors haven't given the Unreal Engine permission, so we couldn't ship builds), and any license with a non-commercial or "no modification" clause |
| Assets (models, textures, fonts, audio, music) | CC0, public domain, CC BY 4.0, and font licenses such as the SIL Open Font License | Anything non-commercial (NC), no-derivatives (ND), ShareAlike under a different license (for example CC BY-SA), "free for personal use", editorial-only, or with an unclear source |
| Documentation | Your own writing; short quotes with a source | Copied text from other sites or books |

For each third-party file:

- Say in the pull request where it came from (a link), who made it and its license.
- Add it to the third-party list in [CREDITS.md](CREDITS.md#third-party) with the same details, and keep any license or attribution file the license requires next to it.
- If you're not sure the license allows a commercial game, don't submit it; ask in an issue first.

**AI-generated material** is treated like third-party material: say which tool made it and what you gave it, and make sure the tool's terms allow use in a commercial game. Never submit anything generated from, or imitating, someone else's protected work. The maintainer may decline AI-generated material whose origin is unclear.

Never submit code or assets ripped from other games.

## 12. Be kind

Be respectful and patient with everyone. People of every skill level are welcome. Critique the work, not the person.
