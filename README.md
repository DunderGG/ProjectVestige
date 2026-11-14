<p align="center"><img src="Images/Promotional/Vestige-Social-Preview-CGPT.png" alt="Hob and the sleeper, tending the garden, while the sun rises over the robot cities" width="100%"></p>
<p align="center"><img src="Images/Promotional/Vestige-Tagline.svg" alt="One forgotten farm. One faithful robot. A whole world waiting to grow again." width="100%"></p>

<h1 align="center">Project Vestige</h1>

<h3 align="center">A cozy 3D farming and life-sim game set in a retro-futuristic, post-apocalyptic world. 

You wake from decades of cryosleep under an overgrown homestead. Humans have almost vanished, and robots carry on with the jobs people gave them. 

You bring the farm back to life, help the town next door, and slowly push out into the wasteland.</h3>

## 📥 Download

Project Vestige is in early-development. Release builds and playable demos are coming for Windows.

## 🚀 Getting started

You need Windows 10 or 11, Unreal Engine 5.8, Git with **Git LFS**, and Visual Studio 2026 with the components in `.vsconfig` (the editor compiles the project's C++ the first time, even if you won't write code).

```powershell
git lfs install
git clone https://github.com/DunderGG/ProjectVestige.git
```

Then double-click `Vestige.uproject`, let Unreal build the C++ modules, open `Content/Maps/L_Farm` in the editor and press **Play**. The **[the setup guide](Docs/Setup.md)** walks through every step on a fresh PC, with troubleshooting; the controls are in [CONTRIBUTING.md](CONTRIBUTING.md#controls).

## 📚 Documentation

| Document | What's in it |
|---|---|
| [Documentation index](Docs/README.md) | Index of the project's guides, plans, reviews, and tests |
| [Game design](Docs/GameDesign.md) | The premise, intended experience, and planned gameplay |
| [Roadmap](Docs/Roadmap.md) | What is implemented and what development plans next |
| [Setup guide](Docs/Setup.md) | Complete Windows setup, build, test, and troubleshooting instructions |
| [Contributing guide](CONTRIBUTING.md) | Ways to help, repository workflow, and requirements for contributions |
| [Architecture](Docs/Architecture.md) | How the code is organized and which classes own each responsibility |
| [Decision log](Docs/DecisionLog.md) | Why important technical and design decisions were made |
| [Systems guide](Docs/Systems.md) | Technical guidance for the game's systems |
| [Coding guidelines](Docs/CodingGuidelines.md) | C++ conventions, asset naming, testing, and review checklist |
| [Play-test guide](Docs/PlayTest.md) | Manual checks and the project's in-editor smoke test |
| [Licensing](LICENSING.md) | Which licenses apply to code, assets, and other project material |
| [Security policy](SECURITY.md) | How to report security problems privately |
| [Lore](Docs/Spoilers/Lore.md) | Story and world details; **contains spoilers** |

## 🧱 Built With

- [Unreal Engine 5.8](https://www.unrealengine.com/): cross-platform game engine
- [Blender](https://www.blender.org/): 3D modeling, animation and rendering

## 🤝 Contributing

Contributions are welcome: gameplay code, 3D art, audio, testing, documentation and mods. Start with [CONTRIBUTING.md](CONTRIBUTING.md), and open an issue to say what you'd like to work on.

## 👤 Author

**David Bennehag** - [@DunderGG](https://github.com/DunderGG) - [dunder.gg](https://dunder.gg)

## 📄 License
> Still under investigation. Any lawyers in chat? Hit me up!

The repository does not use a single license for everything: project code is licensed under **GPL-3.0-or-later** with an additional permission for Unreal Engine and platform SDKs; most project assets and documentation use **CC BY-NC-SA 4.0**; Epic Games template material remains under the Unreal Engine EULA. See [LICENSING.md](LICENSING.md) for the full details and the files that each license covers.

[LICENSING.md](LICENSING.md) explains what this means in practice. Contributors are listed in [CREDITS.md](CREDITS.md).
