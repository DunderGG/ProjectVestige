# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Makes the crack mask of contaminated soil (Docs/Systems/27-Contamination.md, "How the soil shows
contamination"): Art/Export/T_ContaminationCracks.png, read by the material function
MF_ContaminationCracks that M_FarmSoil and M_FarmPlaceholder use.

The texture is a mask, not a colour. Import it with sRGB off, compression BC7 (TC_BC7), Texture Group
World, address Wrap and mips on. Not Masks (BC1): a 4 x 4 block holding (0,0), (0,1) and (1,1) can't be
stored exactly, and heavy-only cracks would leak into the light level as faint ghosts.
    R  the light level's cracks: a broken network of short cracks and a few specks
    G  the heavy level's cracks: R plus the rest of the network, its main cracks about 1.7 times as
       wide (so heavy still reads as bolder at the far rows, after the fine cracks mip away), finer
       cracks between them and more specks; a superset of R, so heavy only ever adds to light
    B  unused (0)
The material reads the tile's contamination c (custom data 4, 0 to 1) and draws
    mask = lerp(R, G, saturate(2c - 1)) * saturate(2c)
so c = 0 shows nothing, c = 0.5 (light, at a cap of 2) exactly R and c = 1 (heavy) exactly G.
The levels differ in shape and density, not only in hue, for colour-blind players. The cracks are a
change of base colour only, with no emissive: nights stay properly dark, and the HUD names the level
of the faced tile.

It tiles seamlessly and covers TEXTURE_SPAN_M metres of ground: the material samples it by world
position (world XY in cm / (100 * TEXTURE_SPAN_M)), so the cracks run on across neighbouring tiles
and repeat only every few tiles, and the grass placeholder slab and the tilled mesh need no UVs for it.

Run from the project folder:
    blender --background --python-exit-code 1 --python Art/Blender/make_contamination_cracks.py
    blender --background --python-exit-code 1 --python Art/Blender/make_contamination_cracks.py -- --preview-dir DIR
--preview-dir renders soil and grass tiles at each level, dry and wet, with a Blender stand-in for the
Unreal material, from the game camera and close, plus a greyscale copy of each (hue removed).
"""

import math
import os
import random
import sys

import bpy  # must come first
import numpy as np
from mathutils import Vector

sys.dont_write_bytecode = True  # no __pycache__ in Art/Blender/
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_farm_assets as farm  # noqa: E402  (the soil tile and the preview scene; nothing runs on import)

SIZE = 1024
TEXTURE_SPAN_M = 4.0  # metres of ground one repeat covers: 0.39 cm a pixel
PATH = os.path.join(farm.EXPORT_DIR, "T_ContaminationCracks.png")

# Widths are half widths in metres. The play camera sees about 160 screen pixels a metre at 4 m
# (90 degree field of view, 1280 wide), so a crack 3 cm wide is about 5 pixels: thinner cracks
# break up into shimmer there.
LIGHT_CELLS, LIGHT_HALF_WIDTH = 6, 0.016   # 6 x 6 cells a repeat: cracks about 0.67 m apart
FINE_CELLS, FINE_HALF_WIDTH = 13, 0.011
HEAVY_WIDTH_SCALE = 1.7  # heavy's main cracks against light's
LIGHT_SPECKS, HEAVY_SPECKS = 300, 240
SPECK_RADIUS = (0.012, 0.024)

# The preview stand-in of the Unreal material.
CUE = "D63A9C"  # the contamination cue, GameDesign.md section 15
GRASS_LINEAR = (0.15, 0.45, 0.10)  # FarmTileLooks::GrassColor
SOIL_DRY, SOIL_WET = "6B4A2B", "3A2817"


def grid():
    t = (np.arange(SIZE, dtype=np.float32) + 0.5) / SIZE
    return np.meshgrid(t, t)  # u along x, v along y, 0 to 1 a repeat


def wrap(d):
    return (d + 0.5) % 1.0 - 0.5


def noise(u, v, rng, octaves=((2, 1.0), (3, 0.7), (5, 0.45), (8, 0.25))):
    """A smooth tileable noise, about -1 to 1: a sum of waves with whole frequencies per repeat."""
    total = np.zeros_like(u)
    weight = 0.0
    for freq, amp in octaves:
        for _ in range(2):
            fx, fy = rng.choice([-1, 1]) * freq, rng.randint(-freq, freq)
            total += amp * np.sin(2 * math.pi * (fx * u + fy * v) + rng.uniform(0, 2 * math.pi))
            weight += amp
    return total / (weight * 0.5)


def voronoi_edge(u, v, cells, rng):
    """Distance (in repeats) to the nearest cell border of a jittered-grid Voronoi on the torus."""
    f1 = np.full_like(u, 9.0)
    f2 = np.full_like(u, 9.0)
    for i in range(cells):
        for j in range(cells):
            px = (i + rng.uniform(0.15, 0.85)) / cells
            py = (j + rng.uniform(0.15, 0.85)) / cells
            d = np.hypot(wrap(u - px), wrap(v - py))
            f2 = np.where(d < f1, f1, np.minimum(f2, d))
            f1 = np.minimum(f1, d)
    return (f2 - f1) * 0.5  # half the F2 - F1 gap: the distance to the border


def line(distance, half_width):
    """1 inside a crack, 0 outside, with a one-pixel soft edge."""
    px = 1.0 / SIZE
    return np.clip((half_width - distance) / px + 0.5, 0.0, 1.0)


def specks(u, v, count, rng):
    out = np.zeros_like(u)
    for _ in range(count):
        cx, cy = rng.random(), rng.random()
        r = rng.uniform(*SPECK_RADIUS) / TEXTURE_SPAN_M
        d = np.hypot(wrap(u - cx), wrap(v - cy) * rng.uniform(0.7, 1.3))
        out = np.maximum(out, line(d, r))
    return out


def make_mask():
    rng = random.Random(27)
    u, v = grid()
    to_repeats = 1.0 / TEXTURE_SPAN_M
    big = voronoi_edge(u, v, LIGHT_CELLS, rng)
    fine = voronoi_edge(u, v, FINE_CELLS, rng)
    breaks = noise(u, v, rng)       # where the light network is broken
    taper = noise(u, v, rng)        # crack width along its length
    fine_keep = noise(u, v, rng)    # where the fine cracks run
    width = 0.55 + 0.45 * np.clip(taper * 0.5 + 0.5, 0, 1)

    # Light: the big network where breaks is high, most of it, and scattered specks.
    light_lines = line(big, LIGHT_HALF_WIDTH * to_repeats * width) * np.clip((breaks + 0.95) * 3.0, 0, 1)
    light = np.maximum(light_lines, specks(u, v, LIGHT_SPECKS, rng))
    # Heavy: the whole big network, much wider, fine cracks over most of the ground, many specks.
    heavy_lines = line(big, LIGHT_HALF_WIDTH * HEAVY_WIDTH_SCALE * to_repeats * width)
    fine_lines = line(fine, FINE_HALF_WIDTH * to_repeats * width) * np.clip((fine_keep + 0.55) * 2.0, 0, 1)
    heavy = np.maximum.reduce([light, heavy_lines, fine_lines, specks(u, v, HEAVY_SPECKS, rng)])
    return light, heavy


MIN_TILE_COVER = {"light": 0.03, "heavy": 0.15}  # every 1 m tile shows at least this share in cracks


def tile_cover_problems(light, heavy):
    """Every 1 m square of ground, at every texel offset (the material samples by world position, so a
    tile can sit anywhere in the repeat), shows enough cracks, so no contaminated tile reads as clean.
    A summed-area table over the mask tiled 2 x 2 gives every square's sum in one lookup. Returns the problems."""
    problems = []
    step = int(SIZE / TEXTURE_SPAN_M)
    for name, mask in (("light", light), ("heavy", heavy)):
        tiled = np.tile(mask.astype(np.float64), (2, 2))
        table = np.zeros((2 * SIZE + 1, 2 * SIZE + 1))
        table[1:, 1:] = tiled.cumsum(0).cumsum(1)
        i = np.arange(SIZE)
        a, b = np.meshgrid(i, i, indexing="ij")
        sums = table[a + step, b + step] - table[a, b + step] - table[a + step, b] + table[a, b]
        lowest = float(sums.min()) / (step * step)
        print(f"{name}: the emptiest 1 m square at any offset is {lowest * 100:.1f} % cracks")
        if lowest < MIN_TILE_COVER[name]:
            problems.append(f"a 1 m square of the {name} mask is only {lowest * 100:.1f} % cracks")
    return problems


def save_png(light, heavy, path):
    pixels = np.zeros((SIZE, SIZE, 4), dtype=np.float32)
    pixels[..., 0], pixels[..., 1], pixels[..., 3] = light, heavy, 1.0
    image = bpy.data.images.new("T_ContaminationCracks", SIZE, SIZE, alpha=False, float_buffer=False)
    image.colorspace_settings.name = "Non-Color"
    image.pixels.foreach_set(pixels.ravel())
    image.filepath_raw = path
    image.file_format = "PNG"
    image.save()
    print(f"Wrote {path}: light covers {light.mean() * 100:.1f} %, heavy {heavy.mean() * 100:.1f} % of the ground")
    return image


# ---------------------------------------------------------------------------
# Preview: a Blender stand-in for M_FarmSoil / M_FarmPlaceholder with MF_ContaminationCracks.

def hex_linear(hex_color):
    return [farm.srgb_to_linear(int(hex_color[i:i + 2], 16) / 255.0) for i in (0, 2, 4)]


def preview_material(name, base_linear, image):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.9
    geo = nodes.new("ShaderNodeNewGeometry")
    mapping = nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (1 / TEXTURE_SPAN_M, 1 / TEXTURE_SPAN_M, 1)
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.interpolation = "Linear"
    sep = nodes.new("ShaderNodeSeparateColor")
    attr = nodes.new("ShaderNodeAttribute")
    attr.attribute_type = "OBJECT"
    attr.attribute_name = "contamination"
    links.new(geo.outputs["Position"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], tex.inputs["Vector"])
    links.new(tex.outputs["Color"], sep.inputs["Color"])

    def math_node(op, a, b=None, clamp=False):
        n = nodes.new("ShaderNodeMath")
        n.operation = op
        n.use_clamp = clamp
        for k, x in enumerate((a, b)):
            if x is None:
                continue
            if isinstance(x, float):
                n.inputs[k].default_value = x
            else:
                links.new(x, n.inputs[k])
        return n.outputs[0]

    c = attr.outputs["Fac"]
    blend = math_node("SUBTRACT", math_node("MULTIPLY", c, 2.0), 1.0, clamp=True)  # saturate(2c - 1)
    show = math_node("MULTIPLY", c, 2.0, clamp=True)                               # saturate(2c)
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "FLOAT"
    links.new(blend, mix.inputs["Factor"])
    links.new(sep.outputs["Red"], mix.inputs["A"])
    links.new(sep.outputs["Green"], mix.inputs["B"])
    mask = math_node("MULTIPLY", mix.outputs["Result"], show)
    color = nodes.new("ShaderNodeMix")
    color.data_type = "RGBA"
    links.new(mask, color.inputs["Factor"])
    color.inputs["A"].default_value = (*base_linear, 1)
    color.inputs["B"].default_value = (*hex_linear(CUE), 1)
    links.new(color.outputs["Result"], bsdf.inputs["Base Color"])
    return mat


def grass_slab(collection):
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    obj = bpy.context.active_object
    obj.scale = (1.0, 1.0, 0.04)
    obj.location.z = 0.02
    for c in obj.users_collection:
        c.objects.unlink(obj)
    collection.objects.link(obj)
    obj.name = "Grass"
    return obj


def render_previews(image, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    coll = farm.new_collection("Preview")
    soil = farm.build_soil(coll, farm.make_material("M_Soil", "6B4A2B", 0.95))
    grass = grass_slab(coll)
    mats = {
        "Dry": preview_material("PV_SoilDry", hex_linear(SOIL_DRY), image),
        "Wet": preview_material("PV_SoilWet", hex_linear(SOIL_WET), image),
        "Grass": preview_material("PV_Grass", GRASS_LINEAR, image),
    }
    scene, cam = farm.setup_preview_scene()
    soil.hide_render = grass.hide_render = True
    extra = []

    def place(kind, level, x, y):
        source = grass if kind == "Grass" else soil
        obj = source.copy()
        obj.data = source.data.copy()
        obj.data.materials.clear()
        obj.data.materials.append(mats[kind])
        obj["contamination"] = level
        obj.location = (x, y, obj.location.z if kind == "Grass" else 0.0)
        obj.hide_render = False
        scene.collection.objects.link(obj)
        extra.append(obj)

    # Levels along Y (clean, light, heavy from left to right in the game camera), rows dry, wet, grass.
    for row, kind in enumerate(("Dry", "Wet", "Grass")):
        for col, level in enumerate((0.0, 0.5, 1.0)):
            place(kind, level, 1.1 - 1.1 * row, 1.1 * (col - 1))
    farm.game_camera(cam, Vector((0.0, 0.0, 0.05)))
    shots = [("Cracks_Levels_Game", 1280, 720)]
    farm.render(scene, os.path.join(out_dir, shots[0][0] + ".png"), 1280, 720)
    cam.data.sensor_fit = "AUTO"
    cam.data.lens = 35
    farm.aim(cam, Vector((3.0, -2.4, 2.6)), Vector((0.0, 0.0, 0.0)))
    farm.render(scene, os.path.join(out_dir, "Cracks_Levels_Close.png"), 1280, 640)
    shots.append(("Cracks_Levels_Close", 1280, 640))
    farm.remove_all(extra)

    # The field as L_Farm starts it: 8 x 8, two clean rows near the camera, four light, two heavy far
    # away; the left half watered.
    for i in range(8):
        level = 0.0 if i < 2 else (0.5 if i < 6 else 1.0)
        for j in range(8):
            place("Wet" if j < 4 else "Dry", level, 2.5 - i, j - 3.5)
    for j in range(8):
        place("Grass", 0.5, -5.5, j - 3.5)
    farm.game_camera(cam, Vector((0.0, 0.0, 0.05)))
    farm.render(scene, os.path.join(out_dir, "Cracks_Field_Game.png"), 1280, 720)
    shots.append(("Cracks_Field_Game", 1280, 720))
    farm.remove_all(extra)

    # The same pictures with the hue removed, to judge the levels by density alone.
    for name, _, _ in shots:
        img = bpy.data.images.load(os.path.join(out_dir, name + ".png"))
        px = np.array(img.pixels[:], dtype=np.float32).reshape(-1, 4)
        grey = px[:, 0] * 0.2126 + px[:, 1] * 0.7152 + px[:, 2] * 0.0722
        px[:, 0] = px[:, 1] = px[:, 2] = grey
        img.pixels.foreach_set(px.ravel())
        img.filepath_raw = os.path.join(out_dir, name + "_Grey.png")
        img.save()


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    bpy.ops.wm.read_factory_settings(use_empty=True)
    light_mask, heavy_mask = make_mask()
    found = tile_cover_problems(light_mask, heavy_mask)
    if found:
        for problem in found:
            print("ERROR: " + problem)
        sys.exit(1)
    os.makedirs(farm.EXPORT_DIR, exist_ok=True)
    texture = save_png(light_mask, heavy_mask, PATH)
    if "--preview-dir" in args:
        render_previews(texture, os.path.abspath(args[args.index("--preview-dir") + 1]))
