# Licensing

Project Vestige's code and its assets are under different licenses, and they allow different things:

- **The code is free software** under the GNU GPL, version 3 or later, with an extra permission for linking with Unreal Engine. Anyone can use, change, share and even sell it, as long as they pass on the same GPL rights and the source code.
- **The assets are public but not free for commercial use.** Art, audio, levels, Blueprints, game data and documentation are under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0). Anyone can use and change them for non-commercial purposes. Commercial use of the assets is reserved to the maintainer, David Bennehag, on purpose.
- **The maintainer sells official builds** of the game on online stores, for players who don't want to build it themselves. Their Project Vestige code stays under the GPL, and its source is published for every build.

So the repository as a whole is not "open source" in the strict sense: the code is, the assets are open for non-commercial use only.

| What | License | File |
|---|---|---|
| **Code**: see [Which license applies where](#which-license-applies-where) | GNU General Public License v3.0 or later (`GPL-3.0-or-later`), with the [additional permission](#additional-permission-for-unreal-engine-gplv3-section-7) for Unreal Engine and platform SDKs | [LICENSE](LICENSE) and [LICENSE-EXCEPTION.txt](LICENSE-EXCEPTION.txt) |
| **Assets and everything else**: art, models, textures, materials, Blueprints, levels, audio, music, game data and documentation | Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (`CC-BY-NC-SA-4.0`) | [LICENSE-ASSETS.txt](LICENSE-ASSETS.txt) |
| **Epic Games material**: see [Not covered](#not-covered-epic-games-material) | Unreal Engine EULA | https://www.unrealengine.com/eula |

## What this means in practice

**The code**

- You can read, change, share and build it, for any purpose.
- If you distribute a program built from it, including a paid one, you must give its users the same GPL rights and the source code of your version. Nobody can ship a closed-source game made from Project Vestige's code.
- The GPL does not stop anyone from selling copies of the code. It only makes sure the code stays free. What stops a fork from selling the *game* is the asset license below.
- The GPL doesn't stop anyone from writing their own, similar game without copying our code or assets.

**The assets**

- You can use them for mods, fan projects, videos and anything else non-commercial, as long as you credit Project Vestige and share your changes to the assets under the same license.
- You can't sell the assets, or a game or other product that contains them, and you can't use them in any other commercial way. That also applies to forks of Project Vestige: a fork can sell its GPL code, but not with our assets in it.
- Only the maintainer may use the assets commercially. He can do this because he made them, or because contributors gave him permission (see [Contributions](#contributions)).

**Official builds**

- The builds the maintainer sells are Project Vestige's code (under the GPL, with the additional permission) and Project Vestige's assets (which he may sell under the contributors' grant).
- Every official build is made from a tagged commit in this repository, and the store page and in-game credits say where to find it. That tagged source is the Corresponding Source the GPL requires for the build's code. Engine and platform SDK code is not included, as the additional permission allows.
- Buying a build gives you the GPL rights to its code. It doesn't give you commercial rights to the assets.
- Buying a build is not the only legal way to play: anyone can build the game from this repository for their own use.

**Credit**

- Contributors are credited in [CREDITS.md](CREDITS.md) and in the game's credits.

## Additional permission for Unreal Engine (GPLv3 section 7)

Project Vestige is built on Unreal Engine, which is not free software. Without an additional permission, the GPL would not allow anyone, including the maintainer, to distribute a build of the game, because a build links GPL code with the engine.

**This permission is part of the license terms for the code.** It is in [LICENSE-EXCEPTION.txt](LICENSE-EXCEPTION.txt), next to the GPL text in [LICENSE](LICENSE), so it travels with every copy of the source. Keep both files when you share the code. The permission reads:

> **Additional permission under GNU GPL version 3 section 7**
>
> If you modify this Program, or any covered work, by linking or combining it with Unreal Engine (or a modified version of it), or with the libraries and SDKs of a game distribution platform or console that are needed to publish the game there (together, the "Non-GPL Components"), containing parts covered by the terms of their own licenses, the licensors of this Program grant you additional permission to convey the resulting work. Corresponding Source for a non-source form of such a combination shall not include the source code for the parts of the Non-GPL Components used, but shall include the source code for the parts of this Program that are used.
>
> This permission applies whichever version of the GNU GPL (version 3 or any later version) you choose to apply to this Program.

This permission covers only the engine and the platform components. Project Vestige's own code stays under the GPL, and its source must be shared. It doesn't settle what each store or console's own terms require; those are checked separately for each platform before release there.

The copyright holders who grant it are the maintainer and every contributor of code, through the [contributor terms](CONTRIBUTING.md#11-licensing-and-credit).

## License notices in files

Each source file we wrote starts with a copyright line and an [SPDX](https://spdx.dev/) license identifier:

```cpp
// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception
```

`AdditionRef-Vestige-Unreal-Exception` names the additional permission in [LICENSE-EXCEPTION.txt](LICENSE-EXCEPTION.txt). Scripts that never link with Unreal Engine (for example `Art/Blender/*.py`) use plain `GPL-3.0-or-later`. Epic template files keep Epic's copyright line instead. Binary assets can't carry a notice, so the table below says which license applies to them.

## Which license applies where

This table is the licensing record for every part of the repository. Update it in the same commit that adds a new folder, a new kind of file or any third-party material. Third-party files are also listed, with their source and license, in [CREDITS.md](CREDITS.md#third-party).

| Path | Copyright holder | License | Origin | In sold builds |
|---|---|---|---|---|
| `Farm*.h/.cpp` and `Vestige*.h/.cpp` in the folders under `Source/Vestige/`, except the template files below (our classes and tests, with the SPDX line) | David Bennehag and contributors | GPL-3.0-or-later with the additional permission | Written for Project Vestige | Yes, compiled |
| `Source/Vestige.Target.cs`, `Source/VestigeEditor.Target.cs`, `Source/Vestige/Vestige.Build.cs`, `Source/Vestige/Core/Vestige.h/.cpp`, and `VestigeCharacter.h/.cpp`, `VestigeGameMode.h/.cpp`, `VestigePlayerController.h/.cpp` in `Source/Vestige/Template/` | Epic Games, plus our changes | Unreal Engine EULA; our changes GPL-3.0-or-later with the additional permission | Third Person template, edited by us (mixed) | Yes, compiled |
| `Vestige.uproject`, `Config/*.ini` | Epic Games and David Bennehag | Our settings GPL-3.0-or-later with the additional permission; template defaults Unreal Engine EULA | Generated by the template, edited by us (mixed) | Yes |
| `Art/Blender/*.py`, `.clang-format`, `.gitattributes`, `.gitignore` | David Bennehag and contributors | GPL-3.0-or-later | Written for Project Vestige | No |
| `Content/Vestige/` (except `BP_PlayerCharacter`, below) | David Bennehag and contributors | CC BY-NC-SA 4.0 | Made for Project Vestige | Yes |
| `Content/Vestige/Blueprints/BP_PlayerCharacter.uasset` | Epic Games, plus our changes | Unreal Engine EULA; our changes CC BY-NC-SA 4.0 | The Third Person template's `BP_ThirdPersonCharacter`, renamed, reparented to `AVestigePlayerCharacter` and edited by us (mixed) | Yes |
| `Content/Maps/`, `Content/L_Farm.umap` | David Bennehag and contributors | CC BY-NC-SA 4.0 | Made for Project Vestige; places template content listed below | Yes |
| `Content/Characters/`, `Content/ThirdPerson/`, `Content/Input/` | Epic Games | Unreal Engine EULA | Third Person template | Yes |
| `Art/Blender/*.blend`, `Art/Export/*.fbx` | David Bennehag and contributors | CC BY-NC-SA 4.0 | Made for Project Vestige (generated by `make_farm_assets.py`) | No, sources only |
| `Images/` (except AI-generated files) | David Bennehag | CC BY-NC-SA 4.0 | Made for Project Vestige | No |
| `Images/*-CGPT.png` and any other file whose name ends in `-CGPT` | David Bennehag (prompts) | CC BY-NC-SA 4.0, as far as anyone holds rights in them (see [AI-generated images](#ai-generated-images)) | Generated with ChatGPT (OpenAI) from David's prompts | No |
| `Docs/`, the Markdown files in the root, `.github/` | David Bennehag and contributors | CC BY-NC-SA 4.0 | Written for Project Vestige | No |
| `LICENSE`, `LICENSE-ASSETS.txt` | Free Software Foundation; Creative Commons | Verbatim license texts, under their own terms | Copied unchanged | Yes, with the build |
| `LICENSE-EXCEPTION.txt` | David Bennehag | Part of the code's license terms | Written for Project Vestige | Yes, with the build |

Build output (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`) isn't committed. It carries the licenses of the files it was made from.

## AI-generated images

Some images, such as concept art, posters and social previews, are generated with ChatGPT (OpenAI) from the maintainer's prompts. Their file names end in `-CGPT` before the extension, so the table above covers them by that pattern. Images made with another tool get a suffix naming that tool and their own row in the table.

OpenAI's terms give the user the rights in the output, including for commercial use. In many countries, though, purely AI-generated images may not be protected by copyright at all. The CC BY-NC-SA 4.0 license applies to them only as far as anyone holds rights in them, and others may be able to reuse them regardless of it.

These images are used for concept art and promotion. Whether any of them is used as in-game art is decided separately. Prompts never ask for imitations of someone else's protected work, such as a named artist's style or another game's characters or logos.

## Not covered: Epic Games material

The files and folders marked "Unreal Engine EULA" above come from Epic Games' Third Person template. They are used under the [Unreal Engine EULA](https://www.unrealengine.com/eula), not under our licenses. Source files from the template carry the line `Copyright Epic Games, Inc. All Rights Reserved.`

Changes we make to these files are ours, under the GPL with the additional permission, but the original Epic material stays under Epic's terms. The plan is to replace template code and content with our own over time; see [Docs/Architecture.md](Docs/Architecture.md).

## Contributions

Every contribution comes with two separate grants. The full, binding wording is in the [contributor terms](CONTRIBUTING.md#11-licensing-and-credit):

1. **To everyone:** the contribution is licensed under the license for its part of the repository, GPL-3.0-or-later with the additional permission for code and CC BY-NC-SA 4.0 for everything else.
2. **To the maintainer only:** permission to use the contribution in official Project Vestige builds, including selling them on stores and consoles. The Project Vestige code in those builds stays under the GPL, and its source is published. The maintainer can't use this grant to make a closed-source version.

Contributions are accepted only as pull requests in which the contributor ticks the contributor terms box, so every contribution has a recorded agreement. Ideas and suggestions in issues are welcome, but they aren't contributions of code or assets; if one turns into code or art, it comes in as a pull request.

Third-party material (code or assets someone else made) is accepted only under the licenses listed in [CONTRIBUTING.md](CONTRIBUTING.md#third-party-material), with its source and license recorded.

## Still to check

This setup has not yet been reviewed by a lawyer. Before Project Vestige is sold, released on a console or opened to outside contributions, a lawyer experienced with the GPL and Unreal Engine should check:

- the wording and scope of the additional permission, and how it works with later GPL versions;
- whether it fits the Unreal Engine EULA and each target store and console's SDK terms;
- the contributor terms and the way agreement is recorded;
- the AI-generated images: who owns them, whether they are protected by copyright, and what each store requires to be disclosed about AI-generated content;
- the maintainer's commercial use of contributed assets under CC BY-NC-SA.

## Questions

The binding terms are [LICENSE](LICENSE) together with [LICENSE-EXCEPTION.txt](LICENSE-EXCEPTION.txt) for the code, [LICENSE-ASSETS.txt](LICENSE-ASSETS.txt) for everything else, and the [contributor terms](CONTRIBUTING.md#11-licensing-and-credit) for contributions. The rest of this page is a plain-language summary, not legal advice. If something is unclear, open an issue.
