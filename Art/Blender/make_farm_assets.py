# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds the step 3 farm assets procedurally and exports them for Unreal.

Run with Blender (any 4.x/5.x):
    blender --background --python Art/Blender/make_farm_assets.py
or with the bpy module:
    python Art/Blender/make_farm_assets.py [--preview-dir DIR]

Writes Art/Blender/FarmAssets.blend and one FBX per asset in Art/Export.
All sizes are in meters. Z = 0 is the ground (soil) or, for the watering can, the grip.
"""

import math
import os
import random
import sys

import bpy  # must come first: bmesh and mathutils are provided by bpy
import bmesh
from mathutils import Matrix, Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BLEND_PATH = os.path.join(ROOT, "Art", "Blender", "FarmAssets.blend")
EXPORT_DIR = os.path.join(ROOT, "Art", "Export")


# ---------------------------------------------------------------------------
# Helpers

def srgb_to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def make_material(name, hex_color, roughness=0.8):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    rgb = [srgb_to_linear(int(hex_color[i:i + 2], 16) / 255.0) for i in (0, 2, 4)]
    mat.diffuse_color = (*rgb, 1.0)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*rgb, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    return mat


def add_box_uvs(bm, scale=1.0):
    """Simple box projection so Unreal gets a UV channel to build lightmaps from."""
    uv_layer = bm.loops.layers.uv.verify()
    for face in bm.faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        for loop in face.loops:
            co = loop.vert.co
            if axis == 0:
                loop[uv_layer].uv = (co.y * scale, co.z * scale)
            elif axis == 1:
                loop[uv_layer].uv = (co.x * scale, co.z * scale)
            else:
                loop[uv_layer].uv = (co.x * scale, co.y * scale)


def finish_object(name, bm, materials, collection):
    bm.normal_update()
    add_box_uvs(bm)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for mat in materials:
        mesh.materials.append(mat)
    for poly in mesh.polygons:
        poly.use_smooth = False
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    return obj


def new_collection(name):
    coll = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(coll)
    return coll


def transform_verts(bm, verts, matrix):
    bmesh.ops.transform(bm, matrix=matrix, verts=list(verts))


# ---------------------------------------------------------------------------
# Tilled soil: 1 x 1 m, ridges 6 cm high, furrows 3.5 cm, ridge on the center line
# so crops planted in the middle of the tile sit on top of a ridge.

def build_soil(collection, mat_soil):
    rng = random.Random(7)
    bm = bmesh.new()
    size, steps_x, steps_y = 1.0, 40, 8
    ridge_top, furrow_bottom, ridge_spacing = 0.06, 0.035, 0.2

    def height(x):
        mid = (ridge_top + furrow_bottom) / 2
        amp = (ridge_top - furrow_bottom) / 2
        return mid + amp * math.cos(2 * math.pi * x / ridge_spacing)

    xs = [-size / 2 + size * i / steps_x for i in range(steps_x + 1)]
    ys = [-size / 2 + size * j / steps_y for j in range(steps_y + 1)]

    # Top surface with a little lumpiness, kept flat at the tile edges so tiles join seamlessly.
    top = []
    for j, y in enumerate(ys):
        row = []
        for i, x in enumerate(xs):
            edge = i in (0, steps_x) or j in (0, steps_y)
            jitter = 0.0 if edge else rng.uniform(-0.004, 0.004)
            row.append(bm.verts.new((x, y, height(x) + jitter)))
        top.append(row)
    for j in range(steps_y):
        for i in range(steps_x):
            bm.faces.new((top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i]))

    # Sides and bottom.
    def side(edge_verts, reverse):
        bottoms = [bm.verts.new((v.co.x, v.co.y, 0.0)) for v in edge_verts]
        for k in range(len(edge_verts) - 1):
            quad = (edge_verts[k], bottoms[k], bottoms[k + 1], edge_verts[k + 1])
            bm.faces.new(tuple(reversed(quad)) if reverse else quad)
        return bottoms

    b_front = side(top[0], False)
    b_back = side(top[-1], True)
    b_left = side([top[j][0] for j in range(steps_y + 1)], True)
    b_right = side([top[j][-1] for j in range(steps_y + 1)], False)
    bmesh.ops.remove_doubles(bm, verts=b_front + b_back + b_left + b_right, dist=1e-6)
    bottom_ring = [v for v in bm.verts if abs(v.co.z) < 1e-6]
    bottom_ring.sort(key=lambda v: math.atan2(v.co.y, v.co.x), reverse=True)
    bm.faces.new(bottom_ring)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    return finish_object("SM_Soil_Tilled", bm, [mat_soil], collection)


# ---------------------------------------------------------------------------
# Turnip

def add_root(bm, scale=1.0, drop=0.0):
    radius = 0.06 * scale
    center_z = 0.04 * scale - drop
    result = bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=radius)
    verts = result["verts"]
    for v in verts:
        z = v.co.z
        v.co.z *= 0.9
        if z < 0:
            # Pull the bottom into a short tail.
            v.co.z -= 0.04 * scale * (-z / radius) ** 3
            v.co.x *= 1 - 0.35 * (-z / radius) ** 2
            v.co.y *= 1 - 0.35 * (-z / radius) ** 2
        v.co.z += center_z
    faces = {f for v in verts for f in v.link_faces}
    top_line = center_z + 0.012 * scale
    for f in faces:
        f.material_index = 1 if f.calc_center_median().z > top_line else 0
    return center_z + radius * 0.9


def add_leaf(bm, base_z, yaw, length, lean_start, lean_end, width, thickness):
    # Centerline curving outward from steep to shallower, with half-widths along it.
    stations = [0.0, 0.3, 0.65, 1.0]
    half_widths = [0.010, 0.045, 0.038, 0.006]
    points, pos, prev_t = [], Vector((0.0, 0.0, 0.0)), 0.0
    for t in stations:
        if t > 0:
            angle = math.radians(lean_start + (lean_end - lean_start) * t)
            seg = (t - prev_t) * length
            pos = pos + Vector((0.0, math.cos(angle) * seg, math.sin(angle) * seg))
        points.append(pos.copy())
        prev_t = t

    left, right = [], []
    for p, hw in zip(points, half_widths):
        left.append(bm.verts.new((p.x - hw * width, p.y, p.z)))
        right.append(bm.verts.new((p.x + hw * width, p.y, p.z)))
    faces = [bm.faces.new((left[k], right[k], right[k + 1], left[k + 1])) for k in range(len(points) - 1)]
    for f in faces:
        f.material_index = 2
    verts = left + right
    # Give the leaf thickness so it doesn't vanish from behind in Unreal.
    solid = bmesh.ops.solidify(bm, geom=faces, thickness=thickness)
    new_verts = {e for e in solid["geom"] if isinstance(e, bmesh.types.BMVert)}
    for f in {f for v in new_verts for f in v.link_faces}:
        f.material_index = 2
    all_verts = set(verts) | new_verts
    transform_verts(bm, all_verts, Matrix.Translation((0, 0, base_z)) @ Matrix.Rotation(yaw, 4, "Z"))


def build_turnip(name, collection, mats, leaf_count, leaf_scale, root_scale=None, root_drop=0.0, seed=1):
    rng = random.Random(seed)
    bm = bmesh.new()
    base_z = 0.005
    if root_scale:
        base_z = add_root(bm, root_scale, root_drop) - 0.01 * root_scale
    for k in range(leaf_count):
        yaw = 2 * math.pi * k / leaf_count + rng.uniform(-0.15, 0.15)
        s = leaf_scale * rng.uniform(0.88, 1.08)
        add_leaf(bm, base_z, yaw, length=0.26 * s, lean_start=rng.uniform(70, 80), lean_end=rng.uniform(30, 45),
                 width=s * 1.1, thickness=0.004)
    return finish_object(name, bm, mats, collection)


# ---------------------------------------------------------------------------
# Watering can: origin at the top of the handle (the grip).

def build_watering_can(collection, mat_can, mat_brass):
    bm = bmesh.new()
    body_h, body_r = 0.18, 0.08

    def cylinder(r1, r2, depth, segments, matrix, material):
        res = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                                    radius1=r1, radius2=r2, depth=depth, matrix=matrix)
        for f in {f for v in res["verts"] for f in v.link_faces}:
            f.material_index = material
        return res["verts"]

    # Body with a slight taper toward the top.
    cylinder(body_r, body_r * 0.92, body_h, 16, Matrix.Translation((0, 0, body_h / 2)), 0)
    # Rim.
    rim = bmesh.ops.create_cone(bm, cap_ends=False, segments=16, radius1=body_r * 0.94, radius2=body_r * 0.94,
                                depth=0.012, matrix=Matrix.Translation((0, 0, body_h + 0.006)))
    for f in {f for v in rim["verts"] for f in v.link_faces}:
        f.material_index = 0

    # Spout from low on the body, angled up and out along +X.
    angle = math.radians(50)
    direction = Vector((math.sin(angle), 0, math.cos(angle)))
    start = Vector((0.05, 0, 0.035))
    length = 0.22
    rot = Matrix.Rotation(angle, 4, "Y")
    mid = start + direction * (length / 2)
    cylinder(0.014, 0.010, length, 8, Matrix.Translation(mid) @ rot, 0)

    # Rose (sprinkler head) on the spout tip, wide end outward.
    tip = start + direction * length
    rose_depth = 0.03
    cylinder(0.011, 0.026, rose_depth, 12, Matrix.Translation(tip + direction * (rose_depth / 2)) @ rot, 1)

    # Arched handle over the top, running front to back.
    major, minor, ring_seg, tube_seg = 0.07, 0.011, 16, 6
    rings = []
    for i in range(ring_seg + 1):
        a = math.pi * i / ring_seg  # half torus: only the arch above the body
        c = Vector((math.cos(a) * major * 0.95, 0, body_h + math.sin(a) * major))
        tangent = Vector((-math.sin(a), 0, math.cos(a)))
        normal = Vector((math.cos(a), 0, math.sin(a)))
        binormal = tangent.cross(normal)
        ring = []
        for j in range(tube_seg):
            b = 2 * math.pi * j / tube_seg
            ring.append(bm.verts.new(c + (normal * math.cos(b) + binormal * math.sin(b)) * minor))
        rings.append(ring)
    for i in range(ring_seg):
        for j in range(tube_seg):
            f = bm.faces.new((rings[i][j], rings[i][(j + 1) % tube_seg], rings[i + 1][(j + 1) % tube_seg], rings[i + 1][j]))
            f.material_index = 0

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    # Move so the top of the handle (the grip) is at the origin.
    grip = Vector((0, 0, body_h + major + minor))
    transform_verts(bm, bm.verts, Matrix.Translation(-grip))
    return finish_object("SM_WateringCan", bm, [mat_can, mat_brass], collection)


# ---------------------------------------------------------------------------

def build_all():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0

    mat_soil = make_material("M_Soil", "6B4A2B", 0.95)
    mat_root = make_material("M_TurnipRoot", "F2EEE6", 0.6)
    mat_top = make_material("M_TurnipTop", "7A3B8F", 0.6)
    mat_leaf = make_material("M_TurnipLeaf", "4C9A2A", 0.7)
    mat_can = make_material("M_WateringCan", "2F7F7A", 0.45)
    mat_brass = make_material("M_WateringCanBrass", "B08D57", 0.35)
    turnip_mats = [mat_root, mat_top, mat_leaf]

    objects = [
        build_soil(new_collection("Soil"), mat_soil),
        build_turnip("SM_Crop_Turnip_Stage0", new_collection("Crop_Turnip_Stage0"), turnip_mats, 2, 0.4, seed=10),
        build_turnip("SM_Crop_Turnip_Stage1", new_collection("Crop_Turnip_Stage1"), turnip_mats, 3, 0.62, seed=11),
        build_turnip("SM_Crop_Turnip_Stage2", new_collection("Crop_Turnip_Stage2"), turnip_mats, 5, 0.9, root_scale=0.6, root_drop=0.03, seed=12),
        build_turnip("SM_Crop_Turnip_Ripe", new_collection("Crop_Turnip_Ripe"), turnip_mats, 5, 1.15, root_scale=1.0, seed=13),
        build_watering_can(new_collection("WateringCan"), mat_can, mat_brass),
    ]
    return objects


def export_all(objects):
    import addon_utils
    addon_utils.enable("io_scene_fbx", default_set=True)
    os.makedirs(EXPORT_DIR, exist_ok=True)
    for obj in objects:
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.fbx(
            filepath=os.path.join(EXPORT_DIR, obj.name + ".fbx"),
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
    bpy.ops.object.select_all(action="DESELECT")
    os.makedirs(os.path.dirname(BLEND_PATH), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)


def render_previews(objects, out_dir):
    """Renders each asset on its own, for checking the result without opening Blender."""
    os.makedirs(out_dir, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 48
    scene.render.resolution_x = scene.render.resolution_y = 640
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("Preview") if not scene.world else scene.world
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.75, 0.8, 0.85, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.8

    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    sun.data.energy = 3.5
    sun.rotation_euler = (math.radians(50), math.radians(10), math.radians(35))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
    scene.collection.objects.link(cam)
    scene.camera = cam

    for obj in objects:
        for other in objects:
            other.hide_render = other is not obj
        corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
        center = sum(corners, Vector()) / 8
        radius = max((c - center).length for c in corners)
        direction = Vector((1.0, -1.3, 0.9)).normalized()
        cam.location = center + direction * radius * 3.2
        cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
        cam.data.lens = 50
        scene.render.filepath = os.path.join(out_dir, obj.name + ".png")
        bpy.ops.render.render(write_still=True)
    # Showcase: the growth stages lined up on the middle ridge of a soil tile, can alongside.
    soil, *crops, can = objects
    for obj in objects:
        obj.hide_render = False
    for k, crop in enumerate(crops):
        crop.location = (0.0, -0.36 + 0.24 * k, 0.06)
    can.location = (0.45, 0.75, can.dimensions.z - 0.0)
    cam.location = (1.55, -1.25, 1.05)
    cam.rotation_euler = (Vector((0.1, 0.1, 0.1)) - cam.location).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 40
    scene.render.resolution_x = 960
    scene.render.filepath = os.path.join(out_dir, "Showcase.png")
    bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    built = build_all()
    export_all(built)
    if "--preview-dir" in args:
        render_previews(built, args[args.index("--preview-dir") + 1])
    for o in built:
        dims = o.dimensions
        print(f"{o.name}: {dims.x:.3f} x {dims.y:.3f} x {dims.z:.3f} m")
