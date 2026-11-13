# Project Vestige play-testing

The one place for what to check by hand in the editor.

- **[Before you test](#1-before-you-test)**: building, the automated tests, starting the game and handling saves.
- **[Pending checks](#2-pending-checks)**: what recent changes still need you to look at. Work through it after pulling or building new code.
- **[Smoke test](#3-smoke-test)**: a five-minute pass over every system, before pushing and at the end of each milestone.
- **[What isn't manual](#4-what-isnt-manual)**: what the automated tests cover instead.

---

## 1. Before you test

- **New or moved source files:** regenerate the Visual Studio project files (right-click `Vestige.uproject` in Explorer > Generate Visual Studio project files), so the solution shows them.
- **Building:** an agent working on this PC builds `VestigeEditor` (Development Editor, Win64) and runs the tests before every commit, so after its changes the editor is ready to open; the agent says so. Rebuild yourself only after changing C++ yourself, after pulling code nobody built on this PC, or when the editor says the Vestige module is missing or out of date (answer Yes to rebuild it, no Visual Studio needed). For C++ that adds classes, `UPROPERTY`s or `UFUNCTION`s, close the editor and build in Visual Studio; Live Coding can't add those, it only patches function bodies.
- **Run the automated tests first.** In the editor: Tools > Test Automation, filter on `Vestige`, tick it and press Start Tests. Or from a command prompt with the editor closed:

  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Unreal Projects\Vestige\Vestige.uproject" -ExecCmds="Automation RunTests Vestige; Quit" -unattended -nopause -nullrhi -nosplash -testexit="Automation Test Queue Empty"
  ```

  Every test logs `Result={Success}`. Two `LogAutomationTest: Error: Condition failed` lines at startup come from the engine and can be ignored.
- **Start the game:** open `/Game/Maps/L_Farm` (Content Browser > `Content/Maps`, double-click), then Play (Alt+P) for Play in Editor. For what players get, and for any performance numbers, use the arrow next to Play > Standalone Game instead.
- **Graphics settings (scalability):** the quickest way, which works in Play in Editor and in Standalone Game, is the console ([see below](#1-before-you-test)). While playing, open it, type `scalability 2` and press Enter for High, or `scalability 3` for Epic (0 is Low, 1 Medium). Everything switches at once. The project has no hardware ray tracing at any level; typing `r.RayTracing` on its own prints 0. New players start at **High**. The level viewport keeps its own setting between sessions (its Settings menu > Engine Scalability Settings); the console changes it too.
- **Saves** live in `Saved/SaveGames/`: `Farm.sav` and `Farm2.sav` are the two slots saves alternate between. Before testing anything that writes saves, copy the folder somewhere safe, and copy it back afterwards to get your farm back. The console command `NewGame` deletes both slots and starts a new game on day 1. Until the first public build, saves aren't upgraded: when a change makes older saves unreadable, the game says it couldn't read the save and won't save, and `NewGame` starts over.
- **Logs:** Window > Output Log. Each of our systems has its own category (`LogFarm`, `LogVestigeSave`, `LogVestigeFeedback` and the others in [Systems.md, section 21](Systems.md#21-logging)), and each line starts with who wrote it, as in `[FarmGrid_1] AFarmGrid::SpawnTiles: ...`. What each part of a line means (the time, the frame number, the category, the sender) is in [Systems.md, section 21, Reading a line](Systems.md#21-logging). To see only ours, type `LogVestige` or `LogFarm` in the search box, or open the Output Log's **Filters** menu > **Categories** and tick ours. That menu only lists the categories that have written a line since the editor started, so `LogFarm`, which only warns about setup problems, is usually missing from it; the search box always works. To give each category its own colour, open Edit > Editor Preferences > General > Output Log and set **Category Colorization Mode** to **Colorize Whole Line** (chosen 2026-11-02; warnings and errors keep their yellow and red). Both are your own editor settings and aren't committed. To see a system's tracing lines, type `log LogFarm Verbose` in the console (and `log reset` to go back). Note any warning or error even if the game looks fine; the full log of the last run is `Saved/Logs/Vestige.log`.
- **Console**, for `scalability`, `stat fps`, `stat unit`, our developer commands (`NewGame` starts a new game, `SkipDay` sleeps where you stand, `SkipToTime 2130` runs the clock forward to 21:30; [Systems.md, section 19](Systems.md#19-debugging-and-cheats)) and similar commands. While playing, press **§** (the key under Esc on a Swedish keyboard) or the key left of Backspace (´ and `), the US layout's console key. If neither works, press Shift+F1 to free the mouse, click the **Cmd** box at the bottom of the editor (in the Output Log panel, or the status bar at the bottom left), type the command there and press Enter; it goes to the game you are playing. Click back into the game viewport afterwards.

## 2. Pending checks

Checks for recent changes that haven't been confirmed in the editor yet. Each names the commit to look at if something is wrong. When a check passes, tick it (or tell the agent, who ticks it); once every check under a heading has passed, the heading moves to the end of [PlayTestArchive.md](PlayTestArchive.md), unchanged except that its links to this file's own headings (such as `#3-smoke-test`) get the `PlayTest.md` prefix. The agent that edits this file next archives every heading whose checks have all passed, so headings ticked by hand move too. When one fails, write what you saw under it; it is fixed before new work starts.

### Tile targeting, 2026-10-30

Replaces the tile highlight check above: the frame now sits on a neighbouring tile chosen by the movement direction (GameDesign.md, section 6). The tap, walking and feel checks moved to [Smooth turning](PlayTestArchive.md#smooth-turning-2026-10-30) below; the results that led to it are in commit 8932448.

- [x] **Field edge** (ce1f1dc): standing on an edge tile and turning to face outwards hides the frame; turning back shows it again.
- [x] **Tools** (ce1f1dc): the tools act on the framed tile, never on the one the farmer stands on.
- [ ] **Gamepad** (ce1f1dc), if you have one: a flick of the left stick turns the farmer with barely a step, and holding it walks, the same as the keys.
   - Unable to test Gamepad support now. Save this for later, when we have a gamepad to test with.

### Five messages and the message log, 2026-10-30

Raises the cap from the message boxes checks above.

- [ ] **At most five** (77a2218): with five boxes showing, a sixth message pushes out the oldest. Since M4 (2026-11-08) tilling and planting show no message, so press E with the hoe on tilled soil, then on grass with the watering can, and so on (each press shows its reason). The boxes stay clear of the key hints and the status line.
- [ ] **Settings** (77a2218): Edit > Project Settings > Game > Vestige HUD shows Max Messages (5) and Message Seconds (3). Changing them takes effect the next time you press Play, and is saved to `Config/DefaultGame.ini`; tell the agent the values you settle on.
- [ ] **Log** (77a2218): every message also appears in the Output Log (Window > Output Log, filter on `LogVestigeFeedback`) as `[L_Farm] UVestigeFeedbackSubsystem::Notify: ...`, so you can read messages that have already faded. (Updated 2026-11-02 for the logging categories; the line read `LogVestige: Feedback: ...` before. Since M4, 2026-11-08, the line is `UVestigeFeedbackSubsystem::Report:` with the tag, checked under "Feedback reports" below.)

### Item data assets, 2026-10-30

Items exist as data but nothing in play uses them yet, so the game itself should behave exactly as before.

- [x] **Validation** (46bf244): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.
   - Checked in 5.8 on 2026-10-31 by the agent with the `DataValidation` commandlet: all 23 assets under `Content/Farm` are valid, and the editor logs have no duplicate content ID error.
- [ ] **Item assets** (46bf244): `Content/Vestige/Items` has `DA_Item_Turnip` and `DA_Item_TurnipSeeds`. Opening the seeds shows one **Farm Seed Fragment** under Fragments, with Crop Id `FarmCrop:Base.Turnip`; its Crop Id picker lists only crops. Pressing + on Fragments offers the fragment classes to pick from.
- [ ] **Turnip crop** (46bf244): `DA_Crop_Turnip` shows Harvest Item Id `VestigeItem:Base.Turnip` and Yield under Farming > Harvest, and no Seed Price or Sell Price.
- [ ] **No change in play** (46bf244): planting, growing and harvesting a turnip work as in the [smoke test](#3-smoke-test).

### Sky lighting, 2026-11-05

The day cycle actor is renamed `VestigeSkyLighting` for its one job, the lighting. `L_Farm` still names the old class until it is saved, and a redirect in `Config/DefaultEngine.ini` loads it as the new one; a test checks the redirect. The commit is the one titled "Rename the day cycle to sky lighting".

- [ ] **Level**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files) so the solution lists `Time/VestigeSkyLighting.h`. Open `L_Farm`. The Outliner still shows the actor as `FarmDayCycle` (its label), and selecting it shows class `VestigeSkyLighting` in the Details panel, with Sun Step Degrees and the other **Lighting** settings as before. The Output Log has no "can't find class" or "Failed to load" lines for it. Play: the sun, the night and the morning look as before. Save `L_Farm` (Ctrl+S) so it stores the new name, and commit the level.
- [ ] **Sun hours in the clock settings** (commit "Move the sunrise and sunset hours to the clock"): Project Settings > Game > **Vestige Clock** shows **Sunrise Hour** (6) and **Sunset Hour** (20) under **Sun**, and the sky lighting actor in `L_Farm` no longer has a **Time** category. With `slomo 20`, the sun comes up just after 06:00, the light warms and fades out toward 20:00, and the moon takes over after it, as before. For a quick check, set Sunset Hour to 12 for a session: the evening light and the moon arrive at noon. Put it back to 20 and don't commit the change to `Config/DefaultGame.ini`.

### Instanced farm tiles, 2026-11-05

The tile actors are gone: the grid's new meshes component draws the whole field with instanced meshes, and `L_Farm` now places `BP_FarmGrid` instead of the C++ grid with `BP_FarmTile` (commit "Draw the farm tiles with instanced meshes"). The rules and the instance counts are covered by tests; these checks are for how it looks.

- [x] **Level**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files), since `FarmTile.h` is gone and `FarmFieldMeshesComponent.h` and `FarmTileLooks.h` are new. Open `L_Farm`. The Outliner shows the grid as a `BP_FarmGrid` in the same place, with **Field Id** `Base.Farm` and the same width and height under **Farming**, and the Output Log has no warning about `BP_FarmTile` or a missing `FarmTile` class.
- [x] **The art**: open `BP_FarmGrid` (`Content/Vestige/Blueprints`), select its **Meshes** component: **Farming > Visuals** shows `SM_Soil_Tilled`, `M_Soil_Watered`, a soil surface height of 6 and `M_FarmPlaceholder`.
- [x] **Looks as before**: press Play. Grass tiles are green slabs with small gaps; the hoe shows the Blender soil, watering darkens it, the seeds plant a turnip sprout standing on the soil, and it grows a stage each watered day until it is ripe. No tile or crop is grey (a grey tile means a material is missing **Used with Instanced Static Meshes**; the Output Log then says "missing usage flag"). Shadows look as they did.
- [x] **Frame**: the yellow frame still sits on the tile in front of the farmer and follows him, and hides at the field's edge.
- [ ] **Placeholder colors** (optional): select the grid in `L_Farm`, then its **Meshes** component, and clear **Tilled Soil Mesh** on this instance. In Play, tilled soil is a brown slab and watered soil a dark one. Reset the property with its yellow arrow afterwards, and don't save the level with it cleared.
- [x] **`On Tool Used` in `BP_PlayerCharacter`**: the Blueprint compiles. Its **On Tool Used** event's **Tile** pin is now an Int Point (the tile's coordinates) instead of a tile; if anything was wired to the old pin, the Blueprint shows an error there.
- [x] **Big field**: in Play, `BenchmarkField 60 Base.Turnip` makes a 60 by 60 field of ripe turnips at once, without a long freeze.

### Input as assets, 2026-11-07

The farming keys are input assets now: `IA_` actions and `IMC_Farming` in `Content/Vestige/Input`, with `DA_InputConfig_Player` naming the action for each input tag. `BP_PlayerCharacter` adds the moving, looking and farming contexts while it is possessed, instead of the player controller. New: the mouse wheel and the gamepad shoulders step through the hotbar, gamepad X uses the item in hand and B interacts ([Systems.md, section 5](Systems.md#5-input)). Look at the commit "Move the farming input to assets bound by tag" if something is wrong.

- [x] **Keys as before**: E uses the item in hand (holding it repeats and slows the walk), F interacts and harvests, and 1 to 9 select hotbar slots, all as before.
- [x] **Mouse wheel**: one notch down selects the next hotbar slot and one notch up the previous one, wrapping from the last slot to the first and back; empty slots count. One notch is one step. Scrolling fast may skip fewer slots than notches, which is acceptable.
- [ ] **Gamepad**, if you have one: the right shoulder (RB) steps to the next slot and the left (LB) to the previous one, one press one step; X uses the item in hand (holding it repeats); B interacts; A still jumps; the left stick moves and the right stick looks.
- [x] **Key hints unchanged**: the bottom line still names the hotbar keys with their items ("1 Hoe   2 Turnip Seeds ...") and "E Use tool", and the prompt reads "F Harvest" or "F Sleep". No gamepad key shows in any hint.
- [x] **Move, look and jump**: with the keyboard (WASD, arrows, space) and the mouse, as before.
- [x] **The assets**: open `Content/Vestige/Input`. Each `IA_` action is Boolean and has **Player Mappable Key Settings** with a Name such as `Vestige.Input.UseTool` and a display name; `IA_UseTool` and `IA_Interact` have the descriptions "Use tool" and "Interact". `IMC_Farming` lists each action with its keyboard or mouse key first and its gamepad key second (the slots have no gamepad key). `DA_InputConfig_Player` lists thirteen actions, each with its `Vestige.Input` tag, and right-click > Validate Assets on the folder reports no errors. `BP_PlayerCharacter`'s **Input** shows Input Config `DA_InputConfig_Player` and Mapping Contexts `IMC_Default` (0), `IMC_MouseLook` (0) and `IMC_Farming` (1); `BP_PlayerController` has no mapping contexts any more.
- [x] **Contexts don't stack**: while playing, open the console and type `showdebug enhancedinput`. `IMC_Default`, `IMC_MouseLook` and `IMC_Farming` each show once. Then have the farmer possessed again and check that each still shows once. There is no console command that only re-possesses: try F8 twice in Play in Editor (eject, then possess); if `showdebug enhancedinput` shows the contexts the whole time, ejecting didn't unpossess him, so tell the agent, and a `RepossessPlayer` cheat or an automated test takes this check's place ([Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review)).
- [ ] **Packaged build**: a packaged Development build has working input: move, look, jump, E, F, 1 to 9 and the mouse wheel.

### The HUD in UMG, 2026-11-08

The HUD is UMG now instead of C++ Slate: `WBP_Hud` and `WBP_HudMessage` in `Content/Vestige/UI`, named by the new `BP_HUD`, which `BP_GameMode` uses as its HUD class ([Systems.md, section 6](Systems.md#6-user-interface)). It should look and behave exactly as before. The commit is the one titled "Move the HUD to UMG".

- [x] **Solution**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files): the solution lists `UI/VestigeHudMessageWidget.h` and `.cpp` and `Tests/VestigeHudWidgetTests.cpp`.
- [x] **Looks as before**: press Play. Top left, the status panel reads the day, clock and item in hand, then "Stamina 100 / 100" and the bag line, on the dark backdrop; the key hints run along the bottom; facing the bed shows "F Sleep" below the farmer, and a ripe crop "F Harvest". Fonts, sizes, colours, shadows and spacing match what you had. Say if anything moved or changed size.
- [x] **Smaller window**: play in a smaller window (Play > New Editor Window, then resize it): the HUD scales down the same way it used to and nothing overlaps.
- [x] **Messages**: sleep (F at the bed). "You slept. Day N begins." and the messages after it each appear in a box of their own, stacked above the key hints, fade in, and fade out one after another. Harvesting several crops quickly shows at most five boxes.
- [x] **Out of stamina**: keep tilling and watering until "Stamina 0 / 100": the stamina line turns red with the out-of-stamina message, and goes white again after sleeping.
- [x] **The mouse isn't caught**: while playing, the mouse still turns the camera everywhere on screen, including over the status panel and the messages.
- [x] **Designer**: open `Content/Vestige/UI/WBP_Hud`. The Hierarchy shows the overlay with `StatusPanel`, `PromptBox`, `MessageList` and `KeyHintsText`, and the Designer shows the HUD with preview texts. Select `StatusPanel`, change its Brush Color, Compile and press Play: the panel shows the new colour without building. Then put the colour back (or close without saving). `WBP_HudMessage` opens too and shows "Game saved." in its box.
- [x] **Widget Reflector**: while playing, press Ctrl+Shift+W, then **Pick Hit-Testable Widgets** and hover over the HUD: it picks the game viewport, never a HUD widget. **Pick Painted Widgets** over the status line finds `StatusText` inside `WBP_Hud`.
- [x] **No widget class**: open `Content/Vestige/Blueprints/BP_HUD` and clear **HUD Widget Class**, Compile, and press Play: no HUD shows and the Output Log has the error "No HUD Widget Class is set, so the HUD shows nothing". Then set it back to `WBP_Hud` (or close without saving).
- [ ] **Packaged build**: a packaged Development build shows the HUD as in the editor.

### Gamepad key hints, 2026-11-08

The key hints and the prompt name the keys of the device you used last, keyboard and mouse or gamepad, told by Common Input, part of the CommonUI plugin, which is now on ([Systems.md, section 5](Systems.md#5-input), gamepad key hints). The gamepad checks need an Xbox-style (XInput) pad: Unreal on Windows doesn't see plain HID pads such as the HORI Switch pad and the Stadia controller (support for those is a nice to have in [Roadmap.md, Later still](Roadmap.md#later-still)). Without one, do the checks that need no gamepad and leave the rest until one is at hand. The commit is the one titled "Key hints follow the device used last".

- [x] **Solution**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files): the solution lists `Player/VestigeKeyHints.h` and `.cpp`.
- [x] **Editor starts**: the editor opens without asking about plugins, and Edit > Plugins shows **Common UI Plugin** ticked. The Output Log at start has no `LogUIActionRouter` error (one Display line, "Found 0 derived classes", is expected).
- [x] **Keyboard first**: press Play and use only the keyboard and mouse. The hints read as before, "1 Hoe   2 Turnip Seeds   3 Watering Can   |   E Use tool   |   F Interact", and the bed shows "F Sleep".
- [ ] **Switch to the gamepad**: press any gamepad button, a trigger or push a stick all the way. The hints change at once to "LB / RB Hotbar   |   X Use tool   |   B Interact", and facing the bed shows "B Sleep" (a ripe crop "B Harvest"). The gamepad plays exactly as before: X uses, B interacts, the shoulders step the hotbar, A jumps, the sticks walk and look.
- [ ] **Back to the keyboard**: press a key, click, scroll or move the mouse: the keyboard hints come back. Switch back and forth a few times; the hints follow each time and don't flicker while you use only one device.
- [ ] **No cursor**: after using the gamepad, Alt-Tab out of the editor and back, click into the game and move the mouse: the camera turns and no mouse cursor shows. Then use the gamepad and stop Play (Esc): the editor's mouse cursor shows. If it stays hidden, it comes back with the next Play; say so.
- [ ] **Touch screen** (Dunder-YOGA): touch the game view while playing: the keyboard hints stay (the game has no touch controls).
- [ ] **Packaged build**: a packaged Development build switches the hints the same way.

### Editor chores

Changes only the editor can make safely. The steps are in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-saving the template Blueprints and `L_Farm`, moving `BP_FarmTile` and the materials with redirectors fixed, and regenerating the Android File Server token if it was ever used. After each one, run the [smoke test](#3-smoke-test).

### Potato and cabbage, 2026-11-13

Two more crops with their own meshes and materials, and their seeds in the starting bag (hotbar keys 4 and 5): "Add potato and cabbage crops to the starting seeds". The rules (growth, harvest, stamina, the five starting stacks, valid data) are automated tests; these checks are only for how it looks and feels. Start with the console command `NewGame`.

- [ ] **Each stage reads** (why a person: a new look the first time): till a few tiles, plant a potato (4), a cabbage (5) and a turnip (2) and water them. The seedlings are visible from where you stand. Step days with `SkipDay`, watering each time: every new shape is clearly different from the one before, the cabbage's blue-green seedling and early shapes especially. Check dry and watered soil and at night (`SkipDay` to dusk, or wait). No crop, at any stage, shows the grey default material (the tests check the materials, but not how they draw in the game).
- [ ] **The early nights show growth** (why a person: feel, it is what the last play-test failed on): after 2 watered sleeps the cabbage has visibly grown; after 4 watered sleeps the potato and the cabbage have each changed visibly at least 3 times (counting each new shape). A tile left dry doesn't change.
- [ ] **Ripe is unmistakable** (why a person: a new look the first time): each ripe crop reads as ripe at a glance, and the ripe potato's tubers read as potatoes, not pebbles.
- [x] **Five-slot hint row** (why a person: HUD as seen): the bottom line names all the hotbar keys ("1 Hoe   2 Turnip Seeds   3 Watering Can   4 Potato Seeds   5 Cabbage Seeds") and stays readable in the window.
- [x] **A full first day** (why a person: feel): till, plant and water all 16 seeds on day 1. It leaves about 20 stamina, which should feel like a full day's work, not a chore and not too easy.

## 3. Smoke test

About five minutes in `L_Farm`, in Play in Editor at High. Run it before pushing and at the end of each milestone, and after any editor chore. Back up `Saved/SaveGames/` first.

1. **Start:** a new game (the console command `NewGame`) starts on day 1 at 06:00. The HUD shows the day, the clock, the item in hand ("Hoe"), the stamina ("Stamina 100 / 100"), the bag ("Bag: Hoe 1   Turnip Seeds 10   Watering Can 1   Potato Seeds 4   Cabbage Seeds 2") and the key hints; the Output Log has no `LogVestige` errors.
2. **Tools:** with each tool on the tile in front of you: the hoe (1) tills grass, the seeds (2) plant in tilled soil and use a seed, the watering can (3) darkens the soil, and E with an empty slot (6) says "Nothing in hand to use.". The item in hand changes with each key. Each action shows a short message, and each one that changes the farm lowers the stamina; facing away from the field says "Stand facing a farm tile." and costs nothing.
3. **Growing:** water a planted tile and sleep in the bed (walk up to it, F), a few times; the console command `SkipDay` skips a day the same way. The crop grows a little each watered night and changes stage every few nights; a dry tile doesn't grow. Face the ripe crop ("F Harvest") and press F; a Turnip stack appears in the bag or grows. Each night in bed refills the stamina.
4. **Day and night:** let the clock run through an evening (it moves about one hour every 40 seconds unless Project Settings > Game > Vestige Clock is set faster). The sun sets with warm light, the night is properly dark with faint moonlight, a message warns at 22:00 and another at midnight, and after midnight the stamina number falls a little every game minute. Don't go to bed: at 06:00 the next morning the day ends where the farmer stands, a message says he stayed up all night and the next day begins, another says he wakes exhausted, and stamina shows 75 of its 100. To get there faster, type `SkipToTime 0550` in the console after midnight (the drain still counts the skipped time).
5. **Saving:** sleep in the bed ("Game saved."), stop the game and play again. The day, the bag, the selected hotbar slot and every tile, crops included, come back.
6. **Highlight:** walk along the field edge and turn. The frame is bright yellow, by day and at night, follows the tile in front of you and disappears off the field.
7. **Performance:** in the console, `stat fps` and `stat unit`. In a busy part of the farm, by day and by night, the frame stays under 16.6 ms (60 fps), and sleeping shows no visible hitch ([Architecture.md, section 10](Architecture.md#10-performance-by-design)). Play in Editor adds its own cost, so check a doubtful number in Standalone Game. For a hitch on the new day, start the game with `-trace=default` and open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`): `UVestigeClockSubsystem::StartNextDay` shows the whole new day, with the tiles, the save and the load inside it.

Put the save folder back afterwards if you want your farm as it was.

## 4. What isn't manual

- **Game rules are tested automatically**, by the `Vestige` automation tests: tile states and growth, tools and seeds, stamina (costs, the zero floor, the slower uses and the refill), the inventory's stacking and saving, the clock, sleeping and passing out, the save rules, content IDs, the include direction between source folders and the rendering settings. A rule is never covered by a manual check alone. If a check here ever finds a rule broken, the fix comes with a test that would have caught it.
- **This document is for what tests can't judge,** and it is kept as short as possible (David, 2026-11-13). Before a check lands here, the test author tries an automated test, then checks it itself by running the game off-screen and reading the log or looking at screenshots. What is left for a person is how the game feels in play, whether it is fun or reads well, input on real hardware, approving a new look the first time, and steps only the editor can do. Each check says why it needs a person, and checks marked "(at the smoke test)" can wait for the milestone's smoke test.
- **When a change needs a manual check**, the commit that makes it adds a line to [Pending checks](#2-pending-checks) under a dated heading, with the commit and what to look for. A system that gains lasting behavior also gets a line in the [smoke test](#3-smoke-test).
