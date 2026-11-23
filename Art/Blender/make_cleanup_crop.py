# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds the cleaning crop (Base.CleanupCrop, Docs/Systems/27-Contamination.md) and exports its meshes
for Unreal: three growth stages and the ripe mesh. It grows only on contaminated soil and draws the
contamination up into its seed spike, so it is the one crop that stands tall: a low, felted, silver
rosette for its first days, a bolting bud on its last stage, and when ripe a tall stalk whose pods
show the contamination cue (magenta), the sign that it is full and ready to be harvested and contained.

It reuses the plant helpers of make_farm_assets.py (leaves, stems, lumps, the preview scene) without
running that script, so a run writes only this crop's four FBX files:
    Art/Export/SM_Crop_CleanupCrop_Stage0.fbx, _Stage1, _Stage2, _Ripe

Run from the project folder:
    blender --background --python-exit-code 1 --python Art/Blender/make_cleanup_crop.py
    blender --background --python-exit-code 1 --python Art/Blender/make_cleanup_crop.py -- --preview-dir DIR
Options after the double dash: --preview-dir DIR renders the review previews into DIR; --no-export
builds and checks without writing the FBX files.

The stage start days are the crop data's (DA_Crop_CleanupCrop): 0, 1 and 3, ripe after 5 watered nights.
Each stage is at least 1.25 times the one before in height and spread (Docs/Systems/22-FarmFields.md,
the art rule); the script stops with an error if one isn't, or a look is wider than 0.6 m or the
seedling narrower than 0.2 m.

Materials (one slot per part, named like the Unreal instance of M_Crop each slot becomes):
    MI_Crop_CleanupCropLeaf  felted silver-green leaves
    MI_Crop_CleanupCropStalk the stalk
    MI_Crop_CleanupCropBud   the pale closed bud of the last growth stage, so it reads as bolting
    MI_Crop_CleanupCropPod   the ripe pods, in the contamination cue
"""

import math
import os
import random
import sys

import bpy  # must come first: bmesh and mathutils are provided by bpy
import bmesh
from mathutils import Matrix, Vector

sys.dont_write_bytecode = True  # no __pycache__ in Art/Blender/
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_farm_assets as farm  # noqa: E402  (the shared plant helpers; nothing runs on import)

CROP = "CleanupCrop"
STAGE_STARTS, RIPE_NIGHT = [0, 1, 3], 5
MIN_SEEDLING_SPREAD, MAX_SPREAD = 0.2, 0.6
STAGE_GROWTH_MIN = 1.25

# sRGB hex and roughness. The leaf is a pale, felted silver-green, lighter and greyer than the
# cabbage's blue-green (6E9A80) and far from the potato sprout's yellow-green, so the crop reads as
# a plant of its own on brown soil. The pod is the contamination cue, D63A9C (GameDesign.md, section 15).
COLORS = {
    "Leaf": ("A7B39B", 0.9),
    "Stalk": ("6F6A4C", 0.8),
    # A pale cream, lighter than the leaves and clear of the cue and the water colour.
    "Bud": ("E2DAB0", 0.7),
    "Pod": ("D63A9C", 0.45),
}

# A broad oval leaf with a blunt tip, lying low like a mullein's.
LEAF_STATIONS, LEAF_WIDTHS = [0.0, 0.3, 0.7, 1.0], [0.06, 0.34, 0.30, 0.05]


def materials():
    return {part: farm.make_material(f"MI_Crop_{CROP}{part}", hex_color, rough)
            for part, (hex_color, rough) in COLORS.items()}


def add_rosette(bm, rng, count, length, lean_start, lean_end, material, yaw_offset=0.0, cup=0.25, start=0.006):
    for k in range(count):
        yaw = 2 * math.pi * k / count + yaw_offset + rng.uniform(-0.12, 0.12)
        out = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        farm.add_blade(bm, out * start + Vector((0, 0, 0.004)), farm.yaw_towards(out.x, out.y),
                       length * rng.uniform(0.92, 1.06), lean_start + rng.uniform(-4, 4),
                       lean_end + rng.uniform(-4, 4), LEAF_STATIONS, LEAF_WIDTHS, material, cup=cup)


def add_cone(bm, base, depth, radius, material, segments=5):
    """A closed cone standing on base: a bud or the stalk's tip."""
    res = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=True, segments=segments, radius1=radius,
                                radius2=0.0, depth=depth, matrix=Matrix.Translation(base + Vector((0, 0, depth / 2))))
    for f in {f for v in res["verts"] for f in v.link_faces}:
        f.material_index = material


def build(name, stage, collection, mats, seed):
    """stage 0 to 2 are the growth stages, 3 is ripe."""
    rng = random.Random(seed)
    bm = bmesh.new()
    leaf, stalk, pod, bud = 0, 1, 2, 2  # the third slot is the bud on stage 2 and the pods when ripe
    used = [mats["Leaf"]]
    if stage == 0:
        # Four felted leaves held low, a second pair rising between the first.
        add_rosette(bm, rng, 2, 0.118, 32, 8, leaf, yaw_offset=0.3)
        add_rosette(bm, rng, 2, 0.12, 48, 18, leaf, yaw_offset=0.3 + math.pi / 2)
    elif stage == 1:
        # A flat star of six leaves with two young ones standing in the middle.
        add_rosette(bm, rng, 6, 0.158, 34, 4, leaf, yaw_offset=0.1)
        add_rosette(bm, rng, 2, 0.095, 70, 45, leaf, yaw_offset=1.2, cup=0.1)
    elif stage == 2:
        # A wide rosette, and the crop bolts: a short stalk with a closed bud in the middle.
        add_rosette(bm, rng, 8, 0.205, 30, -2, leaf, yaw_offset=0.2)
        add_rosette(bm, rng, 3, 0.11, 72, 50, leaf, yaw_offset=0.9, cup=0.1)
        farm.add_stem(bm, [Vector((0, 0, 0)), Vector((0.003, 0.0, 0.06)), Vector((0.0, 0.004, 0.11))], 0.016, stalk, sides=5)
        # A tall pale bud: a fat cone 9 cm high, plainly bigger than the stalk under it.
        add_cone(bm, Vector((0.0, 0.004, 0.08)), 0.09, 0.03, bud, segments=6)
        used = [mats["Leaf"], mats["Stalk"], mats["Bud"]]
    else:
        # Ripe: the rosette, a tall stalk with a few small leaves, and a spike of swollen pods
        # in the contamination cue from about halfway up.
        add_rosette(bm, rng, 8, 0.2, 26, -6, leaf, yaw_offset=0.2)
        height = 0.52
        points = [Vector((0, 0, 0)), Vector((0.006, -0.004, height * 0.45)), Vector((0.0, 0.006, height))]
        farm.add_stem(bm, points, 0.017, stalk, sides=5)
        for k in range(3):
            p, _ = farm.point_on(points, 0.12 + 0.12 * k)
            yaw = 2.4 * k + 0.5
            out = Vector((math.cos(yaw), math.sin(yaw), 0.0))
            farm.add_blade(bm, p, farm.yaw_towards(out.x, out.y), 0.11 - 0.02 * k, 40, 15,
                           LEAF_STATIONS, LEAF_WIDTHS, leaf, cup=0.2)
        pods = 11
        for k in range(pods):
            t = 0.45 + 0.53 * k / (pods - 1)
            p, _ = farm.point_on(points, t)
            yaw = 2.39996 * k  # the golden angle, so the pods spiral round the spike
            size = 0.037 * (1.0 - 0.45 * (t - 0.45) / 0.53)
            offset = Vector((math.cos(yaw), math.sin(yaw), 0.0)) * (0.012 + size * 0.55)
            farm.add_lump(bm, p + offset, (size, size * 0.85, size * 1.25), yaw, pod, rng,
                          u_segments=5, v_segments=3, jitter=0.08)
        used = [mats["Leaf"], mats["Stalk"], mats["Pod"]]
    return farm.finish_plant(name, bm, used, collection)


def build_all():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    mats = materials()
    crops = []
    for stage in range(4):
        suffix = f"Stage{stage}" if stage < 3 else "Ripe"
        name = f"SM_Crop_{CROP}_{suffix}"
        crops.append(build(name, stage, farm.new_collection(name[3:]), mats, seed=40 + stage))
    return crops


def check(crops):
    """The art rule of Docs/Systems/22-FarmFields.md. Returns the list of problems."""
    problems = []
    bpy.context.view_layer.update()
    previous = None
    for obj in crops:
        dims = obj.dimensions
        spread, height = max(dims.x, dims.y), dims.z
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        line = f"{obj.name}: {tris} tris, {dims.x:.3f} x {dims.y:.3f} x {dims.z:.3f} m"
        ripe = obj.name.endswith("_Ripe")
        if previous and not ripe:
            h, s = height / previous[0], spread / previous[1]
            line += f", x{h:.2f} height, x{s:.2f} spread"
            if min(h, s) < STAGE_GROWTH_MIN:
                problems.append(f"{obj.name} grows less than x{STAGE_GROWTH_MIN} over the stage before")
        print(line)
        if spread > MAX_SPREAD:
            problems.append(f"{obj.name} is {spread:.3f} m wide, over {MAX_SPREAD}")
        if obj.name.endswith("_Stage0") and spread < MIN_SEEDLING_SPREAD:
            problems.append(f"{obj.name} is {spread:.3f} m wide, under {MIN_SEEDLING_SPREAD}")
        if tris > 900:
            problems.append(f"{obj.name} has {tris} triangles, over a crop's few hundred")
        corners = [Vector(c) for c in obj.bound_box]
        if min(c.z for c in corners) < -0.001:
            problems.append(f"{obj.name} reaches below its base")
        used = {p.material_index for p in obj.data.polygons}
        if used != set(range(len(obj.data.materials))):
            problems.append(f"{obj.name} has an unused material slot")
        if not ripe:
            previous = (height, spread)
    return problems


def export(crops):
    import addon_utils
    addon_utils.enable("io_scene_fbx", default_set=True)
    os.makedirs(farm.EXPORT_DIR, exist_ok=True)
    for obj in crops:
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.fbx(
            filepath=os.path.join(farm.EXPORT_DIR, obj.name + ".fbx"),
            use_selection=True,
            object_types={"MESH"},
            apply_scale_options="FBX_SCALE_UNITS",
            axis_forward="X",
            axis_up="Z",
            mesh_smooth_type="FACE",
            use_mesh_modifiers=True,
            add_leaf_bones=False,
            bake_anim=False,
        )
        print(f"Exported {obj.name}.fbx")
    bpy.ops.object.select_all(action="DESELECT")


def render_comparison(crops, out_dir):
    """The cleaning crop's ripe and last stage beside the three spring crops' ripe meshes, on one row
    of tiles from the game camera, so the reviewer sees it stand apart in the field."""
    # Rebuild the spring crops' ripe meshes in this scene without exporting them.
    turnip_mats = [farm.make_material("M_TurnipRoot", "F2EEE6", 0.6), farm.make_material("M_TurnipTop", "7A3B8F", 0.6),
                   farm.make_material("M_TurnipLeaf", "4C9A2A", 0.7)]
    potato_mats = {"Leaf": farm.make_material("M_PotatoLeaf", "3E7B34", 0.7),
                   "Sprout": farm.make_material("M_PotatoSprout", "9FC14A", 0.7),
                   "LeafDry": farm.make_material("M_PotatoLeafDry", "B89A3A", 0.8),
                   "Flower": farm.make_material("M_PotatoFlower", "EDEBE1", 0.6),
                   "Tuber": farm.make_material("M_PotatoTuber", "E0C38F", 0.85)}
    cabbage_mats = {"Leaf": farm.make_material("M_CabbageLeaf", "6E9A80", 0.5),
                    "Head": farm.make_material("M_CabbageHead", "B7D58C", 0.55)}
    coll = farm.new_collection("Comparison")
    others = [
        farm.build_turnip("Cmp_Turnip", coll, turnip_mats, 7, 0.21, (45, 15), root_scale=1.1, seed=13),
        farm.build_potato("Cmp_Potato", coll, potato_mats, stems=6, height=0.14, reach=0.13, leaflets=3,
                          leaflet_size=0.08, seed=23, droop=0.5, dry_share=0.65, tubers=3),
        farm.build_cabbage("Cmp_Cabbage", coll, cabbage_mats, farm.CABBAGE_RIPE, seed=34),
    ]
    scene = bpy.context.scene
    cam = scene.camera
    soil = bpy.data.objects["SM_Soil_Tilled"]
    row = [crops[2], crops[3]] + others
    extra = []
    for obj in crops:
        obj.hide_render = True
    for k, obj in enumerate(row):
        y = k - (len(row) - 1) / 2
        extra.append(farm.tile_copy(soil, (0.0, y, 0.0)))
        obj.location = (0.0, y, farm.RIDGE_TOP)
        obj.hide_render = False
    farm.game_camera(cam, Vector((0.0, 0.0, 0.15)))
    farm.render(scene, os.path.join(out_dir, "Compare_SpringRipe_Game.png"), 1280, 720)
    for obj in row:
        obj.hide_render = True
        obj.location = (0.0, 0.0, 0.0)
    farm.remove_all(extra)


def render_on_cracks(crops, out_dir):
    """Every look on light and heavy soil, dry in front and wet behind, from the game camera: the crop
    only ever stands on contaminated soil. Uses the crack mask and the material stand-in of
    make_contamination_cracks.py (run that first)."""
    import make_contamination_cracks as cracks
    image = bpy.data.images.load(cracks.PATH)
    image.colorspace_settings.name = "Non-Color"
    mats = {"Dry": cracks.preview_material("PV_SoilDry", cracks.hex_linear(cracks.SOIL_DRY), image),
            "Wet": cracks.preview_material("PV_SoilWet", cracks.hex_linear(cracks.SOIL_WET), image)}
    scene = bpy.context.scene
    cam = scene.camera
    soil = bpy.data.objects["SM_Soil_Tilled"]
    extra = []
    for row, (kind, level) in enumerate((("Dry", 0.5), ("Dry", 1.0), ("Wet", 0.5), ("Wet", 1.0))):
        x = 1.5 - row
        for col, crop in enumerate(crops):
            y = col - (len(crops) - 1) / 2
            tile = soil.copy()
            tile.data = soil.data.copy()
            tile.data.materials.clear()
            tile.data.materials.append(mats[kind])
            tile["contamination"] = level
            tile.location = (x, y, 0.0)
            tile.hide_render = False
            scene.collection.objects.link(tile)
            plant = crop.copy()
            plant.location = (x, y, farm.RIDGE_TOP)
            plant.hide_render = False
            scene.collection.objects.link(plant)
            extra += [tile, plant]
    farm.game_camera(cam, Vector((0.0, 0.0, 0.1)))
    farm.render(scene, os.path.join(out_dir, "OnCracks_CleanupCrop_Game.png"), 1280, 720)
    farm.remove_all(extra)


def render(crops, out_dir):
    soil = farm.build_soil(farm.new_collection("Soil"), farm.make_material("M_Soil", "6B4A2B", 0.95))
    farm.DAY_SCHEDULES[CROP] = (STAGE_STARTS, RIPE_NIGHT)
    farm.render_previews([soil] + crops, out_dir)
    render_comparison(crops, out_dir)
    render_on_cracks(crops, out_dir)


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    built = build_all()
    found = check(built)
    if found:
        for problem in found:
            print("ERROR: " + problem)
        sys.exit(1)
    if "--no-export" not in args:
        export(built)
    if "--preview-dir" in args:
        render(built, os.path.abspath(args[args.index("--preview-dir") + 1]))
