# 10. Robots: AI, routines and crowds

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** planned. Hob, the caretaker robot, is milestone 2; the town is milestone 3.

**References:** Epic's "StateTree", "Behavior Trees", "Smart Objects", "AI Perception", "Mass Entity" and "Significance Manager" pages. Lyra's bots use behavior trees with an `AAIController`, but StateTree is Epic's newer general-purpose state machine, built for Unreal Engine 5, and the one we choose for new AI.

**Rules:**

- **AI controller plus StateTree** for robot behavior. The robot pawn is a body with components; the controller and its StateTree decide what to do. Log the choice of StateTree over behavior trees when the first robot is built.
- **Daily routines are data.** A robot's schedule (where it is at what hour, by season and weather) lives in a data asset, not in the tree. The tree runs "go to the next activity" generically.
- **Smart Objects for places robots use** (a bench, a pump to repair, a shop counter): the place says what can be done there, and robots claim slots. This suits robots repeating pre-war routines and lets mods add places without touching AI code.
- **Robot definitions are content** (`UVestigeCharacterData`, a `UVestigeContentData` subclass of primary asset type `VestigeCharacter`, for humans and robots alike) with the ID (`Base.Gage`, no category segment: [Architecture.md, section 5](../Architecture.md#5-data-and-content)), name, schedule, dialogue and relationship data. Whether a character is a robot or a human is a gameplay tag, not a separate type.
- **Scale by significance.** Robots far away or out of sight update less often (tick interval, perception off, animation update rate optimization). When the town has many simple background robots, consider Mass for them and keep full actors for the named characters.
- Perception (sight, hearing) only for robots that need it, and not every frame.

**Pitfalls:**

- Behavior logic in the robot's Blueprint event graph or `Tick`.
- Pathfinding on every frame: move with `MoveTo` requests and react to completion events.
