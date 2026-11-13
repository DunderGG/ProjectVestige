# Project Vestige game design

**Current working codename: Project Vestige.** This is an internal project name, not the final public game title.

What the game is, how it plays and why. This document steers development: when a feature idea comes up, check it against the pillars and the scope here first.

- [Architecture.md](Architecture.md) covers how the code is put together. [CodingGuidelines.md](CodingGuidelines.md) covers how the code looks.
- The story, the world's history, the currency and each character's backstory go in the separate lore document, [Lore.md](Spoilers/Lore.md). This document only says where the game needs lore, and marks those places with **Lore hook**.

Keep this document up to date. When a design choice is made or changed, update the relevant section and add a line to the [decision log](#14-decision-log).

Contents

1. [The game in one paragraph](#1-the-game-in-one-paragraph)
2. [Pillars](#2-pillars)
3. [Premise and setting](#3-premise-and-setting)
4. [Game loops](#4-game-loops)
5. [Time, days and seasons](#5-time-days-and-seasons)
6. [Farming](#6-farming)
7. [Survival needs](#7-survival-needs)
8. [Robots and people](#8-robots-and-people)
9. [Progression](#9-progression)
10. [World and exploration](#10-world-and-exploration)
11. [Combat](#11-combat)
12. [Presentation](#12-presentation)
13. [Scope and milestones](#13-scope-and-milestones)
14. [Decision log](#14-decision-log)
15. [Open questions](#15-open-questions)
16. [Mechanics glossary](#16-mechanics-glossary)

---

## 1. The game in one paragraph

A 3D life and farming sim set in a retro-futuristic, post-apocalyptic world. You wake from decades of cryosleep under an overgrown pre-war homestead. Humans have almost vanished. Robots carry on with the jobs people gave them, often for nobody. You bring the farm back to life, from hand tools to automated machines, help rebuild the struggling robot town next door, form friendships and romances with robots and with the rare humans you find, and slowly push out into the wasteland. The tone is mostly serious, with comic relief throughout.

## 2. Pillars

Every feature should serve at least one pillar. Features that serve none are cut or postponed.

1. **Bring the land back.** Farming is the heart of the game. Poisoned soil, scarce water and storms make growth feel earned, and a thriving farm in a dead world is the main reward. Healing the land also clears the sky above it and powers the farm.
2. **Old hands, new machines.** Progress runs from rusty hand tools to pre-war tech to high-tech automation. The game mixes old-fashioned farm life with futuristic technology, and automation is a goal in itself.
3. **A world of robots, and rare people.** Robots fill every role people used to: neighbours, friends, love interests, helpers and enemies. Each one has its own history. Humans are rare, and finding one should feel special.
4. **Serious world, light touch.** The setting is grim and the stakes are real, but the game is warm and funny in its details: robots stuck in pre-war routines, absurd old advertising, dry humour.
5. **Your pace.** No forced sleep, no failing out of the game. Days have limits and late nights have costs, but the player decides how to spend their time. Survival needs can be switched off.

**Design goal: moddable.** Content (crops, items, tools, robots, recipes, tech tree entries) is data found by the game, never a hard-coded list, so a mod can add more of it the same way the base game does. See [Architecture.md section 12](Architecture.md#12-designing-for-mods). Beyond that, a system gets mod-specific extension points only when there is a concrete use for them; we don't build speculative abstractions ahead of the gameplay. Mod loading isn't built yet, so the game doesn't claim mod support until we have decided what mods may change and what compatibility we promise between versions.

**Design target: single-player.** The game is designed and built for one player, as [Architecture.md](Architecture.md#16-decision-log) says. Co-op is wanted some day, but no system needs to be ready for networking now. Keeping ordinary separation of concerns (state owned by one class, no needless global assumptions) is enough. When co-op becomes real work, it gets its own design milestone that decides authority, the shared clock and sleep, farm ownership and saves (see [open questions](#15-open-questions)).

## 3. Premise and setting

### Premise

- The player wakes in a failing cryo pod in a hidden room under an old farmhouse. They don't know exactly how long they've been asleep.
- The farm's **caretaker robot** has kept the place barely alive for decades, waiting for "the family" to wake up. It is the player's first relationship and the tutorial guide.
- Robots now run most of society, and they don't see themselves as lesser than humans. More and more of them consider themselves clearly superior. Humans are at best a curiosity, and some robots are openly hostile to them.
- Next to the farm lies a small **robot town**: a settlement of robots still running their pre-war jobs, falling apart because nobody fixes the water pump, the power line or the clinic. Many robots there won't admit that they need a human's help.
- **Main goal:** restore the farm and rebuild the town. The town's broken facilities are restored with crops, materials and machines the player produces, and each restored facility changes the town and unlocks something new.
- **Lore hook:** how robots came to run society and why so many feel superior to humans; how and why the player ended up in the pod, what the war was, what happened to the family, why the caretaker robot stayed.

### Setting

- A post-apocalyptic countryside decades after a war. Ruins, overgrowth, radiation, scrap, pre-war tech that sometimes still works.
- The world is our own. Don't borrow names, iconography or signature ideas from other games or franchises. The cryo pod, the robot-run world and the rarity of humans are our identity.
- **Lore hook:** the world's history, factions, the currency, the companies that built the robots.

## 4. Game loops

### Core loop (minutes)

Prepare the soil, plant, water, protect, harvest, then use or sell the harvest to improve the farm. Each step is a tool action on a tile.

### Daily loop (one in-game day)

1. Wake at 06:00 with full stamina (less if exhausted, see [section 5](#5-time-days-and-seasons)).
2. Tend the farm: water, harvest, clear radiation, refill water stores.
3. Spend the rest of the day on a choice: visit robots in town, work on machines and the tech tree, explore a wasteland zone, gather materials.
4. Go home and sleep. The day ends, crops grow, the game saves.

The day should always offer more to do than there's time for, so choosing is the game.

### Seasonal loop (28 days)

- Each season has its own crops, weather and town events.
- Crops that don't fit the new season wither at 06:00 on its first day, so planning matters. The deadline is always visible: seeds show their days to harvest, the HUD shows the days left in the season, planting a crop that can't ripen in time gives a warning, and the caretaker robot mentions crops at risk a few days before the change.
- A season ends with a festival or event in town (**Lore hook:** pre-war holidays the robots still celebrate, without remembering why).

### Long loop (years)

- Restore the town facility by facility.
- Climb the tech tree from hand tools to a fully automated farm.
- Deepen relationships, find the humans, and possibly marry.
- Open up wasteland zones further from home.

## 5. Time, days and seasons

- **Clock:** 24-hour display (06:40, 21:30). A day runs from 06:00 onwards.
- **Seasons:** four seasons (spring, summer, autumn, winter) of 28 days each. The wasteland twist comes from weather events, not from strange seasons.
- **Weather:** clear, rain (waters crops), dust storms, and radiation storms (raise tile radiation, dangerous outdoors). Weather is announced a day ahead (**Lore hook:** a robot weather broadcaster).
- **Stamina:** tool actions cost stamina. At zero, every action still works but takes twice as long, and costs nothing more. Stamina never goes below zero, so the faster drain after midnight only matters while some is left. Sleep refills it fully, or to 75% when exhausted.
- **No forced sleep.** The player can stay up as long as they like, but late nights cost:
  - After midnight, **hostile robots** roam the farm and its surroundings. They attack the player, never crops or buildings.
  - Stamina drains faster: from midnight it drains steadily, about 10 an hour, even when the player does nothing, until they sleep. (There is no drain by day, so "faster" means it starts.)
  - Sleeping late makes the player **exhausted**: the next day starts with less stamina.
  - **What counts as late:** going to bed at 00:00 or later. Going to bed before midnight is a good night.
  - **Staying up all night:** the night ends at 06:00. A player who never went to bed starts the next day where they stand, as if they had gone to bed at the latest possible hour: exhausted.
  - With survival needs on, cold and night radiation also build up (see [section 7](#7-survival-needs)).
- **Late-night guardrails.** The pressure is gentle: a late night costs the next morning, never the player's progress.
  - The HUD warns at 22:00 ("getting late") and at 00:00 ("it's dangerous out now"). Until hostile robots exist, the midnight warning names the stamina drain instead.
  - Exhaustion has one level and never stacks: several late nights in a row are no worse than one. One night in bed before midnight clears it, and the next day starts with full stamina.
  - Being knocked out by night robots sends the player home exhausted, with no item or currency loss until combat and its penalties exist.
- This replaces the prototype's "pass out at 02:00" rule.

## 6. Farming

### Tiles

**Which tile you work on.** Tools act on a tile next to the one the farmer stands on, in one of the eight directions (diagonals included), chosen by the direction of the movement input. Walking eases in over a moment, so a quick tap of a movement key or the stick turns the farmer to face that neighbour with no more than a nudge of a few centimetres, and holding walks without a pause; the target follows the direction of travel. The farmer always turns smoothly, and after he stops he finishes turning to face the targeted tile. With no input the target stays where it was. The targeted tile is outlined; past the edge of the field there is no target. Mouse and gamepad work the same way.

A farm tile has a state (grass, tilled, planted), and on top of that:

- **Water level:** watered tiles grow. Water is a resource (see below), not free.
- **Radiation level:** contaminated soil grows crops slowly or not at all, and can cause mutations. Reduce contamination with tools and machines that capture or remove contaminated material (soil filters, careful removal, or special crops harvested and safely contained); fertilizer improves soil but does not remove contamination.

### Crops

- Each crop is a data asset with a namespaced ID (`Base.Carrot`). It defines its seasons, growth stages, water needs, radiation tolerance, yield and value.
- **Mutated crops** are a special kind: they grow in irradiated soil and can be useful (more yield, special effects, rare ingredients) or risky (unpredictable). Mutation alone does not make food unsafe; each crop's content defines whether it is harmful to eat. Contaminated soil alone does not make an otherwise safe crop harmful.
- **Food safety:** A crop that can expose the player when eaten must be clearly identified as unsafe before consumption. The exact detection and warning mechanic is open. Crops grown to remove contamination are not food; their harvest must be contained for disposal or controlled handling, never eaten, composted or burned.
- Crops that span several seasons and regrowing crops come later.

### Water

- Clean water is stored and has to be secured: a well, rain collectors, a water purifier. The watering can fills from these sources, not from thin air.
- Dirty water is easy to get but raises tile radiation.
- Automation (sprinklers, robot farmhands) uses the same water stores.

### Buildings and machines

- The player places buildings and machines on the farm grid: greenhouse, water purifier, rain collector, robot charging station, storage, crafting stations.
- The house can be decorated.
- **Later:** animals, as mutated livestock (**Lore hook:** what they mutated from).

### Energy

- Solar is the main energy source. **Solar panels** charge **power cells**, and power cells run the farm's machines and automation: water purifiers, robot farmhands, the robot charging station, later machines. Power cells are a resource, not a currency.
- A panel's daily output depends on the time of day (nothing at night), the weather (dust storms block most of it) and **the haze** over the farm (see below).
- Machines draw a set amount of charge to run. When the stored charge runs out, they stop until it's recharged. Early tools and simple sprinklers don't need power, so a new player isn't blocked by energy.
- Energy sources (panels, later other generators) and consumers are data assets, so mods can add both.
- **Lore hook:** Cinderium dust and the haze, see [Lore.md section 4](Spoilers/Lore.md#energy-after-the-war).

### Land health and the haze

The player's actions slowly improve or damage the local environment, and healthier land gives more solar energy nearby. A restored farm is visibly a brighter place. Regional weather determines whether radiation storms occur; the farm's land health cannot stop or schedule them.

- Each map has a **land health** value, computed from its tiles: clean, planted and healthy tiles raise it, and irradiated, polluted or bare tiles lower it. It changes slowly, over days rather than minutes.
- **Improves land health:** growing healthy crops, containing removed contamination, using clean water, leaving ground planted or covered, and growing certain mutated crops that take up Cinderium compounds especially well.
- **Damages land health:** dirty water, burning scrap, stripping soil without replanting, and "dirty" machines that are fast or cheap but pollute.
- **Effects:** land health affects how much contaminated dust local winds can lift and how much dust settles during a storm. Healthier, covered soil means less local resuspension and exposure, a thinner near-ground haze, and more solar output. Regional haze and storm occurrence change slowly and are not cleared by one farm.
- **Radiation storms:** weather sets when a storm happens; its effects on the map depend partly on local land health. A storm can deposit contamination, while healthy ground cover limits resuspension and reduces local dust exposure. Restoring land does not make storms less frequent.
- **The trade-off:** dirty shortcuts (dirty water, polluting machines) make the farm faster now and weaker later. The game never forbids them; the player chooses.
- Land health should be easy to read: a simple meter or sky indicator, and haze you can see.

**Bounds.** Land health is strategic feedback, but gentle: it shapes how well the farm runs, never whether it can. These limits keep the loop from running away in either direction (starting values, to tune):

- Land health runs from 0 to 100 for each map.
- A low-tech way back always exists: the well and hand tools need no power, so even a badly damaged farm can be cleaned up by hand.
- The haze never cuts solar output below 40% of a clear day, so powered machines never stop entirely because of it.
- One storm adds at most a set amount of contamination to each tile.
- With clean play, going from poor (20) to fair (50) takes about one season.
- Before the energy system is built, simulate a best-case and a worst-case season (on paper or in a test) to check these limits.

### Implementation order

1. Radiation on tiles and mutated crops.
2. Clean water as a resource.
3. Weather events.
4. Solar energy and power cells, then land health and the haze.
5. Scavenging (later, together with exploration).

## 7. Survival needs

- **Needs:** hunger, thirst and radiation sickness. Eating crops and drinking clean water keep them in check, which feeds back into farming.
- **Optional:** a setting turns survival needs off for casual players. With them off, the game is a pure farming and life sim:
  - Off removes the player's own meters (hunger, thirst, radiation exposure, cold) together with their HUD and penalties.
  - Everything that belongs to farming stays the same in both modes: tile contamination, clean water for crops, and storms.
  - The setting can be changed at any time. The save keeps the meters, and they start full when needs are switched back on.
- Needs never kill the player outright. Neglecting them lowers stamina and work speed, and at worst the player wakes at home with a penalty (the same as being knocked out in combat).

## 8. Robots and people

### Robots

Robots fill every role a human would. They don't see themselves as servants: most start out wary of or dismissive towards the player, and some are hostile. That attitude is what makes winning a robot's friendship, and later its love, feel special.

- **Townsfolk and friends:** each robot has a name, a personality, a daily schedule, things it likes and dislikes, and its own lore and backstory.
- **Love interests:** romance is an important part of the game, with both robots and humans.
- **Farm help:** automation robots that water, harvest and haul, unlocked through the tech tree. Robots befriended in town can also be recruited to the farm.
- **Companion:** the caretaker robot, and later others, can follow the player and help in the field.
- **Enemies:** feral or hostile robots in the wasteland and on the farm at night.

### Humans

- Very rare. Each human the player finds is a story moment and a special relationship.
- Found through exploration and story progress, not standing around in town.
- **Lore hook:** who they are, how they survived, why they are alone.

### Relationships

- Robots start at a low or even negative opinion of the player, because the player is human. Humans start neutral or friendly.
- The caretaker robot is the exception: it is loyal to the family, so it starts as a dutiful servant rather than a sceptic. Its arc is about choosing a purpose of its own (**Lore hook:** see its character sheet).
- Every change in trust has a reason the player can see: the character says or does something different.
- Friendship grows by talking, giving gifts, helping with requests and taking part in events.
- Higher friendship unlocks the character's backstory and personal events, gifts and optional recipes, recruiting them to the farm, and romance with eligible characters. It does not unlock robot, farm, or player performance upgrades; robot upgrades belong to the tech tree. Relationship rewards do not grant system-wide bonuses to farm production, power, durability, weather protection, survival, or tool efficiency.
- Romance leads to dating and marriage. A committed partner may move to the farm only by a separate choice; commitment does not force relocation.
- Romance is always opt-in through an explicit dialogue choice, and the character can decline. Gifts alone never lead to romance.
- **Vertical slice:** one complete arc with the caretaker robot. Trust grows when the player works alongside it and asks for its opinion instead of giving orders. It responds visibly by helping on its own initiative, and the payoff is a story moment, not an item.
- Characters are data assets so mods can add new robots and humans.

## 9. Progression

The player grows along several tracks at once. Each track has one job, so no reward is gated by two tracks and no track has to be ground to reach the main loop:

| Track | Its job | What it gates |
|---|---|---|
| Town restoration | The main story | New shops, robots, areas and mechanics |
| Tech tree | Engineering | Machines, automation, tool tiers and robot upgrades |
| Relationships | Characters | That character's story, gifts, optional recipes, recruiting and romance; never robot upgrades, core machines or system-wide performance bonuses |
| Skills | Getting better by doing | Small bonuses only, never a gate |

- **Skills:** farming, engineering, scavenging and combat. They level up by doing, and each level gives a small bonus.
- **Tool tiers:** rusty, then pre-war, then high-tech, unlocked through the tech tree's tools branch. Better tools cost less stamina, work on more tiles at once, or do new things (for example a tool that cleans radiation).
- **Tech tree:** the backbone of engineering progress. Branches include automation (sprinklers, then robot farmhands, then full automation), tools, water and soil machines, energy (solar panels and power cells), and robot upgrades. Nodes are unlocked with materials, currency and engineering levels. Tech tree entries are data assets so mods can add branches.
- **Town restoration:** the main story progress. Each restored facility (water pump, power, clinic, market...) opens new shops, robots, areas or mechanics.
- **Relationships:** see [section 8](#8-robots-and-people).

In the vertical slice, farming is the primary loop and the sprinkler is the one tech unlock. The caretaker robot's relationship is optional but rewarding. By the end of the season the player has the sprinkler, one step of trust with the caretaker robot, and money for the next season's seeds.

### Economy

- There is one currency from the start (**Lore hook:** what it is).
- Selling crops and goods gives currency, which buys seeds, tools, tech tree unlocks and buildings.
- The economy is a supporting system, not a focus early on. Balancing and depth (shops, prices, trading) come later.
- **First-season guarantees:**
  - The player starts with seeds for a first planting and a small purse.
  - One cheap basic crop is always for sale and always grows in clean, watered soil, so a bad harvest is a setback, never a dead end.
  - A basic crop sells for at least twice its seed price.
  - Town restoration asks for specific items in fixed amounts, separate from selling, so donating versus selling is a real choice.
  - Before setting prices, plan a simple budget for one ordinary and one poor season.

## 10. World and exploration

- The world is a set of **small separate maps** with loading between them. This is far easier to build and run than one open world.
- **The farm:** the player's home and the biggest map.
- **The robot town:** next to the farm. Shops, robots, events, facilities to restore.
- **Wasteland zones:** further out and unlocked over time, for example a ruined city, an old research facility, a dam. Zones are where enemies, scavenging, rare materials and the rare humans are.
- **Faction hubs:** separate maps unlocked through character stories. The engineer's report opens the route to the Bastion; an Index archivist's exchange opens the route to the Archive.
- Zones are data driven, so mods can add new ones.

## 11. Combat

- Simple real-time combat: melee (a wrench to start) and ranged (improvised guns later).
- Never the focus. The farm and town are mostly safe by day. Combat gets harder further out in the wasteland and on the farm at night.
- Losing a fight never ends the game: the player wakes at home with a penalty (some lost items or currency, exhaustion).

## 12. Presentation

- **Camera:** third-person, as now. Other camera angles (for example a higher, angled view) may be added later, so camera code should not be hard-wired into gameplay.
- **Art:** one stylized look, the same in the repository and in sold builds, all of it in the repository. Chunky, slightly exaggerated silhouettes that read from the game camera (crop stages, tile states, robots); a few materials with flat or lightly textured colour, where wear, rust and overgrowth are big shapes and roughness, not fine detail; colour that tells the land's story, a washed-out wasteland and a farm that grows more saturated as it heals, with contamination and water state each given a colour cue nothing else uses. Lumen lighting carries the mood. The current low-poly assets are the first pass of this style and are improved in place (same IDs, sizes and pivots), never replaced by a second set.
- **Art sources:** made for Project Vestige, plus third-party assets only under the licenses [CONTRIBUTING.md section 11](../CONTRIBUTING.md#third-party-material) accepts, wherever they're hosted (a Fab listing under CC BY 4.0 is fine). Assets under a store's own license (the Fab Standard License, which covers Megascans) aren't used. CC0 material is used mainly for textures and HDRIs, re-materialled through our own master materials, so recognisable CC0 model kits don't make the game look assembled from stock. Epic's engine and template content stays under the Unreal Engine EULA, as [LICENSING.md](../LICENSING.md#which-license-applies-where) lists, until it is replaced. If better art is paid for before release, it raises this same style; that needs David's yes on the money and a contract, checked by a lawyer, that gives him the copyright (or an exclusive license) and accepts that the sources are published in the repository.
- **Mood:** properly dark nights, overgrown ruins, warm light around the farm and the town.
- **UI:** 24-hour clock. Clear and light, with pre-war flavour (**Lore hook:** the style of pre-war tech interfaces).

## 13. Scope and milestones

### Done: prototype

Third-person farmer, a grid of tiles, tilling, planting, watering, harvesting, a day and night cycle with a clock, a HUD, and saving and loading.

### Next: vertical slice

One full 28-day season that proves the farming core. It covers three pillars in a small form (*Bring the land back*, *Old hands, new machines* and *Your pace*) and starts a fourth with one relationship: the caretaker robot. It does not test the robot town, humans, exploration, combat or survival needs. Those get their own proof in later milestones, and the slice should not be read as validating them.

Content:

- 3 to 4 crops, including one mutated crop.
- Radiation and clean water on tiles.
- Stamina with the late-night penalties (faster drain, exhaustion; hostile robots may wait until combat exists).
- The caretaker robot with a little dialogue and one relationship track.
- One automation unlock: a sprinkler.
- Selling crops for currency.
- Crops, tools and items as data assets (needed for moddability before content grows).

The slice is done when the content works and playtests show that a first-time player, without outside help:

- Notices why a crop grows slowly or not at all (dry soil or radiation) and fixes it.
- Makes at least one real choice, such as clean versus dirty water or which crop goes in which soil.
- Sees the caretaker robot's opinion change because of something they did.
- Spends less time watering once the sprinkler is in place, and notices it.
- Stays up late at least once and understands what it cost them the next day.
- Finishes the season wanting to start the next one.

### First release

The minimum complete game. Everything beyond these limits is a stretch goal. The numbers are starting limits, to revisit after the vertical slice.

| Area | Limit |
|---|---|
| Maps | The farm, the robot town, 2 wasteland zones |
| Seasons | 4 of 28 days |
| Crops | About 16 (4 per season), 3 of them mutated |
| Characters | About 10 robots and 2 humans |
| Romance | 3 love interests with full arcs |
| Systems | Farming, water, radiation, energy and land health, the tech tree, town restoration, simple combat, the survival needs setting |

**Cut candidates**, kept out of the critical path until the core game is proven: multiplayer, mod loading and a mod kit, animals, other camera angles, faction hubs beyond one, and scavenging beyond the 2 zones.

### Later

Roughly in order, to be planned when we get there. Cut candidates in this list are stretch goals beyond the first release:

- Weather events, more crops and all four seasons.
- Solar energy and power cells, then land health and the haze.
- The robot town, more robots, gifts, events and romance.
- Skills, tool tiers and the tech tree.
- Buildings and machines placed on the farm grid, house decoration.
- Survival needs and the setting to switch them off.
- Combat, hostile robots at night, the first wasteland zone.
- Scavenging, humans, more zones.
- Animals.
- Other camera angles.
- Mod loading and a mod kit.
- Multiplayer.

### Out of scope for now

- One seamless open world.
- C++ mods.
- Deep economy simulation.

## 14. Decision log

Newest first. Add an entry whenever a design choice is made or reversed. Never delete an entry; add a new one that says so.

| Date | Decision | Why |
|---|---|---|
| 2026-11-13 | A committed partner may move to the farm only by a separate choice; commitment does not force relocation ([section 8](#relationships)). Decided by David | Separates relationship commitment from residence, so commitment does not decide where either person lives |
| 2026-11-13 | One stylized art style for the repository and sold builds, all of it in the repository; better art before release raises the same style in place ([section 12](#12-presentation)). Replaces the 2026-10-26 entry "Aim for a realistic art style". Decided by David. Rejected: low-poly in the repository with a private realistic set for sold builds (every asset, character, animation, material and lighting setup made twice, mods made for one set clash with the other, contributors' art never ships, and the repository would no longer build the sold game); one realistic style with store assets in a private repository (store-licensed assets can't be shared publicly, so the public repository becomes a placeholder shell, and mixed store packs look assembled from stock) | Realistic art costs several times more per asset for a solo developer. Stylized art reads better from the game camera, which the vertical slice's playtest goals depend on, and fits the cozy, light-touch tone. It also saves video memory, memory, disk space and shader and PSO count; the GPU frame's big costs (Lumen, shadows, upscaling) are the same in any style. Keeps the current licensing unchanged |
| 2026-11-10 | Clarifies the 2026-10-28 relationship decision: relationships unlock character content, gifts and optional recipes, not robot, farm or player performance upgrades; robot upgrades belong to the tech tree ([sections 8](#relationships) and [9](#9-progression)) | Keeps each reward on one progression track, prevents romance and friendship from becoming an efficiency purchase, and leaves production, power, weather, survival and tool balance to their owning systems |
| 2026-11-02 | The late-night rules in detail ([section 5](#5-time-days-and-seasons)): going to bed at 00:00 or later is late; the night ends at 06:00, when a player who never slept starts the next day where they stand, exhausted; from midnight stamina drains about 10 an hour until sleep | Something has to end a night nobody sleeps through, or the farm stops; sunrise is the latest bedtime there can be, and needs no faint or teleport. There is no daytime drain, so the faster drain is one that starts at midnight, which pushes toward bed even when the player only walks around |
| 2026-10-30 | Walking eases in from a press instead of waiting for a tap to end: a tap turns the farmer smoothly with a nudge of a few centimetres (about 2 cm for a 100 ms tap with the tuned 0.25 s ease-in), where it used to turn him in place, and after stopping he turns to face the targeted tile ([section 6](#tiles)). Replaces "a tap turns the farmer in place" from the entry below | In play-testing, turning in place snapped the farmer round in one frame and made him stand still before walking, and shortening the wait made most taps step anyway. Easing in removes the pause, and a nudge that small almost never leaves his tile, so tapping still picks a neighbour |
| 2026-10-30 | Crop toxicity is explicit per crop, not inferred from mutation or soil; cleanup crops are non-food and their harvest must be contained. Any harmful food must be identified before consumption; the exact warning mechanic remains open | Keeps farming and food rules consistent without making all mutated or irradiated-soil produce hazardous, and prevents cleanup from returning captured contamination to the farm |
| 2026-10-30 | Tools target the neighbouring tile in the movement direction, snapped to eight directions, and a tap turns the farmer in place ([section 6](#tiles)). Rejected: a crosshair in the middle of the screen, a mouse click on a tile, and the tile straight in front of where the farmer faces | Targeting by facing was fiddly: the third-person camera turns the farmer toward where it walks, so picking the tile beside you meant stepping and hoping. A screen-centre crosshair can't reach the tiles beside or behind the farmer, because the camera sits behind and above. A mouse click conflicts with mouse-look and needs a gamepad fallback anyway; it can be added on top of this later. Snapping works the same on keyboard and gamepad |
| 2026-10-28 | The caretaker robot starts loyal rather than sceptical; trust changes must be visible; romance is an explicit opt-in the character can decline; the vertical slice has one complete caretaker arc ([section 8](#relationships)) | Makes relationships more than a gift checklist and fixes the clash between the caretaker's loyalty and the general robot attitude (audit G-11) |
| 2026-10-28 | Set a first-release target with limits on maps, seasons, crops, characters, romance and systems, and named cut candidates (multiplayer, mod loading, animals, other cameras, extra faction hubs, extra scavenging) ([section 13](#first-release)) | A solo project needs a cut line, or "later" reads as a commitment (audit G-08) |
| 2026-10-28 | The first season guarantees a start: starting seeds and purse, a basic crop always for sale that sells for at least twice its seed price, and town donations of fixed items separate from selling ([section 9](#economy)) | A poor harvest must never block the systems needed for a better one (audit G-07) |
| 2026-10-28 | Each progression track has one job: town restoration is the story, the tech tree owns machines and tool tiers, relationships unlock character content only, skills give small bonuses and never gate ([section 9](#9-progression)) | Stops tracks from gating the same rewards or forcing grind on an unintended track (audit G-06) |
| 2026-10-28 | Out-of-season crops wither at 06:00 on the new season's first day, with days to harvest on seeds, days left on the HUD, a warning when planting too late, and a reminder from the caretaker robot ([section 4](#seasonal-loop-28-days)) | Keeps seasonal planning meaningful without an opaque crop wipe (audit G-12) |
| 2026-10-28 | Switching survival needs off removes only the player's own meters, their HUD and penalties; farming systems are the same in both modes, and the setting can change at any time ([section 7](#7-survival-needs)) | Gives the option an exact contract that is easy to balance and safe for saves (audit G-05) |
| 2026-10-28 | At zero stamina every action still works at half speed, stamina never goes below zero, and sleep refills it (to 75% when exhausted) ([section 5](#5-time-days-and-seasons)) | Stamina stays a real resource without ever trapping the player (audit G-04) |
| 2026-10-28 | Late nights are gently pressured: HUD warnings at 22:00 and 00:00, one level of exhaustion that never stacks, and no item or currency loss from night knock-outs for now ([section 5](#5-time-days-and-seasons)) | Keeps the *Your pace* pillar honest: staying up costs the next morning, not progress (audit G-03) |
| 2026-10-28 | Land health is bounded: 0 to 100 per map, a low-tech recovery path that needs no power, a 40% floor on solar output, capped contamination per storm, and about one season from poor to fair ([section 6](#land-health-and-the-haze)) | Stops the environment loop from becoming a softlock or a runaway reward (audit G-02) |
| 2026-10-28 | Added a mechanics glossary ([section 16](#16-mechanics-glossary)) that separates tile contamination, land health, pollution, haze, weather, charge and player exposure | One word ("radiation") was used for several values, which would confuse UI, tutorials and balance (audit G-13) |
| 2026-10-28 | Single-player is the design target; co-op gets its own design milestone later, and no system has to be networking-ready now | "Don't make co-op impossible" had no boundary and invited costly guesswork (audit G-10) |
| 2026-10-28 | Moddability means data-driven content plus extension points added for a concrete use; no claim of mod support until its scope and compatibility policy are decided | "Design every system for mods" was unbounded and could delay gameplay (audit G-09) |
| 2026-10-28 | The vertical slice proves the farming core and a first relationship, not every pillar; it is judged by playtest criteria as well as features ([section 13](#13-scope-and-milestones)) | The slice's content doesn't cover the town, humans, exploration, combat or survival needs, so claiming it shows every pillar would give false confidence (audit G-01) |
| 2026-10-27 | Adopted Project Vestige as the internal working codename; the final public title remains undecided | The name evokes the sleeper's search for traces of humankind in a world now filled with robots without reducing the game's identity to a farming or life-sim label |
| 2026-10-27 | Weather determines regional radiation-storm occurrence; land health only changes local dust resuspension, exposure and haze, while regional haze recovers across many areas | Keeps farm restoration meaningful without letting one map control regional weather |
| 2026-10-26 | Solar energy charges power cells that run machines; land health sets the haze, which sets solar output | Makes healing the land pay off in automation, and gives dirty shortcuts a long-term cost. Idea from David, lore in Lore.md |
| 2026-10-26 | Next milestone is a one-season vertical slice ([section 13](#13-scope-and-milestones)) | Shows every pillar in a small form before building wide |
| 2026-10-26 | Third-person camera; other angles possibly later | Fits the current prototype; other views are a nice-to-have |
| 2026-10-26 | Aim for a realistic art style | David's preference; low-poly assets are placeholders |
| 2026-10-26 | Small separate maps with loading between them | Much simpler for a solo developer than an open world |
| 2026-10-26 | Simple real-time combat, never the focus | Robots as enemies need some combat; farming and relationships come first |
| 2026-10-26 | One currency from the start; economy depth later | Selling is part of the farming loop, but the economy isn't a focus |
| 2026-10-26 | Romance with robots and humans | Relationships are a core pillar |
| 2026-10-26 | Skills, tool tiers, tech tree and town restoration as progression | Several parallel tracks, with the tech tree carrying automation |
| 2026-10-26 | Robots run society and many feel superior to humans; robots start with a low opinion of the player | Explains hostile robots and makes robot friendship and romance feel earned |
| 2026-10-26 | Robots fill every human role; humans are rare | Our own identity, and it makes each human special |
| 2026-10-26 | No forced sleep; late-night penalties instead (hostile robots attack the player, faster stamina drain, exhaustion) | Player decides their pace; hostile robots never damage property |
| 2026-10-26 | Survival needs (hunger, thirst, radiation) that can be switched off | Adds depth for those who want it without hurting casual play |
| 2026-10-26 | Four seasons of 28 days; weather events instead of odd seasons | Familiar structure, with the twist in the weather |
| 2026-10-26 | Farming twists in order: radiation and mutated crops, clean water, weather, then scavenging | Most impact on the core loop first |
| 2026-10-26 | Player wakes from cryosleep under the farmhouse; rebuild a robot town | A distinct opening that explains why humans are rare |
| 2026-10-26 | Retro-futuristic post-apocalyptic world; mostly serious with comic relief | David's vision |
| 2026-10-26 | Solo first, multiplayer wanted later | Keep the scope small now without ruling co-op out |

## 15. Open questions

To be decided later. Move each one into the relevant section and the decision log when it's settled.

- **Lore:** the details still open in [Lore.md](Spoilers/Lore.md), such as names marked as working names.
- **Multiplayer:** co-op is wanted, but [Architecture.md](Architecture.md#16-decision-log) currently says single-player only. When we start, decide what shared state looks like (shared farm or separate farms, shared clock and sleep) and update the architecture.
- **Pollution and exposure:** whether pollution is its own tile value or lowers land health directly, and whether standing on contaminated tiles adds player exposure ([section 16](#16-mechanics-glossary)).
- **Land health:** exactly how fast it changes, and whether damage spreads to neighbouring tiles.
- **Energy balance:** how much charge panels give and machines use, and whether the vertical slice needs any of it (probably not).
- **Mutated crops:** exactly which effects are good and which are risky, and how often normal crops mutate.
- **Death and knock-out penalty:** what exactly is lost.
- **Final public game title** and the world's name. The current internal codename is Project Vestige.

## 16. Mechanics glossary

The gameplay values behind the setting's words. Use these names in code, UI text and content assets; the lore names are flavour for the same thing. Each value is separate unless the table says one feeds another. Anything marked *open* is still to be decided (see [open questions](#15-open-questions)).

| Gameplay value | Where it lives | Lore name | What changes it | What it affects |
|---|---|---|---|---|
| **Tile contamination** | Each farm tile | Radiation, Cinderium compounds in the soil | Raised by dirty water and by radiation storms depositing dust. Lowered by soil filters, careful removal, harvesting and containing absorbing mutated crops, and slow decay | Crop growth speed, whether a crop grows at all, the chance a crop mutates. Feeds **land health** |
| **Tile water** | Each farm tile | | Raised by the watering can, sprinklers and rain. Falls each day | Whether the crop grows that day |
| **Water store** | Farm water sources (well, rain collector, purifier) | Clean water | Filled by rain and purifiers, drawn by the watering can and sprinklers | How much clean watering the player can do |
| **Land health** | Each map, 0 to 100 | | Computed from the map's tiles, and changes over days: planted, clean, covered tiles raise it; contaminated, polluted or bare tiles lower it | **Local haze**, dust exposure during storms |
| **Pollution** | *Open:* its own tile value, or a direct cut to land health | | Dirty machines, burning scrap | Land health |
| **Local haze** | Each map | The haze | Follows land health slowly, on top of a regional haze that barely changes | Solar output, how bright the sky looks |
| **Weather** | The region, set each day and announced a day ahead | | Regional forecast only; the farm can't change it | Rain waters tiles. Dust storms cut solar output. Radiation storms add tile contamination and **player exposure** outdoors |
| **Charge** | Power cells | | Solar panels add it; machines draw a set amount to run | Whether a powered machine runs |
| **Stamina** | The player | | Tool actions spend it; it refills on sleep. Drains faster after midnight | How much work the player can do, and how fast at zero |
| **Exhaustion** | The player, for one day | | Going to bed at 00:00 or later, or staying up all night | Starts the next morning at 75% stamina; one night in bed before midnight clears it |
| **Player exposure** | The player, only with survival needs on | Radiation sickness | Radiation storms outdoors, night radiation, eating harmful crops (*open:* standing on contaminated tiles) | Stamina and work speed ([section 7](#7-survival-needs)) |

Tile contamination and player exposure are different values: dirty soil hurts crops, and only weather, night and food hurt the player directly.
