# Project Vestige system guides

How each game system is built. [Architecture.md](Architecture.md) says how the systems fit together and who owns what; [CodingGuidelines.md](CodingGuidelines.md) says how the code looks. The guides go one level down: for each system the game has or will have, what Epic recommends, what the Lyra Starter Game does, what we adopt, and the mistakes to avoid.

## How to use this document

- **Read the system's guide before building or changing it.** The guide's rules apply on top of the general ones.
- **No system without a guide.** Before the first code for a system that has no guide file in `Systems/` (or only a "planned" one), write or finish its guide: read Epic's documentation for the engine feature, read how Lyra does it (the Lyra source is free on Fab and installs as a project from the Epic launcher), and write down what we adopt, what we skip and why. The guide is approved before the code: its technical choices by a senior Unreal reviewer, its game design choices by David ([decision log](Architecture.md#16-decision-log), 2026-11-11). Adopting an engine framework (the Gameplay Ability System, Common UI, StateTree, Mass) is also a [decision log](Architecture.md#16-decision-log) entry.
- **A new guide is a new file,** `Systems/<NN>-<Name>.md` with the next number, plus a heading in this index that links to it, in the same commit. A guide's number never changes.
- **Update the guide when the system changes.** The guide describes the system as built plus the rules for growing it. When code and guide disagree, fix one of them in the same commit.
- **Lyra at our scale.** Lyra is built for a large team and networked multiplayer. We take its structure (components, data assets, tags, subsystems) and skip what only multiplayer or a large team needs (replication, prediction, Experiences, most of its plugin setup), as [Architecture.md, section 11](Architecture.md#11-patterns-borrowed-from-lyra) says. Each guide names what we skip.
- Performance rules are in [CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance) and apply to every system; each guide only adds what is specific to it.

Each guide has the same parts: **Status** (built, partly built or planned, and the roadmap milestone), **References**, **Rules**, **Pitfalls**.

The guides, one file each in [`Systems/`](Systems/). The numbered headings below never change, so "Systems.md, section N" always finds guide N.

---

## 1. Saving and loading

[Systems/01-Saving.md](Systems/01-Saving.md)

## 2. Content and the Asset Manager

[Systems/02-Content.md](Systems/02-Content.md)

## 3. Gameplay tags

[Systems/03-GameplayTags.md](Systems/03-GameplayTags.md)

## 4. Time, days and seasons

[Systems/04-Time.md](Systems/04-Time.md)

## 5. Input

[Systems/05-Input.md](Systems/05-Input.md)

## 6. User interface

[Systems/06-UserInterface.md](Systems/06-UserInterface.md)

## 7. Interaction

[Systems/07-Interaction.md](Systems/07-Interaction.md)

## 8. Inventory and items

[Systems/08-Inventory.md](Systems/08-Inventory.md)

## 9. Abilities, attributes and effects (Gameplay Ability System)

[Systems/09-AbilitySystem.md](Systems/09-AbilitySystem.md)

## 10. Robots: AI, routines and crowds

[Systems/10-Robots.md](Systems/10-Robots.md)

## 11. Dialogue and relationships

[Systems/11-Dialogue.md](Systems/11-Dialogue.md)

## 12. Camera

[Systems/12-Camera.md](Systems/12-Camera.md)

## 13. Animation

[Systems/13-Animation.md](Systems/13-Animation.md)

## 14. Audio

[Systems/14-Audio.md](Systems/14-Audio.md)

## 15. Visual effects and feedback

[Systems/15-EffectsAndFeedback.md](Systems/15-EffectsAndFeedback.md)

## 16. Placing objects and building

[Systems/16-PlacingAndBuilding.md](Systems/16-PlacingAndBuilding.md)

## 17. Settings

[Systems/17-Settings.md](Systems/17-Settings.md)

## 18. Localization

[Systems/18-Localization.md](Systems/18-Localization.md)

## 19. Debugging and cheats

[Systems/19-DebuggingAndCheats.md](Systems/19-DebuggingAndCheats.md)

## 20. Maps, loading screens and the main menu

[Systems/20-MapsAndMenus.md](Systems/20-MapsAndMenus.md)

## 21. Logging

[Systems/21-Logging.md](Systems/21-Logging.md)

## 22. Farm fields and tiles

[Systems/22-FarmFields.md](Systems/22-FarmFields.md)
