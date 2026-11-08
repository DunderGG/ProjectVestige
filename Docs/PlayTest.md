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

Checks for recent changes that haven't been confirmed in the editor yet. Each names the commit to look at if something is wrong. When a check passes, delete its line (or tell the agent, who deletes it). When one fails, write what you saw under it; it is fixed before new work starts.

### Code clean-up, 2026-10-29

- [x] **Sleep and passing out** (758f81c, 9d77ee1): N starts the next day. Letting the clock reach 02:00 passes the farmer out into the next day.
- [x] **Key hints** (6805d86, 812ea57): the bottom line reads "1 Hoe   2 Seeds   3 Watering Can   4 Hand   |   E Use tool   |   N Sleep   |   F9 New game". The tool names come from the tool data assets.
   - This works, but we need a much better toolbar.
- [x] **Clock** (812ea57): the status line shows the day, a 24-hour clock in 10-minute steps and the tool in hand.
- [x] **Counts** (812ea57): planting lowers the seed count by one, and harvesting a ripe crop raises the harvest count by the crop's yield. Both update at once.
- [x] **Old save** (9cdf0ac, 8fff295): your existing save loads with its day, inventory and planted crops. A save from before 2026-10-29 is copied to `Farm_Version0.sav` first. Each new day saves to whichever of `Farm.sav` and `Farm2.sav` doesn't hold the newest save.
- [x] **Lighting** (42f3a44): at High and at Epic, days look as before and nights are properly dark. Epic may differ slightly, since only Epic uses hardware ray tracing now. **How: while playing, open the console (§, or see [Before you test](#1-before-you-test)), type `scalability 2` for High, look at a day and a night, then `scalability 3` for Epic and look again ([Before you test](#1-before-you-test)).** Let the clock run to get from day to night; sleeping (N) brings back the morning.
- [x] **Content validation** (139e88d): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.

### Tile targeting, 2026-10-30

Replaces the tile highlight check above: the frame now sits on a neighbouring tile chosen by the movement direction (GameDesign.md, section 6). The tap, walking and feel checks moved to [Smooth turning](#smooth-turning-2026-10-30) below; the results that led to it are in commit 8932448.

- [x] **Field edge** (ce1f1dc): standing on an edge tile and turning to face outwards hides the frame; turning back shows it again.
- [x] **Tools** (ce1f1dc): the tools act on the framed tile, never on the one the farmer stands on.
- [ ] **Gamepad** (ce1f1dc), if you have one: a flick of the left stick turns the farmer with barely a step, and holding it walks, the same as the keys.
   - Unable to test Gamepad support now. Save this for later, when we have a gamepad to test with.

### Smooth turning, 2026-10-30

Replaces the tap to turn, walking and feel checks of the tile targeting change, after David's notes that turns snapped round and the farmer stood still before walking. Walking now eases in over 0.25 s from a press, and every turn goes through the movement component at 540 degrees per second (GameDesign.md, section 6). Both are David's tuning from play.

- [x] **Tap** (a3e118b): a quick tap of W, A, S or D turns the farmer smoothly toward that tile, with no pop, and he moves no more than a few centimetres. The frame moves to the neighbouring tile in that direction, and after a tap he is still on his own tile.
- [x] **Walking** (a3e118b): holding a key walks at once, with no pause before he starts, and the frame stays one tile ahead in the direction of travel.
- [x] **No double turn** (a3e118b): turn the camera to an angle to the field (about 30 degrees) and tap or walk. The farmer turns once, without snapping one way and sliding back.
- [x] **Settling** (a3e118b): after walking at an angle and letting go, he finishes turning to face the framed tile.
- [x] **Tuning** (a3e118b): if a tap moves too far or the walk feels slow to start, change Move Ramp Seconds on `BP_Farmer` in `Content/Farm/Blueprints` (Details panel, Farming > Input; lower walks sooner, higher keeps taps smaller). If turns feel slow or too snappy, change Rotation Rate Z on its Character Movement component (Details panel, select CharacterMovement, Character Movement (Rotation Settings)). Note the values that feel right.
   - David tuned them in play on 2026-10-30 (300 degrees per second and 0.5 s felt heavy; 540 and 0.25 feel good), and they are now the C++ defaults. Still open for more tuning.

### Message boxes, 2026-10-30

Replaces the earlier messages check (c87081e), after David's note that messages stacked in one box read like a log.

- [x] **Separate boxes** (826f80b): every message shows in a small box of its own, stacked above the key hints with a gap between them. Sleeping shows "You slept. Day N begins." and "Game saved." as two boxes.
- [x] **Own timers** (826f80b): each box fades in, stays about three seconds and fades out on its own. Use a tool, wait a second and use it again: the first box fades a second before the second.
- [x] **At most three** (826f80b): with three boxes showing, a fourth message pushes out the oldest. Tilling and planting quickly is the easiest way to get there.

### Five messages and the message log, 2026-10-30

Raises the cap from the message boxes checks above.

- [ ] **At most five** (77a2218): with five boxes showing, a sixth message pushes out the oldest. Since M4 (2026-11-08) tilling and planting show no message, so press E with the hoe on tilled soil, then on grass with the watering can, and so on (each press shows its reason). The boxes stay clear of the key hints and the status line.
- [ ] **Settings** (77a2218): Edit > Project Settings > Game > Vestige HUD shows Max Messages (5) and Message Seconds (3). Changing them takes effect the next time you press Play, and is saved to `Config/DefaultGame.ini`; tell the agent the values you settle on.
- [ ] **Log** (77a2218): every message also appears in the Output Log (Window > Output Log, filter on `LogVestigeFeedback`) as `[L_Farm] UVestigeFeedbackSubsystem::Notify: ...`, so you can read messages that have already faded. (Updated 2026-11-02 for the logging categories; the line read `LogVestige: Feedback: ...` before. Since M4, 2026-11-08, the line is `UVestigeFeedbackSubsystem::Report:` with the tag, checked under "Feedback reports" below.)

### Unreal Engine 5.8.4 build, 2026-10-30

The project now builds only with 5.8. The rest of the upgrade (assets, rendering, documents) is in [Roadmap.md, section 4.1, Engine](Roadmap.md#41-foundations).

- [x] **Editor start** (674adff): double-clicking `Vestige.uproject` opens the 5.8 editor straight away, with no "Target Upgrade Required" or "missing modules" prompt, and the Output Log at start has no `LogVestige` errors.
- [x] **Walking** (de2e2c2): a tap of W, A, S or D still turns the farmer with barely a step, and holding a key walks as before.
- [x] **Visual Studio plugin** (ae539f7): with the Development Editor configuration, open `Source/Vestige/Player/VestigePlayerCharacter.h` and, if needed, Project > Rescan UE Blueprints. After a minute, grey hints such as "1 derived Blueprint" appear above `UCLASS`, `UPROPERTY` and `UFUNCTION` lines, and `Vestige.uproject` still has `"Optional": true` on the plugin.
   - Works once Tools > Options > Text Editor > CodeLens > Show Unreal Engine Blueprints References (C++) is ticked; it was off. Visual Studio's own asset reader logs "Unexpected EOF" for some assets during the rescan, which doesn't affect the hints.

### Item data assets, 2026-10-30

Items exist as data but nothing in play uses them yet, so the game itself should behave exactly as before.

- [x] **Validation** (46bf244): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.
   - Checked in 5.8 on 2026-10-31 by the agent with the `DataValidation` commandlet: all 23 assets under `Content/Farm` are valid, and the editor logs have no duplicate content ID error.
- [ ] **Item assets** (46bf244): `Content/Vestige/Items` has `DA_Item_Turnip` and `DA_Item_TurnipSeeds`. Opening the seeds shows one **Farm Seed Fragment** under Fragments, with Crop Id `FarmCrop:Base.Turnip`; its Crop Id picker lists only crops. Pressing + on Fragments offers the fragment classes to pick from.
- [ ] **Turnip crop** (46bf244): `DA_Crop_Turnip` shows Harvest Item Id `VestigeItem:Base.Turnip` and Yield under Farming > Harvest, and no Seed Price or Sell Price.
- [ ] **No change in play** (46bf244): planting, growing and harvesting a turnip work as in the [smoke test](#3-smoke-test).

### Source folders, 2026-10-30

- [x] **Solution** (adef4a7): after regenerating the project files, the solution shows `Core`, `Content`, `Time`, `Farming`, `Save`, `Player`, `UI`, `Template` and `Tests` under `Source/Vestige`.
- [x] **Editor start** (adef4a7): the editor opens `L_Farm` with no errors about missing classes, and the farmer Blueprint (`Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`), `BP_FarmTile`, the crops and the tools open normally.

### Tile highlight material, 2026-10-31

After the upgrade to 5.8 the frame drew dark, because its engine material wasn't flagged for instanced meshes and 5.8 draws the default grey material instead. It now uses `Content/Farm/Materials/M_TileHighlight`, made by script, so the editor hasn't shown it yet.

- [x] **Bright frame** (a397e1c): in Play in Editor and in Standalone Game, the frame around the targeted tile is bright yellow by day and still clearly visible at night, as before the upgrade.
- [x] **No warning** (a397e1c): the Output Log (Window > Output Log) has no "missing usage flag" warning after playing.
- [x] **Asset** (a397e1c): `M_TileHighlight` opens in the material editor with Blend Mode Opaque, Shading Model Unlit and Usage > Used with Instanced Static Meshes ticked, and the editor doesn't ask to save it.

### Inventory, 2026-10-31

Seeds and harvests are now items in an inventory on the player state. `BP_PlayerState` was made, and `BP_ThirdPersonGameMode` and `BP_Farmer` were changed, by a headless editor script, so the editor hasn't shown them yet. Back up `Saved/SaveGames/` first: your save is upgraded to version 2 on the first new day.

- [x] **Old save** (a3ace26): pressing Play loads your farm, and the HUD's inventory line reads "Bag: Turnip Seeds N   Turnip M" with the seed and harvest counts you had. Sleep once: the game saves, and stopping and playing again brings the same bag back.
- [x] **New game** (a3ace26): F9 starts with "Bag: Turnip Seeds 10".
- [x] **Planting and harvesting** (a3ace26): planting lowers the seeds by one; harvesting a ripe turnip adds a Turnip stack (or raises it). With no seeds left, the seeds tool says "Out of seeds." and the Turnip Seeds entry disappears from the line.
- [x] **Assets** (a3ace26): `Content/Farm/Blueprints/BP_PlayerState` opens; its **Inventory** component shows Slot Count 24 and Starting Items with `FarmItem:Base.TurnipSeeds` × 10. `BP_ThirdPersonGameMode` shows Player State Class `BP_PlayerState`. `BP_Farmer`'s **Farming** component shows Seed Item Id `FarmItem:Base.TurnipSeeds`, and its picker lists only items. None of them asks to be saved when you close it.
- [x] **Solution** (a3ace26): after regenerating the project files, the solution shows the new `Inventory` folder under `Source/Vestige`.

### Vestige rename, 2026-11-01

The general C++ types lost their `Farm` prefix (`AFarmHUD` is now `AVestigeHUD`, `AFarmerCharacter` is `AVestigePlayerCharacter`, the full list is in the Architecture decision log). The level, `BP_Farmer` and the tool and item assets still name the old classes until they are saved, and `[CoreRedirects]` in `Config/DefaultEngine.ini` map them. A headless check loaded them all without warnings, and the tests load a save that names the old classes, but the editor hasn't shown them yet. Back up `Saved/SaveGames/` first.

- [x] **Solution** (74bff97): regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files). The solution lists the renamed files (`Player/VestigePlayerCharacter.h`, `UI/VestigeHUD.h`...) and no `FarmHUD` or `FarmDayCycle` files.
- [x] **Old save** (74bff97): pressing Play loads your farm as you left it: the day, the clock, the bag and the tiles. Sleep once; stopping and playing again brings the same farm back.
- [x] **Assets** (74bff97): `BP_PlayerCharacter` opens and compiles without errors, and its Class Settings show Parent Class `Vestige Player Character`; its components include **Rest** (`Vestige Rest Component`) and **Character Movement** (`Vestige Character Movement Component`). In `L_Farm`, the day cycle actor shows class `VestigeDayCycle` with its settings as before. `DA_Tool_Hoe` and `DA_Item_TurnipSeeds` open with their values. The Output Log has no "Failed to load" or "can't find class" lines after opening them and playing. If the editor asks to save any of them, saving is fine: they then store the new names.
- [x] **Settings** (74bff97): Edit > Project Settings > Game lists **Vestige HUD** (no more Farm HUD) with the same values.
- [x] **Asset types** (1849c74): the tool, item and crop assets, `BP_PlayerCharacter` and `BP_PlayerState` were re-saved by script with the new types. `BP_PlayerCharacter`'s **Farming** component shows Toolbar Ids `VestigeTool:Base.Hoe` to `VestigeTool:Base.Hand` and Seed Item Id `VestigeItem:Base.TurnipSeeds`; `BP_PlayerState`'s **Inventory** shows Starting Items `VestigeItem:Base.TurnipSeeds` × 10; `DA_Crop_Turnip` shows Harvest Item Id `VestigeItem:Base.Turnip`. Each picker still lists only its own kind of asset, and none of them asks to be saved when you close it.
- [x] **Season tags** (1849c74): `DA_Crop_Turnip`'s Seasons picker offers `Vestige.Season.Spring` to `Vestige.Season.Winter`.
- [x] **Content folder** (35975de): our assets were moved by script from `Content/Farm` to `Content/Vestige`, with the same subfolders; `L_Farm` and `BP_ThirdPersonGameMode` were re-saved to point at them. The Content Browser shows `Content/Vestige` and no `Farm` folder (if an empty one shows, right-click `Content` > Fix Up Redirectors in Folder, then delete it). The grid in `L_Farm` still spawns `BP_FarmTile` tiles with the soil meshes, and the targeted tile's frame is bright yellow. Right-click `Content/Vestige` > Validate Assets reports no errors.
- [x] **Player Blueprint** (c101b8f): `BP_Farmer` is now `Content/Vestige/Blueprints/BP_PlayerCharacter` (renamed by script, no redirector left), with Parent Class `Vestige Player Character`. `BP_ThirdPersonGameMode` shows Default Pawn Class `BP_PlayerCharacter`, and pressing Play spawns the player as before.
- [x] The [smoke test](#3-smoke-test).

### Stamina, 2026-11-01

The player now has stamina, through the Gameplay Ability System (Systems.md, section 9). Using a tool is an ability: a use that changes the farm costs the tool's Stamina Cost (hoe and watering can 2, seeds and hand 1, out of 100), and every use takes the tool's Use Seconds (0.4 s) before the next one can start, twice as long at zero stamina. Sleeping refills stamina, and the save stores it. `DA_AbilitySet_Player` was made and `BP_PlayerState` and the four tool assets changed by a headless editor script, so the editor hasn't shown them yet. Back up `Saved/SaveGames/` first: your save is upgraded to version 3 on the first new day.

- [x] **Editor start** (1bfe8f1): the editor opens without asking to enable a plugin or rebuild, and the Output Log at start has no errors from `LogAbilitySystem` or `LogVestige`. Edit > Plugins shows **Gameplay Abilities** enabled.
- [x] **HUD** (1bfe8f1): under the status line, the HUD reads "Stamina 100 / 100". Tilling a tile drops it to 98 at once.
- [x] **No cost for nothing** (1bfe8f1): watering bare grass or using the hand on an unripe crop shows its message and leaves stamina where it was.
- [x] **Use time** (1bfe8f1): E uses the tool once per press (holding it does nothing more). Pressing it quickly uses the tool about twice a second, no faster: presses that come too soon are ignored. Walking isn't slowed while a use is under way. Say if 0.4 s feels sluggish or too quick: it is Use Seconds on each tool asset in `Content/Vestige/Tools` (Details panel, Farming).
   - Corrected 2026-11-01: this check first said holding E repeats the use. It doesn't: E is bound to the press only, and the agent wrote the check wrong.
- [x] **Out of stamina** (1bfe8f1): keep tilling and watering until the HUD reads "Stamina 0 / 100". The message "You're out of stamina. Everything takes longer until you sleep." appears once, the tools still work, each use now takes about twice as long, and stamina stays at 0.
- [x] **Sleep** (1bfe8f1): sleeping (N) brings the HUD back to "Stamina 100 / 100", and so does passing out at 02:00.
- [x] **Saving** (1bfe8f1): spend some stamina, then stop and play again: stamina is full, because the game saves at the start of each day, after sleeping. Your old save loads with full stamina.
- [x] **Assets** (1bfe8f1): `Content/Vestige/Abilities/DA_AbilitySet_Player` opens with Attribute Sets `VestigeStaminaSet` and Abilities `VestigeGameplayAbility_UseTool`. `BP_PlayerState` shows Ability Sets with `DA_AbilitySet_Player`, and its components include **AbilitySystem**. `DA_Tool_Hoe` shows Stamina Cost 2 and Use Seconds 0.4. None of them asks to be saved when you close it.
- [x] **Tuning** (1bfe8f1): over a few in-game days, say whether 100 stamina and these costs feel right: running out should happen on a busy day, not every day. Tell the agent the numbers you settle on.
- [x] **Solution** (1bfe8f1): after regenerating the project files, the solution shows the new `AbilitySystem` folder under `Source/Vestige`.

### Blueprint extension points, 2026-11-01

Blueprints, and later mods, can now hear of a new day, a harvest and a tool use ([Architecture.md, section 12](Architecture.md#extension-points-today)). The tests check that the events fire; these checks are for how they look in the editor. Don't save the Blueprints you try them in: close them with Don't Save.

- [x] **Events in the Details panel** (7e84ca8): open `BP_PlayerCharacter` (`Content/Vestige/Blueprints`), select **FarmingComponent** in the Components panel, and scroll the Details panel to Events: **On Crop Harvested** and **On Tool Used** are listed with a + button. Pressing + on On Tool Used adds an event node with **Tool** and **Tile** pins.
- [x] **Content ID** (7e84ca8): drag from the Tool pin and search "Get Content Id"; the node is there. Wire it into a Print String from the On Tool Used node, compile and play: tilling grass prints `Base.Hoe`, watering bare grass prints nothing. Harvesting a ripe crop with the hand also fires **On Crop Harvested**.
- [x] **New day** (7e84ca8): in the same Event Graph, right-click and search "Get Day Cycle", drag from its return value and search "Assign On New Day" (or "Bind Event to On New Day"). Wire it from Event BeginPlay to a Print String of the Day, compile and play: sleeping (N) prints 2. Close the Blueprint with Don't Save afterwards.

### Interaction and the bed, 2026-11-01

The player now sleeps in a bed, by walking up to it and pressing **F** ([Systems.md, section 7](Systems.md#7-interaction)). N stays as a test key that skips to the next day anywhere. The tests check the rules (focus, the first option, the bed sleeping the player, the ability); these checks are for how it looks and feels. The commit is the one titled "Add the interaction interface and a bed to sleep in".

- [x] **The bed**: a placeholder bed (a flat grey box, 2 m long) lies lengthwise along the near edge of the field, to the left of where the farmer starts, outside the tiles. The farmer can't walk through it.
- [x] **Prompt**: walking up to the bed shows a box reading "F Sleep" below the middle of the screen. It appears when the farmer is about a step from the bed, not from across the field, and disappears again when he walks away. Say if the reach feels too short or too long: it is Interaction Radius on the farmer's **Interaction** component (100 cm), and the bed's Interaction Volume box.
- [x] **Sleeping**: pressing F by the bed starts the next day at 06:00 with "You slept. Day 2 begins.", full stamina and "Game saved.", the same as N did. F away from the bed does nothing.
- [x] **Key hints**: the bottom line reads "... | E Use tool | F Interact | N Skip day (test) | F9 New game".
- [x] **Tool by the bed**: standing on the edge tile next to the bed, facing into the field, the prompt shows and E still uses the tool on the framed tile, not the bed. (Moved 2026-11-01: the bed first stood too far from the field to test this.)
- [x] **Assets**: `DA_AbilitySet_Player` lists `VestigeGameplayAbility_Interact` under Abilities, and `Content/Vestige/Blueprints/BP_Bed` opens with a cube **BedMesh** and an **InteractionVolume** box. Project Settings > Engine > Collision lists an Object Channel **Interactable** (default response Ignore) and a preset **VestigeInteractable**. None of them asks to be saved when you close it.
- [x] **Solution**: after regenerating the project files, the solution shows the new `Interaction` folder under `Source/Vestige`.

### Logging, 2026-11-02

Our log lines are now split into one category per system, and each line starts with its sender and function ([Systems.md, section 21](Systems.md#21-logging)). The tests check the line format and that no code logs around the macro; these checks are for the Output Log itself. The commit is the one titled "Split the log into a category per system with a sender macro".

- [x] **Colours**: with Category Colorization Mode set as in [Before you test](#1-before-you-test), `LogFarm`, `LogVestigeSave`, `LogVestigeFeedback` and the others each show in their own colour. Say which mode you settle on.
   - Chose the mode "Colorize whole line"
- [x] **Lines**: playing a day and sleeping shows `LogVestigeFeedback` lines such as `[L_Farm] UVestigeFeedbackSubsystem::Notify: Game saved.`, and a load at Play shows `LogVestigeSave: [VestigeSaveSubsystem_0] UVestigeSaveSubsystem::LoadFarm: Loading day ...`. No line shows `{` braces or a field name instead of its value.
- [x] **Filters**: after a Play, the Output Log's Filters > Categories menu lists `LogVestigeSave` and `LogVestigeFeedback`, and ticking only `LogVestigeSave` hides the feedback lines. `LogFarm` and the other categories appear in the menu only once they have written a line, which they don't in a level that is set up right. (Corrected 2026-11-02: this check first said to tick `LogFarm`.)
- [x] **Console**: `log list` in the console lists our categories, and `log LogVestigeSave Warning` hides the save's `Loading day` line on the next Play (`log reset` brings it back).
- [x] **Solution**: after regenerating the project files, each layer folder shows its new log header (`Farming/FarmLog.h`, `Save/VestigeSaveLog.h` and so on) and `Core/VestigeLog.h`.

### Out of stamina on the HUD, 2026-11-02

The stamina line turns red while the player is out of stamina. The HUD follows the set's out-of-stamina tag, so it changes colour exactly when the "You're out of stamina" message appears. The commit is the one titled "Show the stamina line in red while out of stamina".

- [x] **Red at zero**: keep tilling and watering until the HUD reads "Stamina 0 / 100". The line turns red at the same moment as the out-of-stamina message, and stays red while you keep working.
- [x] **White again**: sleeping (F at the bed, or N) brings the line back to white with "Stamina 100 / 100".
- [x] **Colour**: say if the red is hard to read on the dark backdrop, or too loud. It is `VestigeHudStyle::Warning` in `UI/VestigeHudWidget.cpp`.

### Late nights, 2026-11-02

The late-night rules from [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons): no pass-out at 02:00, a stamina drain from midnight, and exhaustion after going to bed at 00:00 or later. The tests cover the rules; these checks are for how they feel and read. The commit is the one titled "Add the late-night rules: drain, exhaustion and warnings". Back up `Saved/SaveGames/` first: your save is upgraded to version 4 on the first new day. To get to the night quickly, lower Real Seconds Per Game Hour on the `VestigeDayCycle` in `L_Farm` for the session (Details panel, Farming > Time), and put it back afterwards.

- [x] **Warnings**: at 22:00 a message says it's getting late, and at 00:00 one says it's past midnight and stamina drains until you sleep.
- [x] **Drain**: after midnight the stamina number counts down by itself, about 10 per game hour, even standing still. It stops at 0, the line turns red, and the out-of-stamina message appears once.
- [x] **Late bed**: sleeping (F at the bed, or N) after midnight shows "You slept. Day N begins.", then "You went to bed late and wake up exhausted.", then "Game saved.". The HUD reads "Stamina 75 / 100   Exhausted", and the drain has stopped.
- [x] **Early bed**: the next night, sleeping before midnight shows "A good night's sleep. You're no longer exhausted." and the HUD reads "Stamina 100 / 100" without "Exhausted".
- [x] **All night**: staying up until 06:00 starts the next day where the farmer stands, with "You stayed up all night. Day N begins." and the exhausted message. Nothing jumps: the sun rises as the new day starts.
- [x] **Saving**: go to bed late, then stop and play again: the HUD still says "Exhausted", with 75 stamina.
- [x] **Feel**: say whether 10 per game hour pushes you toward bed without feeling harsh, and whether 75% is enough of a cost. Both are settings on `BP_PlayerCharacter`'s **Rest** component (Details panel, Farming > Rest), as are the warning and late-night hours.

### Holding E, 2026-11-02

Holding E keeps using the tool: the next use starts when the last one has finished or when the framed tile changes, and only if it would change that tile. The tests cover the rule; these checks are for the feel. The commit is the one titled "Repeat the tool use while E is held".

- [x] **A row**: with the hoe, hold E and walk along a row of grass. Every tile you pass is tilled, one "Tilled the soil." message each, and walking doesn't need to stop.
- [x] **Quiet skips**: hold E with the hoe while standing still facing a tile you just tilled. Nothing more happens and no message repeats. Walk on with E still held, and the next grass tile is tilled.
- [x] **The press says why**: press E once facing tilled soil with the hoe. "Only grass can be tilled." shows once, not again while you hold E.
- [x] **Release**: let go of E halfway along a row. The use under way finishes, and no tile after it changes.
- [x] **Watering and planting**: holding E with the watering can waters a row of tilled soil, and with the seeds plants a row until the seeds run out, then stops quietly.
- [x] **Speed**: say whether walking with E held outruns the 0.4 s use (tiles left behind unworked). If so, the use time or the walk speed needs tuning.

### Slower walk while holding E, 2026-11-03

After David's play-test: the farmer walks at half speed while E is held, so a row is worked evenly. The tests check the speed; these checks are for the feel. The commit is the one titled "Walk at half speed while E is held".

- [x] **Slower**: holding E while walking along a row moves at about half speed, and every tile of the row is worked. Letting go brings back the normal walk at once.
- [x] **No E, no change**: walking without E, and jumping or falling with E held, feel as before.
- [x] **Feel**: say if half is too slow or not slow enough. It is Tool Use Speed Scale on `BP_PlayerCharacter`'s **CharacterMovement** component (Details panel, Farming > Movement).

### No tools in the air, 2026-11-03

After David found that jumping got round the slower walk: tools don't work while the farmer is in the air, a held E carries on when he lands, and a jump with E held is slowed like the walk. The tests cover the rules. The commit is the one titled "Keep tools on the ground".

- [x] **Jumping**: hold E with the hoe and jump along a row of grass. No tile changes while he is in the air, and the tile he faces when he lands is tilled.
- [x] **No speed-up**: with E held, jumping along the row is no faster than walking it.
- [x] **Press in the air**: press E once during a jump without holding it. Nothing happens, and no message shows.

### Tile highlight material from the Blueprint, 2026-11-03

The frame's material, `M_TileHighlight`, is now set on `BP_PlayerCharacter`'s highlight component (by script) instead of in C++. The test checks the Blueprint has it; these checks are for how it looks. The commit is the one titled "Set the tile highlight material on the player Blueprint".

- [x] **Asset**: open `Content/Vestige/Blueprints/BP_PlayerCharacter`, select its **TargetHighlight** component: Highlight Material is `M_TileHighlight` (Details panel, Farming > Visuals). The Blueprint doesn't ask to be saved when you close it.
- [x] **Frame**: in Play in Editor the thin yellow frame around the target tile (the tile itself doesn't change color) is a flat, bright yellow, in daylight, in shadow and at night alike: its material is unlit, so light and shadow don't darken it, and the Output Log has no "No Highlight Material" warning and no "missing usage flag".

### Details categories, 2026-11-03

The general types' settings moved out of the `Farming` categories into ones named for their topic; the farm types (tiles, grid, crops, the farming component and the highlight) keep `Farming`. Only display names changed, so nothing needs saving. The commit is the one titled "Name the general types' Details categories by topic".

- [x] **Sky lighting**: the `VestigeSkyLighting` (until 2026-11-05 `VestigeDayCycle`) in `L_Farm` shows its settings under **Lighting** only (Night Post Process too). It had a **Time** category for the sunrise and sunset hours until they moved to the clock settings on 2026-11-05.
- [x] **Player Blueprint**: in `BP_PlayerCharacter`, Move Ramp Seconds is under **Input** with the template's input actions, the **Rest** component's hours and drain under **Rest**, and the **CharacterMovement** component's Tool Use Speed Scale under **Movement**.
- [x] **Assets**: a tool data asset shows **Content**, **Tool** and **Visuals**; an item shows **Content**, **Item** and **Economy**; a crop shows **Content** and **Farming** with Growth, Harvest and Visuals.
- [x] **Blueprint menus**: in any Blueprint's graph, right-click and search: Get Vestige Clock is there (it replaced Get Day Cycle on 2026-11-04), Get Content Id under **Content**. In Add Component, the Vestige Rest component is listed under **Rest**.

### Test keys as console commands, 2026-11-04

F9 (new game) and N (skip day) are no longer keys: they are the console commands `NewGame` and `SkipDay`, which only development builds have. The tests check the commands exist on the game's player controller and that `SkipDay` starts the next day; these checks are for the console and the keys. The commit is the one titled "Move the test keys F9 and N into a cheat manager".

- [x] **Key hints**: the bottom line ends "... | E Use tool | F Interact", with no N or F9. Pressing N or F9 in the game does nothing (F9 may still take an editor screenshot).
- [x] **SkipDay**: open the console ([Before you test](#1-before-you-test)), type `SkipDay` and press Enter. The next day starts as it does in the bed: "You slept. Day 2 begins." and "Game saved." The console's suggestions should list `SkipDay` and `NewGame` as you type them.
- [x] **NewGame**: back up `Saved/SaveGames/` first. Type `NewGame`: the map reopens on day 1 at 06:00 with the starting bag.

### Save versions restart, 2026-11-04

The save upgrades are gone and the save version starts again at 1, so your current farm (a version 4 save) can't be loaded any more. The tests cover what loading decides; these checks are for the real files. The commit is the one titled "Delete the old save versions and their upgrades".

- [x] **Old save refused**: before deleting anything, press Play. The game says "The save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game." and the Output Log has a `LogVestigeSave` error naming slot `Farm` and version 4. Sleeping doesn't save.
- [x] **Delete the development saves**: run the console command `NewGame` (it deletes `Farm.sav` and `Farm2.sav`), then stop the game and delete the old upgrade backups `Farm_Version0.sav`, `Farm_Version2.sav` and `Farm_Version3.sav` from `Saved/SaveGames/` by hand. Nothing makes backups any more.
- [x] **New save round trip**: play from day 1, plant and water a tile, sleep ("Game saved."), stop and play again: "Welcome back. Day 2." with the bag, the tool in hand, the stamina and the tile as you left them. `Saved/SaveGames/` holds only `Farm.sav` (and `Farm2.sav` after a second night).

### Benchmark field, 2026-11-04

The console command `BenchmarkField` makes the farm a field of ripe crops for the performance baseline. The tests cover that every tile gets a ripe crop and that an unknown crop changes nothing; these checks are for how it looks and plays. The commit is the one titled "Measure the first performance baseline".

- [x] **Field**: back up `Saved/SaveGames/` first. Press Play, open the console and type `BenchmarkField 20 Base.Turnip`. The 8 by 8 field becomes a 20 by 20 field of ripe turnips with the same corner, and the frame around the targeted tile follows you across it.
- [x] **Harvest**: the hand tool harvests the ripe turnips, and the bag fills as usual.
- [x] **Back to normal**: type `NewGame`. The map reopens on day 1 with the normal 8 by 8 grass field.

### Sun in steps, 2026-11-04

The sun (and the moon at night) now turns in quarter-degree steps instead of every frame, so the shadows it casts stay cached between steps. The tests cover the steps and that a new day moves the sun at once; these checks are for how it looks. The commit is the one titled "Turn the sun in small steps".

`L_Farm`'s day cycle runs a game hour in about 1.5 seconds, which turns the sun every few frames anyway. To see the steps at the normal pace, select the `VestigeDayCycle` in `L_Farm` and set **Real Seconds Per Game Hour** (Details panel, **Time**) to 40 for these checks, then set it back without saving the level, or tell me if 40 should stay.

- [x] **Midday shadows**: press Play and watch the farmer's and the crops' shadows for a game hour around midday. They creep along as before, with no visible jumps or flicker.
- [x] **Sunrise, sunset and moonlight** (2026-11-05: David saw small jumps at 0.25 and 0.1 degrees with the clock at 40 seconds per game hour; 0.05 looks smooth and is now the default): watch the long shadows just after 06:00, before 20:00, and the moon's shadows after 20:00. They move in small hops about every half second. Say whether the hops are noticeable; if they are, lower **Sun Step Degrees** on the `VestigeDayCycle` (Details panel, **Lighting**) and tell me which value looks smooth.
- [x] **Smooth fades**: the warm sunrise colour, the evening dimming and the darkening into night still fade without steps.
- [x] **New day**: sleep in the bed. The morning starts with the sun at its sunrise position at once, not turning there over a few seconds.

### No hardware ray tracing, 2026-11-04

Hardware ray tracing is off for the whole project, so Epic now lights the farm with software Lumen like High does. The test `Vestige.Rendering.HardwareRayTracingIsOff` covers the setting; these checks are for how it looks. The commit is the one titled "Turn off hardware ray tracing".

- [x] **Editor start**: the first time you open the editor after this change it recompiles shaders, which can take a while (the count is at the bottom right). After that, the Output Log has no warnings about ray tracing.
- [x] **High**: press Play, open the console and type `scalability 2`. A day and a night look as before, and the nights are still properly dark.
- [x] **Epic**: type `scalability 3` and look at a day and a night again. Epic may look a little different from before, mostly in shadowed corners and reflections, since it no longer uses hardware ray tracing. Say if anything looks worse than High or clearly worse than it used to.

### Cheaper clouds below Epic, 2026-11-04

The volumetric clouds take fewer samples at High, Medium and Low, set in `Config/DefaultScalability.ini`. The tests cover the settings for each level; these checks are for how the clouds look while they move, which screenshots can't show. The commit is the one titled "Make the volumetric clouds cheaper below Epic".

- [x] **High**: press Play, open the console and type `scalability 2`. Watch the clouds through a morning and an evening, turning the camera now and then. They look as before, perhaps a little grainier at the edges, with no flicker, crawling noise or smearing when the camera turns. Say if it bothers you; High can take more samples.
- [x] **Medium and Low**: type `scalability 1`, then `scalability 0`, and look again. The edges are grainier; the clouds shouldn't flicker or break up.
- [x] **Back to Epic**: after Low, type `scalability 3`. The clouds are smooth again, as before this change.

### Clock and fields in the game instance, 2026-11-04

The clock and the farm moved out of the level into the game instance; the grid, its tiles and the day cycle's lighting now only show them. The rules are covered by tests; these checks are for the editor and play. The commit is the one titled "Move the clock and the farm data into the game instance".

- [x] **Leftover nodes** (done by David, 2026-11-05): open `BP_PlayerCharacter`. It shows compile errors from the Print String nodes left from the extension point checks (Get Day Cycle, Bind Event to On New Day, Print String in the Event Graph). Delete those nodes, compile and save. This also finishes the housekeeping item in [Roadmap.md, section 3](Roadmap.md#3-housekeeping). Until then the test `Vestige.Farming.AFullBagLeavesTheCropInTheGround`, the first to load the Blueprint, fails on the compile errors.
- [x] **Old save refused**: press Play. The HUD says the save couldn't be read and saving is off (the save version is now 2). Type `NewGame` in the console; the map reopens on day 1.
- [x] **Clock settings**: Project Settings > Game > **Vestige Clock** shows Real Seconds Per Game Hour (40), Day Start Hour (6) and Day End Hour (30). The sky lighting actor in `L_Farm` (`VestigeDayCycle` until 2026-11-05) shows only **Lighting**; since 2026-11-05 the sunrise and sunset hours are in the clock settings (see Sky lighting, 2026-11-05). The clock now runs at 40 seconds per game hour; `slomo 20` in the console speeds it up for testing. Tell me if L_Farm's old fast clock should come back as the project setting.
- [x] **Grid**: the `FarmGrid` in `L_Farm` shows **Field Id** `Base.Farm` under **Farming**.
- [x] **A day of farming**: till, plant and water a few tiles, sleep, and check that the watered crops grew and the soil dried, as before. Stop, press Play again: the field, the day and the time come back from the save.
- [x] **Pause**: with the clock running, open the pause or console with the game paused (`pause` in the console). The clock on the HUD stops, and runs again after `pause` once more.
- [x] **New game**: after a few days, `NewGame` reopens the map on day 1 at 06:00 with grass everywhere.
- [x] **Benchmark field**: `BenchmarkField 20 Base.Turnip` still makes a 20 by 20 field of ripe turnips.

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

### Jump to a time, 2026-11-06

The console command `SkipToTime` runs the clock forward to a time today, for checks at a time of day. The tests cover reading the time, after midnight, refusing earlier or unreadable times, and that the hours passed are broadcast; these checks are for the console. The commit is the one titled "Add a console command to jump to a time of day".

- [x] **Evening**: press Play, open the console and type `SkipToTime 1930`. The HUD clock shows 19:30 and the light turns warm and dims toward 20:00. `SkipToTime 1800` afterwards does nothing and the Output Log says the clock only runs forward.
- [x] **Late night**: type `SkipToTime 2359`. The 22:00 warning shows; a minute later the midnight warning shows and stamina starts to drain. `SkipToTime 559` takes you to just before 06:00, where the day ends as if you had stayed up.

### Save registry, 2026-11-06

Every owner now saves its own part under a key through the save registry, the save is read once per game, and the player's data crosses a map change. The tests cover the registry's rules, an outdated or newer part, unknown parts being kept, a new game, and the player's bag, stamina, exhaustion and tool surviving the map being reopened; these checks are for the real files and a real map change. The commit is the one titled "Save the game through a registry of owners".

- [x] **Old save refused once**: press Play with your current save (version 2). The game says "The save couldn't be read. It is kept as it is, and the game won't save until you start a new game." and the Output Log has a `LogVestigeSave` warning that slot `Farm` holds an older save, version 2. Then type `NewGame` in the console: the map reopens on day 1 with the starting bag.
- [x] **Round trip**: plant and water a tile, choose the watering can, use some stamina, and sleep ("Game saved."). Stop and press Play again: "Welcome back. Day 2." with the bag, the watering can in hand, the stamina and the tile as you left them, and no "Some saved things..." message. The Output Log has a `LogVestigeSave` line "Loaded day 2 from save slot ... 6 parts".
- [x] **Reopening the map keeps the day**: mid-morning, after tilling a few tiles and changing the bag, type `open L_Farm` in the console. The map reopens at the same time of day (it doesn't jump back to the morning's save), the tiles you tilled since the morning are still tilled, and the bag, stamina and tool in hand are as they were. You start at the player start, which is expected.
- [x] **Exhaustion and the late night cross a map change**: go to bed after midnight (`SkipToTime 0030`, then the bed) so the next day starts exhausted, then type `open L_Farm`: the HUD still shows "Exhausted". Then `SkipToTime 0030` and `open L_Farm` again: stamina keeps draining in the reopened map.
- [x] **No reload on a new game**: type `NewGame`, play until the first night and sleep: "Game saved." shows and `Saved/SaveGames/` gets a new `Farm.sav`.

### Game mode and player controller, 2026-11-06

The game mode and the player controller are ours now (`Game/` in the source, `BP_GameMode` and `BP_PlayerController` in `Content/Vestige/Blueprints`), and the template's Blueprints and `Content/ThirdPerson/` are gone. The game mode starts the next day when the clock says the day has ended; the bed only asks the clock to end it. The tests cover the day's end with and without a body, the messages, exhaustion and the save; these checks are for the editor, input and a packaged build. The commit is the one titled "Give the game its own game mode and player controller".

- [x] **Editor start**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files), since `Game/` is new and the template's game mode and controller files are gone. Open the editor: the Output Log has no "Failed to load" or "can't find" line for `BP_ThirdPersonGameMode` or `BP_ThirdPersonPlayerController`. Project Settings > Maps & Modes shows **Default GameMode** `BP_GameMode`, and its **Selected GameMode** shows **Player Controller Class** `BP_PlayerController`, **HUD Class** `VestigeHUD`, **Player State Class** `BP_PlayerState` and **Default Pawn Class** `BP_PlayerCharacter`. `L_Farm`'s World Settings have no **GameMode Override**.
- [x] **Controls**: press Play. Walking, the mouse turning the camera, jumping, the farming keys (1 to 9, E) and F all work as before (the moving and looking keys now come from `BP_PlayerController`'s **Default Mapping Contexts**, `IMC_Default` and `IMC_MouseLook`). The HUD shows as before. Typing `SkipDay` in the console still works.
- [x] **Going to bed**: in the evening, use the bed: "You slept. Day 2 begins." and "Game saved.", stamina full. After midnight (`SkipToTime 0030`, then the bed): "You slept. Day 3 begins.", then "You went to bed late and wake up exhausted.", then "Game saved.", stamina at 75%.
- [x] **Staying up**: `SkipToTime 559` and wait a moment: at 06:00 "You stayed up all night. Day N begins." and the exhausted message, and the clock runs again from 06:00.
- [x] **Rest on the player state** (commit "Move the rest component to the player state"): open `BP_PlayerState` (`Content/Vestige/Blueprints`): its Components panel lists **Rest**, with **Late Warning Hour** 22, **Late Night Hour** 24, **Late Night Drain Per Game Hour** 10 and **Exhausted Refill Fraction** 0.75 under **Rest**. `BP_PlayerCharacter` no longer lists **Rest**, and opening it shows no warning about a missing component. In Play, the 22:00 and midnight warnings, the drain, the bed and the exhaustion work as in the checks above.
- [x] **Packaged build**: package a Development build (as for the benchmark) and start it: it opens `L_Farm` with the HUD, the farmer and working controls, so the packaged game uses `BP_GameMode`.

### Tools as abilities, 2026-11-07

Each tool's behaviour is now its own gameplay ability, named on the tool asset and granted while the tool is in hand; holding E repeats inside that ability ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system)). A headless editor script set the four tool assets and `DA_AbilitySet_Player`, so the editor hasn't shown them yet. Look at the commit "Make each tool's behaviour its own gameplay ability" if something is wrong.

- [x] **Assets**: open `Content/Vestige/Tools/DA_Tool_Hoe`: under **Tool**, **Ability** is `FarmGameplayAbility_Till` (Seeds `FarmGameplayAbility_Plant`, Watering Can `FarmGameplayAbility_Water`, Hand `FarmGameplayAbility_Harvest`), and there is no **Action** any more. `Content/Vestige/Abilities/DA_AbilitySet_Player` lists only `VestigeGameplayAbility_Interact` under Abilities. Right-click `Content/Vestige/Tools` > Validate Assets: no errors.
- [x] **Each tool as before**: the hoe tills, the seeds plant and use a seed, the watering can waters, the hand harvests a ripe crop, with the same messages and the same stamina costs (hoe and watering can 2, seeds and hand 1). A use that changes nothing ("Only grass can be tilled.") costs nothing.
- [x] **Holding E**: hold E with the hoe and walk along a row: each grass tile you reach is tilled, one use every 0.4 s, and nothing is posted for tiles already tilled. Releasing E stops it. Pressing E again quickly, while the last use is finishing, carries on.
- [x] **Jumping**: jump with E held over grass: nothing is tilled in the air, and the tile in front is tilled when you land.
- [x] **Switching tools**: hold E with the hoe on a row, then press 3: the repeat stops, and the watering can starts on the next press of E. Switching right after a use and pressing E at once waits for that use's 0.4 s first.
- [x] **Print String**: the tests' log shows `LogBlueprintUserMessages: [BP_PlayerCharacter_C_0] DA_Tool_Hoe` on every tool use, which looks like a Print String on **On Tool Used** in `BP_PlayerCharacter`. If it was only for checking the event, remove it, since it shows on screen in development builds.

### Stamina effects as GE_ assets, 2026-11-07

The four stamina effects are now gameplay effect Blueprints in `Content/Vestige/Abilities/`, named by a new game data asset, `Content/Vestige/DA_GameData`, that the asset manager loads at startup ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system), effects as `GE_` assets). A temporary headless commandlet made the five assets, so the editor hasn't shown them yet. Look at the commit "Make the stamina effects GE_ assets" if something is wrong.

- [x] **Startup**: the Output Log at editor start has no "The game data ... didn't load" error (search for `LogVestigeContent`).
- [x] **Game data**: open `DA_GameData`: under **Stamina**, the four effects are `GE_StaminaCost`, `GE_StaminaRefill`, `GE_LateNightDrain` and `GE_Exhausted`. Right-click `Content/Vestige` > Validate Assets: no errors.
- [x] **The effects**: open each and look at its Class Defaults. `GE_StaminaCost`: Duration Policy Instant, one modifier on `VestigeStaminaSet.Stamina`, Add, Set By Caller with the tag `Vestige.Data.StaminaCost`. `GE_StaminaRefill`: Instant, Override, `Vestige.Data.StaminaRefill`. `GE_LateNightDrain`: Infinite, Period 1, **Execute Periodic Effect On Application** unticked, Add, `Vestige.Data.StaminaDrain`. `GE_Exhausted`: Infinite, no modifiers, and under Components a **Grant Tags to Target Actor** component that adds `Vestige.State.Exhausted`. Close them without saving unless something is wrong.
- [x] **Stamina as before**: tool uses cost the same (hoe and watering can 2, seeds and hand 1). `SkipToTime 2330`, then let the clock pass midnight: the stamina counts down, about 10 per game hour. Sleep (bed or N) after midnight: you wake at 75 and "Exhausted". Sleep before midnight the next night: full stamina, and the exhaustion goes.
- [x] **Tuning in the editor**: set `GE_LateNightDrain`'s Period to 2, play past midnight: the stamina now drops every 2 seconds, still about 10 per game hour. Set it back to 1 (or don't save).
- [x] **Packaged build**: package a Development build and play past midnight: stamina drains and tools still cost stamina, so the cook included `DA_GameData` and the effects.

### The hotbar, 2026-11-07

The toolbar is now the bag's first nine slots: keys 1 to 9 select a slot, and the item in it is in hand. The hoe and the watering can are items, an empty slot (or a turnip) is the hand, which harvests, and seeds plant from the stack in hand. The aim moved into a targeting component of its own ([Systems.md, section 8](Systems.md#8-inventory-and-items), the hotbar). A temporary headless commandlet made `DA_Item_Hoe` and `DA_Item_WateringCan`, added a tool fragment to `DA_Item_TurnipSeeds` and set `BP_PlayerState`'s starting items and `BP_PlayerCharacter`'s Hand Tool Id, so the editor hasn't shown them yet. An older save has a version 1 bag, which is left out: you start with the new starting items, and the Output Log says "The saved Base.Inventory is version 1, older than this build's 2" once. Look at the commit "Make the toolbar the inventory's hotbar" if something is wrong.

- [x] **Assets**: open `Content/Vestige/Items/DA_Item_Hoe`: Content Id `Base.Hoe`, Display Name "Hoe", Max Stack Size 1, and under Fragments a **Vestige Tool Fragment** with Tool Id `VestigeTool:Base.Hoe` (`DA_Item_WateringCan` the same with `Base.WateringCan`). `DA_Item_TurnipSeeds` has its seed fragment and a tool fragment with `VestigeTool:Base.Seeds`. `BP_PlayerState`'s **Inventory** has the starting items Hoe 1, Turnip Seeds 10, Watering Can 1, and `BP_PlayerCharacter`'s **Farming** has Hand Tool Id `VestigeTool:Base.Hand` and no Toolbar Ids or Seed Item Id. Right-click `Content/Vestige` > Validate Assets: no errors.
- [x] **New game**: `NewGame`. The status line ends in "Hoe", the bag reads "Bag: Hoe 1   Turnip Seeds 10   Watering Can 1", and the key hints read "1 Hoe   2 Turnip Seeds   3 Watering Can   |   E Use tool   |   F Interact" (or with your other action hints).
- [x] **Selecting**: 1, 2 and 3 put the hoe, the seeds and the watering can in hand: the status line and the mesh in the farmer's hand cxhange with each. 4 shows "Empty hand" and no mesh.
- [x] **Each tool as before**: the hoe tills, the seeds plant, the watering can waters, and with 4 (the empty hand) E harvests a ripe crop, with the same messages and stamina costs. The harvested turnips land in slot 4 (the first empty slot), so a "4 Turnip" hint appears and slot 4 still harvests.
- [x] **The last seed**: plant until the seeds run out. After the last one the status line shows "Empty hand", the 2 hint disappears and the farmer's hand empties; the next E with 2 does what the hand does.
- [x] **Holding E with seeds**: with 2 seeds left, hold E with the seeds and walk along two tilled tiles: both are planted, then the repeat stops quietly.
- [x] **Can you tell how to harvest?** Empty slots have no hint, and the 4 Hand hint is gone. Play a little as a new player would: is it clear that an empty slot (or holding a crop) harvests? Write what you think; a hint for the hand can come with the UMG hotbar.
   - No, it wasn't clear (David, 2026-11-07). Harvesting moved to F, with a prompt on the ripe crop: [Harvesting with F](#harvesting-with-f-2026-11-07) below.
- [x] **Saving**: select the watering can (3), sleep in the bed, stop and play again: the watering can is still in hand.
### Harvesting with F, 2026-11-07

Facing a ripe crop now shows "F Harvest", and F harvests it whatever is in hand; E only uses the item in hand, and an empty hand does nothing ("Nothing in hand to use."). What F uses is what the farmer faces: a ripe tile in front of him, or the bed when he looks at it ([Systems.md, section 7](Systems.md#7-interaction), harvesting with F). The farming component's Hand Tool Id is now Harvest Tool Id; a redirect in `DefaultEngine.ini` and a temporary commandlet carried `BP_PlayerCharacter`'s value over. Look at the commit "Harvest ripe crops with F" if something is wrong.

- [x] **Asset**: `BP_PlayerCharacter`'s **Farming** shows Harvest Tool Id `VestigeTool:Base.Hand`, and the Output Log at editor start has no warning about it.
- [x] **The prompt**: grow a crop until it's ripe (water it and `SkipDay`). Facing it, the prompt reads "F Harvest"; facing an unripe or empty tile, there's no prompt. The prompt appears within a moment of turning to the crop and goes as soon as you've harvested.
- [x] **Harvesting**: F harvests the ripe crop with the hoe, the seeds or nothing in hand: the turnips go in the bag, the tile is tilled soil again, and the stamina drops by 1.
- [x] **E with nothing in hand**: select an empty slot and press E: "Nothing in hand to use.", nothing changes, and holding E doesn't slow the walk.
- [x] **Next to the bed**: walk along the field edge past the bed with an unripe crop in front of you and press F: nothing happens (you don't sleep). With a ripe crop in front of you, beside the bed, F harvests. Turn to face the bed: the prompt reads "F Sleep", and F sleeps.
- [x] **Walking**: with a row of ripe crops, walk along it and press F as you pass: each time the crop in front of you is harvested (holding F doesn't repeat yet).

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

### Feedback reports, 2026-11-08

Gameplay no longer writes the player's messages: it reports what happened as a tag, and the HUD shows the text `DA_FeedbackResponses` (`Content/Vestige/UI`) gives it ([Systems.md, section 15](Systems.md#15-visual-effects-and-feedback), feedback reports). The texts are the same as before. The commit is the one titled "Report feedback as tags and let the HUD pick the text".

- [x] **Solution**: regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files): the solution lists `UI/VestigeFeedbackResponses.h` and `.cpp`.
- [x] **Quiet work**: hold E with the hoe across a row of grass, then plant, water and harvest it. No message shows for any use that worked.
- [x] **Reasons**: one press of E shows its reason once for each of these: the hoe on tilled soil ("Only grass can be tilled."), the watering can on grass ("Nothing to water here."), seeds on grass ("Seeds need empty tilled soil."), an empty hotbar slot ("Nothing in hand to use."), and any tool facing away from the field ("Stand facing a farm tile."). F on a ripe crop with a full bag shows "No room in your bag." and the crop stays. Holding E on such a tile shows the reason once, not again and again.
- [x] **Day and stamina**: sleep: "You slept. Day N begins." shows, and "Game saved." no longer does. (Passed; David then gave the row the text "Game Saved!", which shows after it.) Running out of stamina, 22:00, midnight and waking exhausted show their messages as before, and a Play with a save shows "Welcome back. Day N."
- [x] **Log**: the Output Log, filtered on `LogVestigeFeedback`, shows a line per report such as `UVestigeFeedbackSubsystem::Report: Vestige.Feedback.Day.Slept Day=2`, also for the save (`Vestige.Feedback.Save.Saved`), which showed nothing on screen until David gave it a text.
- [x] **Edit without a build**: open `DA_FeedbackResponses` (`Content/Vestige/UI`), give the `Vestige.Feedback.Save.Saved` row the text "Game saved.", save and Play, then sleep: the text shows. Put it back afterwards, or tell the agent to keep it. (Kept as "Game Saved!", 2026-11-08.)
- [x] **Missing row**: remove the `Vestige.Feedback.Farm.CantWater` row, Play and water grass: nothing shows, and the Output Log has a `LogVestigeFeedback` warning naming the tag and `DA_FeedbackResponses`. Close the asset without saving.
- [x] **No grid**: in a level without a `BP_FarmGrid`, Play: no message on screen, and the Output Log warns that the level has no FarmGrid.

### Editor chores

Changes only the editor can make safely. The steps are in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-saving the template Blueprints and `L_Farm`, moving `BP_FarmTile` and the materials with redirectors fixed, and regenerating the Android File Server token if it was ever used. After each one, run the [smoke test](#3-smoke-test).

## 3. Smoke test

About five minutes in `L_Farm`, in Play in Editor at High. Run it before pushing and at the end of each milestone, and after any editor chore. Back up `Saved/SaveGames/` first.

1. **Start:** a new game (the console command `NewGame`) starts on day 1 at 06:00. The HUD shows the day, the clock, the item in hand ("Hoe"), the stamina ("Stamina 100 / 100"), the bag ("Bag: Hoe 1   Turnip Seeds 10   Watering Can 1") and the key hints; the Output Log has no `LogVestige` errors.
2. **Tools:** with each tool on the tile in front of you: the hoe (1) tills grass, the seeds (2) plant in tilled soil and use a seed, the watering can (3) darkens the soil, and E with an empty slot (4) says "Nothing in hand to use.". The item in hand changes with each key. Each action shows a short message, and each one that changes the farm lowers the stamina; facing away from the field says "Stand facing a farm tile." and costs nothing.
3. **Growing:** water a planted tile and sleep in the bed (walk up to it, F), a few times; the console command `SkipDay` skips a day the same way. The crop grows one stage per watered night, and a tile left dry doesn't grow. Face the ripe crop ("F Harvest") and press F; a Turnip stack appears in the bag or grows. Each night in bed refills the stamina.
4. **Day and night:** let the clock run through an evening (it moves about one hour every 40 seconds unless Project Settings > Game > Vestige Clock is set faster). The sun sets with warm light, the night is properly dark with faint moonlight, and at 02:00 the farmer passes out into the next day.
5. **Saving:** sleep in the bed ("Game saved."), stop the game and play again. The day, the bag, the selected hotbar slot and every tile, crops included, come back.
6. **Highlight:** walk along the field edge and turn. The frame is bright yellow, by day and at night, follows the tile in front of you and disappears off the field.
7. **Performance:** in the console, `stat fps` and `stat unit`. In a busy part of the farm, by day and by night, the frame stays under 16.6 ms (60 fps), and sleeping shows no visible hitch ([Architecture.md, section 10](Architecture.md#10-performance-by-design)). Play in Editor adds its own cost, so check a doubtful number in Standalone Game. For a hitch on the new day, start the game with `-trace=default` and open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`): `UVestigeClockSubsystem::StartNextDay` shows the whole new day, with the tiles, the save and the load inside it.

Put the save folder back afterwards if you want your farm as it was.

## 4. What isn't manual

- **Game rules are tested automatically**, by the `Vestige` automation tests: tile states and growth, tools and seeds, stamina (costs, the zero floor, the slower uses and the refill), the inventory's stacking and saving, the clock, sleeping and passing out, the save rules, content IDs, the include direction between source folders and the rendering settings. A rule is never covered by a manual check alone. If a check here ever finds a rule broken, the fix comes with a test that would have caught it.
- **This document is for what tests can't judge:** how the game looks and feels, lighting, the HUD as you see it, input, saves on real files across sessions, performance, and steps that need the editor.
- **When a change needs a manual check**, the commit that makes it adds a line to [Pending checks](#2-pending-checks) under a dated heading, with the commit and what to look for. A system that gains lasting behavior also gets a line in the [smoke test](#3-smoke-test).
