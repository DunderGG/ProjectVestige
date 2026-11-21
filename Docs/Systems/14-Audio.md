# 14. Audio

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** built 2026-11-21 for [Roadmap.md](../Roadmap.md), "Set up audio and make the first sound" (decision log, 2026-11-21): the volume categories as sound classes, sounds for feedback reports from `DA_FeedbackResponses`, and the first sound, the splash when the watering can fills at the well, made by a script. The splash was play-tested on 2026-11-21: it plays at the right level and at the right times, but it sounds artificial, with a metallic cling at the start (a better splash is a roadmap item). Not built yet: music, ambience, sounds placed in the world, gameplay cues with sounds, concurrency and attenuation assets, submixes of our own, and the volume settings (Audio Modulation's control buses, with the settings menu, [section 17](17-Settings.md)).

**References:**

- Epic's pages "Audio in Unreal Engine 5" (the overview), "Sound Classes", "Submixes Overview", "Audio Modulation Overview" (control buses, control bus mixes and parameter patches; a plugin, off by default; sound classes, submixes, sound waves and MetaSound sources have modulation destinations), "MetaSounds", "Sound Concurrency", "Sound Attenuation" and "Stream Caching" (the loading behaviour of sound waves).
- UE 5.8 source: `Engine/Classes/Sound/SoundBase.h` (`SoundClassObject`, `SoundSubmixObject`, `ConcurrencySet`, `AttenuationSettings`), `SoundClass.h` (`ParentClass`, `ChildClasses`, `Properties.DefaultSubmix`, `Properties.ModulationSettings`), `SoundWaveLoadingBehavior.h` (the order a wave's loading behaviour is resolved in: the wave, its class, the class's parents, the `au.streamcache` console variables), `Kismet/GameplayStatics.h` (`PlaySound2D`, whose `bIsUISound` keeps a sound playing while the game is paused), `Sound/AudioSettings.h` (`DefaultSoundClassName`) and `BaseEngine.ini` (the engine's default class is `/Engine/EngineSounds/Master`).
- Lyra (`Content/Audio`, `Source/LyraGame/Audio`, `Config/DefaultEngine.ini` and `DefaultGame.ini`): a tree of sound classes (`Overall` with `SFX`, `UI`, `Music`, `VoiceChat`), each with a default submix (`SFXSubmix`, `UISubmix`...) and a volume modulator (`PP_DefaultSFX`) in its modulation settings; the project's default sound class is `Overall`; control buses `CB_Main`, `CB_SFX`, `CB_Music`, `CB_Dialogue` driven by bus mixes (`CBM_BaseMix`, `CBM_UserMix`) that `ULyraAudioMixEffectsSubsystem` (a world subsystem) activates, with the buses named in `ULyraAudioSettings` (developer settings) and set from the player's settings; concurrency assets `SCON_*`; attenuation presets `ATT_*`; sounds for cosmetics through gameplay cues and `ULyraContextEffectComponent` (footsteps by surface).

**What we take from Lyra and what we skip:**

- **Take:** sound classes as the volume categories, in a tree with one root that is the project's default class, so every sound, even one nobody assigned, is under the master volume; the volume sliders as control buses on those classes later; concurrency and attenuation as shared assets, not settings on each sound; cosmetic sounds through gameplay cues.
- **Skip for now:** the Audio Modulation plugin and its buses, mixes and patches (nothing sets a volume until the settings menu exists); submixes of our own and their dynamics and reverb chains (one sound needs no mix bus); the HDR and LDR effect chains, the loading screen mix and the voice chat class; the context effects component, which comes with footsteps.

**How the parts fit:**

| Part | Kind and place | Owns |
| --- | --- | --- |
| `SCL_Overall` | Sound class, `Content/Vestige/Audio/Mix/` | The root of every sound: the project's default sound class (`DefaultEngine.ini`, `[/Script/Engine.AudioSettings]`, `DefaultSoundClassName`), where the master volume attaches later |
| `SCL_Effects` | Sound class, child of `SCL_Overall`, same folder | Sound effects (the splash, later tools, feedback and world sounds), where the effects volume attaches later |
| `FVestigeFeedbackResponse::Sound` | Field of the existing response row, `UI/VestigeFeedbackResponses.h` | The sound presentation plays for a feedback report, next to its text ([section 15](15-EffectsAndFeedback.md)) |
| `AVestigeHUD` | Existing, `UI/` | Playing a report's sound when it shows the report's response |
| `S_Water_FillCan` | Sound wave, `Content/Vestige/Audio/Water/` | The splash of water poured into the can at the well |
| `Art/Audio/make_water_fill_can.py` | Python script, outside the engine | Making `Art/Export/S_Water_FillCan.wav`, the splash's source |
| `EditorScripts/SetUpAudio.py` | Editor script | Making the sound classes, importing the wave and setting the two feedback rows' sound |

**Rules:**

- **Game rules never play a sound** ([Architecture.md, section 7](../Architecture.md#7-presentation-ui-audio-and-effects)). Gameplay reports what happened (a feedback report, an event, a gameplay cue tag) and presentation picks the sound. Three ways in, each for its own kind of sound:
  - **A feedback report's sound** comes from its row in `DA_FeedbackResponses`, played by the HUD as a 2D sound with the report's text. For results the player caused and is told about: the can filled, a refused press, the game saved. The splash is one: the player stands at the well when it plays, so a place in the world would add nothing they can hear. When the well gets a visual splash (a Niagara effect at the well, [section 15](15-EffectsAndFeedback.md)), that effect has a place, so it comes through a gameplay cue or an effect component at the well, and the splash sound moves into it with it: the cue plays both, and the rows' `Sound` is cleared (a data change).
  - **A sound with a place in the world** (a tile watered, a crop harvested, the sprinkler across the field) comes through a gameplay cue ([section 9](09-AbilitySystem.md)): the ability executes a `GameplayCue.*` tag with the location and a cue notify asset in `/Game/Vestige/Abilities` (the folder `DefaultGame.ini` names for cue notifies) plays the sound there with attenuation. Designed with the first one.
  - **Music and ambience** that continue across maps belong to a game-instance subsystem in an `Audio/` layer, made with the first music or ambience; ambience of one place (the wind in a field) is placed in that map.
- **Every sound belongs to a sound class under `SCL_Overall`,** set on the sound (`Sound Class` in its *Details*): `SCL_Effects` for effects. A new category (`SCL_Music`, `SCL_Ambience`, `SCL_Voice`, `SCL_UI`) is a new child of `SCL_Overall` (or of `SCL_Effects` for UI, as in Lyra), made with its first sound; a volume slider is one category. A sound left without a class falls to `SCL_Overall` through the default sound class, so the master volume still reaches it, but no category slider does; `Vestige.Audio.EverySoundHasAVolumeCategory` fails on it, and on a sound set to `SCL_Overall` itself, which is the root, not a category.
- **A child class is set from its parent:** `ParentClass` isn't editable, so add the child to the parent's `Child Classes` (in the editor or with `set_editor_property("child_classes", ...)` in a script); the parent's `PostEditChangeProperty` sets the child's parent. A mismatch between the two isn't repaired on load, so a script checks the child's `parent_class` after setting it and saves both classes' packages.
- **Volume sliders are control buses on the sound classes,** as Lyra's: each category's sound class gets a volume modulator (a parameter patch reading the master bus and its own) in its *Modulation* settings, the buses are set from the settings ([section 17](17-Settings.md)), and a bus mix is activated by an owner that lives as long as the setting applies. Built with the settings menu, after the maintainer's yes on the Audio Modulation plugin; nothing on a sound changes then.
- **Submixes** (mix buses for effects such as reverb, dynamics or ducking) come when a category needs one; the category's sound class then names it as its `Default Submix`, so no sound changes. Until then everything plays into the engine's master submix.
- **Sound waves for one-shots, MetaSounds for anything with variation or logic** (a random pick of several takes, random pitch, layers, a loop with a start and an end). No Sound Cues: MetaSounds replace them in UE5. A MetaSound source takes the same class as a wave.
- **A sound that plays the moment something happens is in memory before it does.** Sounds named in a presentation asset that is loaded with the HUD (`DA_FeedbackResponses`) are hard references, like the asset itself: a report's sound has to play on the frame it is reported, a few short effects cost little memory, and an asynchronous load on the first report would make the first splash late or silent. Such a wave sets *Loading Behavior Override* to *Retain On Load*, so its first audio chunk (which holds all of a sound under about a second) stays in the stream cache while the wave is loaded. Sounds in content data (a crop's, a tool's) are soft references in an `Audio` bundle, loaded with their content ([section 2](02-Content.md)).
- **A feedback sound never loops.** `PlaySound2D` returns no handle, so a looping sound would never stop; `UVestigeFeedbackResponses::IsDataValid` refuses a row whose sound loops.
- **A feedback sound is a game sound, not a UI sound:** the HUD plays it with `bIsUISound` false, so it pauses with the game. Menu sounds (Common UI's button sounds, later) are UI sounds.
- **Frequent sounds get concurrency** (footsteps, tool hits, many sprinklers): a shared `SCON_` asset in `Content/Vestige/Audio/Mix/` with a limit and a resolution rule (stop the oldest or the farthest), set on the sound; the project default concurrency comes with the first. A sound played at most once a press needs none.
- **Every world sound has attenuation** from a shared `ATT_` preset, made with the first world sound; 2D sounds (feedback, music) have none. Where the listener is (the camera, which is some metres from the farmer, or the farmer) is decided with the first world sound.
- **Our sounds are made or recorded for the game, or come from a library whose license [CONTRIBUTING.md](../CONTRIBUTING.md) accepts** (CC0 where there is a choice, no NC), each recorded in [LICENSING.md](../LICENSING.md) and [CREDITS.md](../CREDITS.md). Generated sounds come from a script committed with its output, as the meshes do: **the script is the source.** AI audio generators need the maintainer's yes.

**Making a sound with a script:**

- A script per sound, `Art/Audio/make_<sound>.py`, in plain Python 3 with the standard library only (`wave`, `struct`, `math`, `random`), so it runs on any machine without installing anything; shared helpers move to `Art/Audio/vestige_audio/` when the second script needs them. It writes `Art/Export/S_<Name>.wav` (Git LFS), next to the meshes the import reads.
- The output is the same on every run: the random generator has a fixed seed in the script, so a rerun that changes nothing gives the same file and a diff of the WAV means the script changed.
- Format: 48 kHz (the project's `AudioSampleRate`), 16-bit PCM, mono for a one-shot (a 2D sound plays in both ears; a mono source is what a world sound needs anyway).
- The script checks its sound before writing it and stops with an error and a failing exit status otherwise: the length within the range it states, the peak at most -3 dBFS, no DC offset, and a sound that starts and ends at silence (a short fade), so it never clicks. `--check-only` runs the check without writing.
- `S_Water_FillCan`, about 0.7 s: a short slosh of band-passed noise, a run of small bubbles (damped sine waves whose pitch rises as each one dies away, the way an air bubble in water rings), and a faint ring of the tin can, then a fade. If it sounds synthetic in the play-test, the fallback is a recording of a can filled under a tap or a CC0 library sound, through the same import.
- **Import:** `EditorScripts/SetUpAudio.py` runs headless like the other set-up scripts (the command is in its docstring; the editor closed, `VestigeEditor` built): it makes `SCL_Overall` and `SCL_Effects` if missing and makes Effects its child, imports or reimports `Art/Export/S_Water_FillCan.wav` to `/Game/Vestige/Audio/Water/S_Water_FillCan` with an automated `AssetImportTask` and checks that a `SoundWave` came out (in 5.8 the WAV goes through Interchange's WAV translator when its feature flag is on and the legacy sound factory otherwise; either gives a `SoundWave`), sets the wave's sound class to `SCL_Effects` and its loading behaviour to *Retain On Load*, and sets `Sound` on the `Vestige.Feedback.Water.CanFilled` and `Vestige.Feedback.Water.WellRanDry` rows of `DA_FeedbackResponses`, leaving their texts. A rerun writes only what differs. Every line it prints starts with `SetUpAudio:`.

**Asset names** follow [CodingGuidelines.md, section 14.2](../CodingGuidelines.md#142-content-and-asset-naming) (the audio prefixes approved by @DunderGG, 2026-11-21). Epic's naming page lists no audio types (checked 2026-11-21), so the prefixes are ours, as Lyra names them where it has one; a sound class is `SCL_` because `SC_` is the sound cue. Folders: `Content/Vestige/Audio/Mix/` for classes, submixes, concurrency, attenuation and modulation, and `Content/Vestige/Audio/<Area>/` for the sounds (`Water/`).

**Data and failures:**

- `FVestigeFeedbackResponse` gains `UPROPERTY(EditDefaultsOnly, Category = "Feedback") TObjectPtr<USoundBase> Sound`: the sound played for the report, or none for silence (most rows). `USoundBase`, so a row can name a wave or a MetaSound. Existing rows load with none.
- `AVestigeHUD::ShowFeedback` plays the row's sound, if it has one, with `UGameplayStatics::PlaySound2D(this, Sound, 1, 1, 0, nullptr, nullptr, false)`, whether or not the row has text, so a row can trade its text for a sound with a data change. It logs `VESTIGE_LOG(this, LogVestigeFeedback, Verbose, "Playing {Sound} for {Tag}.", ...)`, so an off-screen run (`-nosound` too) can show the sound was asked for. A report with no row stays a warning and plays nothing.
- No sound for a report is silence, not an error. A missing sound asset (deleted, not cooked) loads as none and is silent; the content tests catch it before a build.
- The splash is set on both rows that pour water into the can, `Water.CanFilled` and `Water.WellRanDry`; the others (`CanFull`, `WellDry`, `CanEmpty`) are silent. "Can filled." keeps its text until the maintainer drops it ([GameDesign.md, section 6](../GameDesign.md#water)), which is clearing the row's text, no build.

**Lifetimes:** the sound classes and the splash load with `DA_FeedbackResponses`, which `BP_HUD` names, so with the map's HUD; the HUD binds to the feedback subsystem at `BeginPlay` (only with a local player and its widget) and unbinds at `EndPlay`. An automation test world has no HUD, so no sound plays in tests. Nothing about audio is saved.

**Packaging:** the splash is cooked because `DA_FeedbackResponses` references it, and the classes because the splash and `SCL_Effects` reference them (`ParentClass` is a saved property). A sound or class named only in config (a default class no sound uses) needs `DirectoriesToAlwaysCook`.

**Performance:** nothing ticks or polls. A sound plays on a report (a press, a morning); `PlaySound2D` makes no `UObject` and does no search. Memory: the splash is under a second of mono audio, tens of kilobytes compressed, kept while the HUD's asset is loaded. Section 10's budget has no audio line yet; one comes with music and ambience, which are streamed.

**Mods:** a mod replaces a sound by changing the row's asset, or adds its own sounds with its reports when response assets are found through the Asset Manager ([section 15](15-EffectsAndFeedback.md), known gaps). A mod's sound class is a child of `SCL_Overall`, so the volume sliders reach it.

**Tests** (`Source/Vestige/Tests/`):

- `Vestige.Audio.TheFilledCanSplashes`: in `BP_GameMode`'s HUD's responses, the `Water.CanFilled` and `Water.WellRanDry` rows name `S_Water_FillCan`, and the other `Water.*` rows no sound.
- `Vestige.Audio.TheSplashIsAShortRetainedOneShot`: `S_Water_FillCan` is a `USoundWave`, mono, not looping, 0.3 to 1.5 s long, with no attenuation and *Retain On Load*, read from the wave's own `LoadingBehavior` property (or `GetLoadingBehavior(false)`): `GetLoadingBehavior(true)` asserts unless the inherited behaviour was cached first.
- `Vestige.Audio.EverySoundHasAVolumeCategory`: every `USoundBase` asset under `/Game/Vestige` (asset registry, subclasses included) has its own `SoundClassObject` set (read the property or its asset registry tag, never `GetSoundClass()`, which returns the default class for none), and that class is a strict descendant of `SCL_Overall` (a category, not the root).
- `Vestige.Audio.TheDefaultSoundClassIsOverall`: `UAudioSettings::DefaultSoundClassName` is `SCL_Overall`.
- `Vestige.UI.TheFeedbackResponsesRefuseALoopingSound` (editor only): `IsDataValid` is invalid for a row whose sound loops and valid for one whose sound doesn't.
- The HUD playing the sound needs a local player, so it is heard by hand (the play-test) and seen in an off-screen run's log (the `Verbose` line).

**Pitfalls:**

- `PlaySound2D` or `SpawnSoundAtLocation` in gameplay code (a tool ability, the well, a component in `Farming/`).
- A looping sound played with `PlaySound2D`: it can't be stopped.
- A sound without a class, or in the engine's `Master` class: no volume slider reaches it. Don't edit the engine's `Master` class; it is engine content.
- A soft reference to a sound that must play at once, loaded on demand: the first play is late or silent.
- A sound with `bIsUISound` that should pause with the game, or a menu sound without it.
- Hundreds of looping ambient sounds placed in a level without concurrency or virtualization settings.
- Generated audio edited by hand after export: the next run of its script overwrites it. Change the script.
