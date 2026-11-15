# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Preview renders of one asset, for looking at (the art review judges the asset from them).

The setup is always the same, so renders compare across rounds and across assets:
- the game camera: 4 m from the asset's centre (the length of the player's camera boom), 30
  degrees down, 90 degrees horizontal field of view (Unreal's default). The player's camera
  turns freely, so there are two renders: Game, from the asset's front (+X), and GameSide, from
  its -Y side (in Unreal, where the importer flips Y, that is the +Y side);
- three close views with a 50 mm lens, framed to fit the asset and the tile: front (from +X),
  side (from -Y) and three-quarter (from the front, left and above);
- one more three-quarter view with the UCX_ collision shapes drawn as magenta wire;
- a 1 m x 1 m farm tile (5 x 5 squares of 20 cm) lies beside the asset for scale, on the side that
  is to the right in the view, and a flat ground under everything;
- one sun, fixed, Cycles with 32 samples and the Standard view transform (not AgX, which washes
  out the colours).

Only the asset's own objects are used, and nothing is moved or changed on them: the lights, the
camera, the tile and the ground are added around them. Call this last: the scene is left dirty.
Images are only written to the folder given. cli.parse_args has already made sure it is outside the
repository or in its git-ignored Saved folder (Saved/AgentWork/<task>/ is the place for previews).
"""

import math
import os

import bmesh
import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

from . import materials

GAME_CAMERA_DISTANCE = 4.0
GAME_CAMERA_PITCH = math.radians(30)
GAME_CAMERA_FOV = math.radians(90)
CLOSE_LENS = 50
TILE_SIZE = 1.0
TILE_THICKNESS = 0.02
TILE_GAP = 0.15
RESOLUTION = (1280, 720)
# The game camera is the player's camera boom, whose yaw is free: look from the asset's front and from
# its -Y side. (x, y) is the horizontal direction from the asset to the camera; the boom's 30 degrees
# of pitch are added in render_previews().
GAME_VIEWS = (("Game", (1.0, 0.0)), ("GameSide", (0.0, -1.0)))
CLOSE_VIEWS = (
    ("Front", Vector((1.0, 0.0, 0.12))),
    ("Side", Vector((0.0, -1.0, 0.12))),
    ("ThreeQuarter", Vector((1.0, -1.0, 0.9))),
)


def _setup_scene():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.resolution_percentage = 100
    scene.render.resolution_x, scene.render.resolution_y = RESOLUTION
    scene.view_settings.view_transform = "Standard"

    world = bpy.data.worlds.new("Preview")
    scene.world = world
    if bpy.app.version < (5, 0, 0):
        world.use_nodes = True
    background = world.node_tree.nodes.get("Background") if world.node_tree else None
    if background is None:
        raise RuntimeError(f"the preview world has no 'Background' node (Blender {bpy.app.version_string})")
    background.inputs["Color"].default_value = (0.75, 0.8, 0.85, 1.0)
    background.inputs["Strength"].default_value = 0.8

    sun = bpy.data.objects.new("PreviewSun", bpy.data.lights.new("PreviewSun", "SUN"))
    sun.data.energy = 3.5
    sun.rotation_euler = (math.radians(50), math.radians(10), math.radians(35))
    scene.collection.objects.link(sun)
    camera = bpy.data.objects.new("PreviewCamera", bpy.data.cameras.new("PreviewCamera"))
    camera.data.clip_end = 200.0
    scene.collection.objects.link(camera)
    scene.camera = camera
    return scene, camera


def _flat_object(name, bm):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj, mesh


def _make_ground():
    bm = bmesh.new()
    s = 30.0
    corners = [bm.verts.new((x, y, -0.002)) for x, y in ((-s, -s), (s, -s), (s, s), (-s, s))]
    bm.faces.new(corners)
    obj, mesh = _flat_object("PreviewGround", bm)
    mesh.materials.append(materials.make_material("Preview_Ground", "6F7550", 0.95))
    return obj


def _make_tile():
    """A farm tile: 1 m x 1 m, in 5 x 5 squares of 20 cm in two soil tones, so it works as a ruler."""
    soil_a = materials.make_material("Preview_TileA", "6B4A2B", 0.95)
    soil_b = materials.make_material("Preview_TileB", "7E5A35", 0.95)
    bm = bmesh.new()
    n, step, half = 5, TILE_SIZE / 5, TILE_SIZE / 2
    grid = [[bm.verts.new((-half + i * step, -half + j * step, TILE_THICKNESS)) for i in range(n + 1)]
            for j in range(n + 1)]
    for j in range(n):
        for i in range(n):
            face = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            face.material_index = (i + j) % 2
    body = bmesh.ops.create_cube(bm, size=2.0)["verts"]
    for v in body:  # a slab just under the squares, for the edges
        v.co = Vector((v.co.x * half, v.co.y * half, (v.co.z + 1) * (TILE_THICKNESS - 0.001) / 2))
    obj, mesh = _flat_object("PreviewTile", bm)
    mesh.materials.append(soil_a)
    mesh.materials.append(soil_b)
    return obj


def _world_bounds(obj):
    corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
    hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
    return lo, hi


def _box_corners(lo, hi):
    return [Vector((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]


def _aim(camera, location, target):
    camera.location = location
    camera.rotation_euler = (target - location).to_track_quat("-Z", "Y").to_euler()


def _place_tile(tile, lo, hi, direction):
    """Puts the tile beside the asset, on the side that is to the right as seen from `direction`
    (the camera's offset from the asset), with a gap so it never touches the asset. Returns its bounds."""
    right = Vector((-direction.y, direction.x, 0.0)).normalized()
    cx, cy = (lo.x + hi.x) / 2, (lo.y + hi.y) / 2
    half = TILE_SIZE / 2
    step = 0.0
    while True:
        tx, ty = cx + right.x * step, cy + right.y * step
        apart = (tx - half >= hi.x + TILE_GAP or tx + half <= lo.x - TILE_GAP
                 or ty - half >= hi.y + TILE_GAP or ty + half <= lo.y - TILE_GAP)
        if apart:
            break
        step += 0.02
    tile.location = (tx, ty, 0.0)
    return Vector((tx - half, ty - half, 0.0)), Vector((tx + half, ty + half, TILE_THICKNESS))


def _fit_camera(scene, camera, target, direction, points, margin=0.06):
    """Moves the camera out along `direction` from `target`, from close, until every point is in the
    frame with a margin."""
    distance = 0.5
    for _ in range(400):
        _aim(camera, target + direction.normalized() * distance, target)
        bpy.context.view_layer.update()
        views = [world_to_camera_view(scene, camera, p) for p in points]
        if all(margin <= v.x <= 1 - margin and margin <= v.y <= 1 - margin and v.z > 0 for v in views):
            return distance
        distance *= 1.03
    raise RuntimeError("could not frame the asset in the preview camera")


def _render(scene, path):
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f"[preview] wrote {path}")
    return path


def _show_collision(asset, scene):
    """Magenta wire copies of the UCX_ shapes. Returns the objects, to remove after the render."""
    material = materials.make_material("Preview_Collision", "FF00FF", 0.5)
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.0, 1.0, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 4.0
    copies = []
    for obj in asset.collision:
        copy = obj.copy()
        copy.data = obj.data.copy()
        copy.data.materials.append(material)
        copy.hide_render = False
        copy.display_type = "SOLID"
        modifier = copy.modifiers.new("Wire", "WIREFRAME")
        modifier.thickness = 0.012
        modifier.use_even_offset = True
        scene.collection.objects.link(copy)
        copies.append(copy)
    return copies


def render_previews(asset, out_dir):
    """Renders <Mesh>_Game.png and _GameSide.png (the game camera from the front and from the -Y
    side), _Front.png, _Side.png, _ThreeQuarter.png and, if the asset has collision, _Collision.png
    into out_dir. Returns the paths."""
    os.makedirs(out_dir, exist_ok=True)
    scene, camera = _setup_scene()
    _make_ground()
    tile = _make_tile()
    lo, hi = _world_bounds(asset.main)
    center = (lo + hi) / 2
    name = asset.main.name
    paths = []

    # The game camera, from each side.
    camera.data.sensor_fit = "HORIZONTAL"
    camera.data.angle = GAME_CAMERA_FOV
    for view, (dx, dy) in GAME_VIEWS:
        direction = Vector((dx * math.cos(GAME_CAMERA_PITCH), dy * math.cos(GAME_CAMERA_PITCH),
                            math.sin(GAME_CAMERA_PITCH)))
        _place_tile(tile, lo, hi, direction)
        _aim(camera, center + direction * GAME_CAMERA_DISTANCE, center)
        paths.append(_render(scene, os.path.join(out_dir, f"{name}_{view}.png")))

    # The close views.
    camera.data.lens = CLOSE_LENS
    for view, direction in CLOSE_VIEWS:
        tile_lo, tile_hi = _place_tile(tile, lo, hi, direction)
        union_lo = Vector((min(lo.x, tile_lo.x), min(lo.y, tile_lo.y), 0.0))
        union_hi = Vector((max(hi.x, tile_hi.x), max(hi.y, tile_hi.y), hi.z))
        target = (union_lo + union_hi) / 2
        _fit_camera(scene, camera, target, direction, _box_corners(lo, hi) + _box_corners(tile_lo, tile_hi))
        paths.append(_render(scene, os.path.join(out_dir, f"{name}_{view}.png")))
        if view == "ThreeQuarter" and asset.collision:
            copies = _show_collision(asset, scene)
            paths.append(_render(scene, os.path.join(out_dir, f"{name}_Collision.png")))
            for copy in copies:
                bpy.data.objects.remove(copy)
    return paths
