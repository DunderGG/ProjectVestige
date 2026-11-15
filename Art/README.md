# Art source files

This folder contains the editable source files and export pipeline for the project's 3D art. It is not the folder for images used in documentation or promotion; those belong in [`Images/`](../Images/). Assets imported into Unreal belong under `Content/Vestige/`.

## Folders

- [`Blender/`](Blender/) contains the scripts that build the models, and the `.blend` files they save.
- [`Export/`](Export/) contains the FBX files the scripts export, ready to import into Unreal.

The models are built by Python scripts, not by hand in Blender, so that anyone can rerun and change them. **The script is the source.** The files in `Export/` and the `.blend` files are generated: change the script and run it again, don't edit them. The export settings, scale, pivots and naming are in the [contributing art guide](../CONTRIBUTING.md#5-contributing-art), and how to run a script is in [Docs/Setup.md, section 11](../Docs/Setup.md#11-blender-art-only).

## The scripts

- `Blender/make_farm_assets.py` is the first script. One run builds the soil tile, every crop stage and the watering can, and rewrites all their FBX files and `FarmAssets.blend`.
- `Blender/make_<asset>.py` is one script per asset: `make_bed.py`, `make_shipping_bin.py`, `make_well.py` and `make_sprinkler.py` so far. It builds the asset, checks it, writes only its own `Art/Export/<AssetName>.fbx` and, when asked, its own `.blend` (`--save-blend`). These scripts share the helpers in `Blender/vestige_art/`, and `Blender/check_vestige_art.py` is the test of those helpers and of the check (see "Checking the shared module" below).

## Adding an asset

Copy the script that is closest to your asset (`Blender/make_bed.py` for furniture, `make_shipping_bin.py` for a machine of boxes, `make_well.py` for a round one with pipes), rename it and change `build()`. What the helpers in `Blender/vestige_art/` give you:

- **Primitives:** `add_box` (with chamfered or rounded edges, and a `taper` that leans the sides in or out), `add_prism` (an outline pushed along an axis, for an arch or a gear; leave out every face but one for a flat one-sided disc), `add_cylinder` (also a tapered cylinder), `add_ring` (a round wall with a hole, for a well), `add_tube` (a pipe or lever swept along a few points) and `add_dome`. Parts that touch leave out the face they share, so most of the geometry that nobody could see is gone; some stays (the chamfers at the joints, a face that a neighbour only partly covers): on the bed, about 4 % of the triangles.
- **Materials:** a small shared palette (`vestige_art/palette.py`), so the machines and furniture share a few colours; it has no Blender code in it, so the Unreal editor's Python can read the same colours when it makes the material instances. A Blender material is named like the Unreal material instance its slot becomes, `MI_Prop_<Colour>`, so the import step can match them by name. Colours are flat: no textures. Two colours are cues that nothing else may use: `Water` (azure, in a slot of its own) and, reserved and not in the palette until a mesh shows it, the contamination colour.
- **Finishing:** `finish_object` welds, triangulates (so what you render is what Unreal imports) and sets the shading: `"flat"` for the faceted look, or `"angle"` for smooth-by-angle, where boxes stay crisp and a cylinder of 12 or more sides reads as round. There is no UV map: the colours are flat.
- **Collision:** `add_collision_box` and `add_collision_hull` make the `UCX_<Mesh>_NN` shapes the player bumps into. A prop the player walks through passes `collision=False` to `run()` and has none.
- **The check:** `run()` stops with an error, and a failing exit status, before writing anything if the pivot or size is wrong, the triangle budget for the asset's kind (prop or building; [CONTRIBUTING.md, Budgets](../CONTRIBUTING.md#budgets)) is exceeded without a stated reason, a face is flipped or has no area, an edge is shared by more than two faces, a material slot is unused, or the collision shapes are named or shaped wrongly or the highest one is under 0.46 m (the farmer steps up 0.45 m, so a lower solid would be climbed, not bumped into).
- **Left and right:** the Unreal importer flips Y (a Blender y of -0.2245 m is an Unreal y of +22.45 cm; X and Z stay). A side that matters in the game, such as a bed's pillow or a machine's lever, goes on the side Unreal shows as the one you want, so build it on the opposite Blender side. The `_GameSide` preview is rendered from Blender's -Y for that reason.
- **Previews:** `--preview-dir` renders the game camera view from the asset's front and from its side (4 m away, 30 degrees down, 90 degrees field of view; the player's camera turns freely) and three close views, with a 1 m farm tile beside the asset for scale. Write them outside the repository or into `Saved/AgentWork/<task>/`, which Git ignores. Look at them: readability from the game camera matters more than detail.
- **Not built yet:** cones, UV maps, `SOCKET_` empties (the grip of a held tool) and the crop and tool kinds of the check (their pivot and collision rules). `make_farm_assets.py` still makes the crops and the watering can. Add what is missing with the first script that needs it, then run that script with `--check-only` and look at its previews.

Importing an exported FBX into `Content/Vestige/` needs the Unreal editor and is a separate step.

## Checking the shared module

The check above is the only thing between a broken mesh and the exported FBX, so it has its own test: `Blender/check_vestige_art.py` builds a small good asset with one defect at a time (and some good ones) and confirms that each defect is refused and each good one passes. From the project folder, with plain Python 3 (it starts Blender itself: `blender` on the PATH, the `BLENDER` environment variable, or the default install path):

```
python Art/Blender/check_vestige_art.py
```

It starts Blender once per case and takes one to two minutes, so run it by hand, not on every commit: before and after you change anything in `Blender/vestige_art/`, and before approving such a change. It ends with `N of N passed` and exit status 0. Name cases after the command to run only those (`... check_vestige_art.py bad_defect_non_manifold`); a new check in `vestige_art/validate.py` gets a case in the file.
