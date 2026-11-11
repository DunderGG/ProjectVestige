# 14. Audio

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** none yet.

**References:** Epic's "MetaSounds", "Sound Classes", "Sound Mixes", "Audio Modulation" (control buses), "Sound Concurrency" and "Submixes" pages; Lyra's audio setup: control buses and bus mixes for the volume settings, and context effects for surface-dependent footsteps.

**Rules:**

- **Sounds react to events** ([Architecture.md, section 7](../Architecture.md#7-presentation-ui-audio-and-effects)): game code broadcasts (tile watered, crop harvested); audio code listens and plays. With the Gameplay Ability System, gameplay cues play them.
- **MetaSounds** for anything with variation or logic (random pitch, layered water splash); plain sound waves for one-shots.
- **Sound classes and a control bus per volume slider** (master, music, effects, ambience, voice), set from the settings, as Lyra does.
- **Concurrency limits** on frequent sounds (footsteps, tool hits), and attenuation on every world sound.
- Music and ambience that continue across maps belong to a game-instance subsystem.
- Audio assets are soft references in content data (an `Audio` bundle).

**Pitfalls:**

- `PlaySound2D` scattered through gameplay code.
- Hundreds of looping ambient sounds placed in the level without concurrency or virtualization settings.
