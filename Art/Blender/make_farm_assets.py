# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds the farm assets procedurally and exports them for Unreal: the tilled soil tile, the growth
stages of each crop (turnip, potato, cabbage) and the watering can.

Run with Blender (any 4.x/5.x):
    blender --background --python Art/Blender/make_farm_assets.py
or with the bpy module:
    python Art/Blender/make_farm_assets.py [--preview-dir DIR]
With Blender, pass the preview folder after a double dash:
    blender --background --python Art/Blender/make_farm_assets.py -- --preview-dir DIR

Writes Art/Blender/FarmAssets.blend and one FBX per asset in Art/Export.
All sizes are in meters. Z = 0 is the ground (soil) or, for the watering can, the grip.
It prints each mesh's triangles and size, and warns when a crop stage grows less than
STAGE_GROWTH_MIN over the stage before it.
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
# Plant parts shared by the potato and the cabbage. The turnip keeps its own add_leaf above, so
# tuning these never changes the turnip's geometry.

def yaw_towards(dx, dy):
    """The yaw about Z that turns a part built along +Y to point along (dx, dy)."""
    return math.atan2(-dx, dy)


def add_blade(bm, origin, yaw, length, lean_start, lean_end, stations, half_widths, material, cup=0.0):
    """A leaf, leaflet or petal along a centerline that bends from lean_start to lean_end degrees
    above the horizontal; past 90 it curls back over its base. Half widths are fractions of the
    length, and a width of zero is a pointed tip. cup lifts the edges by that share of the half
    width toward the leaf's upper side, so broad leaves read as cupped instead of flat cards.
    The front faces the leaf's upper side; a reversed copy is its back, so it shows from both sides
    without a two-sided material and without the zero-area rim a solidify of a thin sheet leaves."""
    rows, pos, prev_t = [], Vector((0.0, 0.0, 0.0)), 0.0
    columns = (-1.0, 0.0, 1.0) if cup else (-1.0, 1.0)
    for t, hw in zip(stations, half_widths):
        angle = math.radians(lean_start + (lean_end - lean_start) * t)
        if t > 0:
            seg = (t - prev_t) * length
            pos = pos + Vector((0.0, math.cos(angle) * seg, math.sin(angle) * seg))
        prev_t = t
        up = Vector((0.0, -math.sin(angle), math.cos(angle)))
        half = hw * length
        if half < 1e-6:
            rows.append([bm.verts.new(pos)])
            continue
        rows.append([bm.verts.new(pos + Vector((c * half, 0.0, 0.0)) + up * (cup * half * abs(c))) for c in columns])

    faces = []
    for a, b in zip(rows, rows[1:]):
        if len(a) == 1:
            faces += [bm.faces.new((a[0], b[i + 1], b[i])) for i in range(len(b) - 1)]
        elif len(b) == 1:
            faces += [bm.faces.new((a[i], a[i + 1], b[0])) for i in range(len(a) - 1)]
        else:
            faces += [bm.faces.new((a[i], a[i + 1], b[i + 1], b[i])) for i in range(len(a) - 1)]
    back = [e for e in bmesh.ops.duplicate(bm, geom=faces)["geom"] if isinstance(e, bmesh.types.BMFace)]
    bmesh.ops.reverse_faces(bm, faces=back)
    for f in faces + back:
        f.material_index = material
    verts = {v for f in faces + back for v in f.verts}
    transform_verts(bm, verts, Matrix.Translation(origin) @ Matrix.Rotation(yaw, 4, "Z"))


def add_stem(bm, points, radius, material, sides=3):
    """A thin tapered stem through the points, closing to a point at the last one."""
    rings = []
    for k, p in enumerate(points):
        if k == len(points) - 1:
            rings.append([bm.verts.new(p)])
            break
        tangent = (points[k + 1] - p).normalized()
        side = tangent.cross(Vector((0.0, 0.0, 1.0)))
        side = side.normalized() if side.length > 1e-6 else Vector((1.0, 0.0, 0.0))
        other = tangent.cross(side)
        r = radius * (1 - 0.5 * k / (len(points) - 1))
        rings.append([bm.verts.new(p + (side * math.cos(2 * math.pi * j / sides) + other * math.sin(2 * math.pi * j / sides)) * r)
                      for j in range(sides)])
    for a, b in zip(rings, rings[1:]):
        for j in range(sides):
            f = bm.faces.new((a[j], a[(j + 1) % sides], b[0])) if len(b) == 1 else \
                bm.faces.new((a[j], a[(j + 1) % sides], b[(j + 1) % sides], b[j]))
            f.material_index = material


def add_lump(bm, center, size, yaw, material, rng, u_segments=6, v_segments=4, jitter=0.12):
    """A lumpy, flat-shaded ellipsoid: a potato tuber or a cabbage head."""
    res = bmesh.ops.create_uvsphere(bm, u_segments=u_segments, v_segments=v_segments, radius=1.0)
    for v in res["verts"]:
        k = 1 + rng.uniform(-jitter, jitter)
        v.co = Vector((v.co.x * size[0] * k, v.co.y * size[1] * k, v.co.z * size[2] * k))
    transform_verts(bm, res["verts"], Matrix.Translation(center) @ Matrix.Rotation(yaw, 4, "Z"))
    for f in {f for v in res["verts"] for f in v.link_faces}:
        f.material_index = material


def point_on(points, t):
    """The point at share t of the way along a polyline, by length, and the direction there."""
    lengths = [(b - a).length for a, b in zip(points, points[1:])]
    target = t * sum(lengths)
    last = len(lengths) - 1
    for i, seg in enumerate(lengths):
        if seg < 1e-9:
            continue
        if target <= seg or i == last:
            a, b = points[i], points[i + 1]
            return a.lerp(b, min(1.0, target / seg)), (b - a).normalized()
        target -= seg
    return points[-1], Vector((0.0, 0.0, 1.0))


def finish_plant(name, bm, materials, collection):
    # No recalc_face_normals: every part is built facing out, and a recalc could turn a leaf's
    # front and back copies the same way.
    return finish_object(name, bm, materials, collection)


# ---------------------------------------------------------------------------
# Potato: a bush of small, dark, pointed leaflets on stems. Ripe, the foliage turns yellow and
# sags, and tubers show on the ridge (along Y, where the soil stays at ridge height).

LEAFLET_STATIONS, LEAFLET_WIDTHS = [0.0, 0.4, 1.0], [0.0, 0.42, 0.0]
TUBER_SPOTS = [(0.02, 0.115, 0.4), (-0.025, -0.11, 1.9), (0.03, -0.20, 2.6)]


def build_potato(name, collection, mats, stems, height, reach, leaflets, leaflet_size, seed,
                 droop=0.0, flowers=0, dry_share=0.0, tubers=0):
    """mats: the potato's materials by part; the mesh gets a slot only for the parts it uses."""
    rng = random.Random(seed)
    parts = ["Leaf"] + (["LeafDry"] if dry_share else []) + (["Flower"] if flowers else []) + (["Tuber"] if tubers else [])
    slot = {part: k for k, part in enumerate(parts)}
    bm = bmesh.new()
    for s in range(stems):
        yaw = 2 * math.pi * s / stems + rng.uniform(-0.25, 0.25) + 0.3
        out = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        h = height * rng.uniform(0.85, 1.0)
        r = reach * rng.uniform(0.85, 1.05)
        points = [Vector((0, 0, 0)) + out * 0.006, out * r * 0.3 + Vector((0, 0, h * 0.75)),
                  out * r + Vector((0, 0, h * (1.0 - droop)))]
        dry = rng.random() < dry_share
        stem_mat = slot["LeafDry"] if dry else slot["Leaf"]
        add_stem(bm, points, 0.006, stem_mat)
        # Leaflets in pairs along the outer part of the stem and one at its tip, lying almost flat,
        # so the plant reads as a leafy mound from above rather than a bundle of stalks.
        for k in range(leaflets):
            t = 0.3 + 0.7 * k / max(1, leaflets - 1)
            p, along = point_on(points, t)
            if k == leaflets - 1:
                direction = Vector((along.x, along.y, 0.0))
            else:
                turn = math.radians(65 if k % 2 else -65)
                direction = Matrix.Rotation(turn, 3, "Z") @ Vector((along.x, along.y, 0.0))
            leaf_mat = slot["LeafDry"] if (dry or rng.random() < dry_share * 0.6) else slot["Leaf"]
            lean = rng.uniform(15, 30) - 45 * droop
            add_blade(bm, p, yaw_towards(direction.x, direction.y), leaflet_size * rng.uniform(0.85, 1.1),
                      lean, lean - 20, LEAFLET_STATIONS, LEAFLET_WIDTHS, leaf_mat)
        if s < flowers:
            for j in range(2):
                tip = points[-1] + Vector((rng.uniform(-0.015, 0.015), rng.uniform(-0.015, 0.015), 0.018 + 0.008 * j))
                res = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=True, segments=5, radius1=0.016,
                                            radius2=0.0, depth=0.008, matrix=Matrix.Translation(tip))
                for f in {f for v in res["verts"] for f in v.link_faces}:
                    f.material_index = slot["Flower"]
    for x, y, yaw in TUBER_SPOTS[:tubers]:
        add_lump(bm, Vector((x, y, 0.012)), (0.042, 0.03, 0.026), yaw, slot["Tuber"], rng)
    return finish_plant(name, bm, [mats[p] for p in parts], collection)


# ---------------------------------------------------------------------------
# Cabbage: a low rosette of broad, cupped, blue-green leaves; later a pale head forms in the middle.

CABBAGE_STATIONS, CABBAGE_WIDTHS = [0.0, 0.3, 0.75, 1.0], [0.07, 0.42, 0.40, 0.15]
COTYLEDON_STATIONS, COTYLEDON_WIDTHS = [0.0, 0.45, 1.0], [0.08, 0.5, 0.3]
CABBAGE_S0_SEED_LEAF, CABBAGE_S0_TRUE_LEAF = 0.075, 0.055
CABBAGE_S1_LEAF = 0.106
CABBAGE_S2_LEAF = 0.15
CABBAGE_S3_HEART = 0.07  # centre height and radius of the curled heart leaves


def add_rosette(bm, rng, count, length, lean_start, lean_end, material, start=0.008, cup=0.35, yaw_offset=0.0,
                stations=CABBAGE_STATIONS, widths=CABBAGE_WIDTHS):
    for k in range(count):
        yaw = 2 * math.pi * k / count + yaw_offset + rng.uniform(-0.12, 0.12)
        out = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        s = rng.uniform(0.9, 1.08)
        add_blade(bm, out * start + Vector((0, 0, 0.004)), yaw_towards(out.x, out.y), length * s,
                  lean_start + rng.uniform(-5, 5), lean_end + rng.uniform(-5, 5), stations, widths, material, cup=cup)


def add_wrapped_leaves(bm, rng, count, center_z, radius, theta_start, theta_end, material, yaw_offset=0.0, cup=0.3):
    """Leaves that follow a circle around a head (centre center_z on the axis, given radius),
    from theta_start (leaning out) to theta_end (curled over), so they hug or close a head."""
    for k in range(count):
        yaw = 2 * math.pi * k / count + yaw_offset + rng.uniform(-0.1, 0.1)
        out = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        r = radius * rng.uniform(0.95, 1.05)
        t0 = math.radians(theta_start)
        base = out * (r * math.sin(t0)) + Vector((0, 0, center_z - r * math.cos(t0)))
        length = r * math.radians(theta_end - theta_start)
        add_blade(bm, base, yaw_towards(out.x, out.y), length, theta_start, theta_end,
                  CABBAGE_STATIONS, CABBAGE_WIDTHS, material, cup=cup)


def build_cabbage(name, collection, mats, stage, seed):
    rng = random.Random(seed)
    bm = bmesh.new()
    leaf, head = 0, 1
    if stage == 0:
        # Seedling: two broad seed leaves held up off the ground and the first true leaf between them.
        add_rosette(bm, rng, 2, CABBAGE_S0_SEED_LEAF, 56, 22, leaf, start=0.004, cup=0.0, yaw_offset=0.4,
                    stations=COTYLEDON_STATIONS, widths=COTYLEDON_WIDTHS)
        add_rosette(bm, rng, 1, CABBAGE_S0_TRUE_LEAF, 70, 45, leaf, start=0.002, cup=0.0, yaw_offset=2.0)
        used = [mats["Leaf"]]
    elif stage == 1:
        # A raised cross of four leaves.
        add_rosette(bm, rng, 4, CABBAGE_S1_LEAF, 60, 18, leaf)
        used = [mats["Leaf"]]
    elif stage == 2:
        # A bowl: six leaves rising steeply from the centre, then opening out.
        add_rosette(bm, rng, 6, CABBAGE_S2_LEAF, 67, 0, leaf, cup=0.45)
        used = [mats["Leaf"]]
    elif stage == 3:
        # A bowl with a pale heart: inner leaves curl over the centre, wide outer leaves around them.
        add_rosette(bm, rng, 5, 0.17, 45, 0, leaf, start=0.02)
        add_wrapped_leaves(bm, rng, 4, CABBAGE_S3_HEART, CABBAGE_S3_HEART, 35, 140, head, yaw_offset=0.6, cup=0.0)
        used = [mats["Leaf"], mats["Head"]]
    else:
        # Ripe: a big, tight, pale head; the wrapper leaves rise against its sides, then flare out.
        add_lump(bm, Vector((0, 0, 0.10)), (0.11, 0.11, 0.095), 0.0, head, rng, u_segments=8, v_segments=6, jitter=0.04)
        add_rosette(bm, rng, 6, 0.22, 80, -10, leaf, start=0.075, cup=0.45, yaw_offset=0.3)
        used = [mats["Leaf"], mats["Head"]]
    return finish_plant(name, bm, used, collection)


# ---------------------------------------------------------------------------
# Size check: the game scales a growing crop from 80% on a stage's first day to 100% on its last,
# so each growth stage must be at least 1.25 times the stage before it in height (Z size) and in
# spread (the larger of the X and Y sizes), or the end of one stage looks bigger than the start of
# the next. The automated test checks the same on the imported meshes' bounds, with 1% tolerance.
# The ripe mesh has no size rule (dying foliage can shrink); it only has to look clearly different.

STAGE_GROWTH_MIN = 1.25


def crop_name(obj):
    parts = obj.name.split("_")
    return parts[2] if len(parts) == 4 and parts[1] == "Crop" else None


def report_sizes(objects):
    previous = {}
    for obj in objects:
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        dims = obj.dimensions
        line = f"{obj.name}: {tris} tris, {dims.x:.3f} x {dims.y:.3f} x {dims.z:.3f} m"
        crop = crop_name(obj)
        if crop:
            height, spread = dims.z, max(dims.x, dims.y)
            if crop in previous:
                h_ratio, s_ratio = height / previous[crop][0], spread / previous[crop][1]
                line += f", x{h_ratio:.2f} height, x{s_ratio:.2f} spread"
                if not obj.name.endswith("_Ripe") and min(h_ratio, s_ratio) < STAGE_GROWTH_MIN:
                    line += f"  WARNING: under x{STAGE_GROWTH_MIN} the stage before"
            previous[crop] = (height, spread)
        print(line)


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
    potato_mats = {
        "Leaf": make_material("M_PotatoLeaf", "3E7B34", 0.7),
        "LeafDry": make_material("M_PotatoLeafDry", "B89A3A", 0.8),
        "Flower": make_material("M_PotatoFlower", "EDEBE1", 0.6),
        "Tuber": make_material("M_PotatoTuber", "E0C38F", 0.85),
    }
    cabbage_mats = {
        "Leaf": make_material("M_CabbageLeaf", "6E9A80", 0.5),
        "Head": make_material("M_CabbageHead", "B7D58C", 0.55),
    }

    objects = [
        build_soil(new_collection("Soil"), mat_soil),
        build_turnip("SM_Crop_Turnip_Stage0", new_collection("Crop_Turnip_Stage0"), turnip_mats, 2, 0.4, seed=10),
        build_turnip("SM_Crop_Turnip_Stage1", new_collection("Crop_Turnip_Stage1"), turnip_mats, 3, 0.62, seed=11),
        build_turnip("SM_Crop_Turnip_Stage2", new_collection("Crop_Turnip_Stage2"), turnip_mats, 5, 0.9, root_scale=0.6, root_drop=0.03, seed=12),
        build_turnip("SM_Crop_Turnip_Ripe", new_collection("Crop_Turnip_Ripe"), turnip_mats, 5, 1.15, root_scale=1.0, seed=13),
        build_potato("SM_Crop_Potato_Stage0", new_collection("Crop_Potato_Stage0"), potato_mats,
                     stems=3, height=0.065, reach=0.035, leaflets=3, leaflet_size=0.06, seed=20),
        build_potato("SM_Crop_Potato_Stage1", new_collection("Crop_Potato_Stage1"), potato_mats,
                     stems=4, height=0.09, reach=0.06, leaflets=4, leaflet_size=0.065, seed=21),
        build_potato("SM_Crop_Potato_Stage2", new_collection("Crop_Potato_Stage2"), potato_mats,
                     stems=5, height=0.15, reach=0.11, leaflets=4, leaflet_size=0.08, seed=22, flowers=3),
        build_potato("SM_Crop_Potato_Ripe", new_collection("Crop_Potato_Ripe"), potato_mats,
                     stems=6, height=0.17, reach=0.17, leaflets=3, leaflet_size=0.085, seed=23,
                     droop=0.5, dry_share=0.65, tubers=3),
        build_cabbage("SM_Crop_Cabbage_Stage0", new_collection("Crop_Cabbage_Stage0"), cabbage_mats, 0, seed=30),
        build_cabbage("SM_Crop_Cabbage_Stage1", new_collection("Crop_Cabbage_Stage1"), cabbage_mats, 1, seed=31),
        build_cabbage("SM_Crop_Cabbage_Stage2", new_collection("Crop_Cabbage_Stage2"), cabbage_mats, 2, seed=32),
        build_cabbage("SM_Crop_Cabbage_Stage3", new_collection("Crop_Cabbage_Stage3"), cabbage_mats, 3, seed=33),
        build_cabbage("SM_Crop_Cabbage_Ripe", new_collection("Crop_Cabbage_Ripe"), cabbage_mats, 4, seed=34),
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


RIDGE_TOP = 0.06  # crops stand on the middle ridge of a tilled tile
GAME_CAMERA_DISTANCE = 4.0  # the player's camera boom
GAME_CAMERA_PITCH = math.radians(30)
GAME_CAMERA_FOV = math.radians(90)  # Unreal's default horizontal field of view
LINEUP_CLOSE_SPACING = 0.5


def setup_preview_scene():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.film_transparent = False
    world = scene.world or bpy.data.worlds.new("Preview")
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
    return scene, cam


def aim(cam, location, target):
    cam.location = location
    cam.rotation_euler = (target - location).to_track_quat("-Z", "Y").to_euler()


def game_camera(cam, target):
    """The play camera: 4 m from the target, 30 degrees down, 90 degree horizontal field of view,
    looking at the front (+X) of the asset."""
    offset = Vector((math.cos(GAME_CAMERA_PITCH), 0.0, math.sin(GAME_CAMERA_PITCH))) * GAME_CAMERA_DISTANCE
    aim(cam, target + offset, target)
    cam.data.sensor_fit = "HORIZONTAL"
    cam.data.angle = GAME_CAMERA_FOV


def render(scene, path, width, height):
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def tile_copy(soil, location):
    tile = soil.copy()
    bpy.context.scene.collection.objects.link(tile)
    tile.location = location
    tile.hide_render = False
    return tile


def render_previews(objects, out_dir):
    """Renders every crop with the art review's fixed setup, on the middle ridge of a 1 m tile:
    the game camera view and three close views (front, side, three-quarter from above), then each
    crop's stages side by side on a row of tiles and the whole field, from the game camera."""
    os.makedirs(out_dir, exist_ok=True)
    scene, cam = setup_preview_scene()
    soil = next(o for o in objects if o.name == "SM_Soil_Tilled")
    crops = [o for o in objects if crop_name(o)]
    for obj in objects:
        obj.hide_render = True
    soil.hide_render = False

    for crop in crops:
        crop.hide_render = False
        crop.location = (0.0, 0.0, RIDGE_TOP)
        bpy.context.view_layer.update()
        corners = [crop.matrix_world @ Vector(c) for c in crop.bound_box]
        center = sum(corners, Vector()) / 8
        radius = max(0.12, max((c - center).length for c in corners))
        game_camera(cam, center)
        render(scene, os.path.join(out_dir, crop.name + "_Game.png"), 1280, 720)
        cam.data.sensor_fit = "AUTO"
        cam.data.lens = 50
        views = {"Front": Vector((1, 0, 0.12)), "Side": Vector((0, -1, 0.12)), "ThreeQuarter": Vector((1, -1, 0.9))}
        for view, direction in views.items():
            aim(cam, center + direction.normalized() * radius * 3.4, center)
            render(scene, os.path.join(out_dir, f"{crop.name}_{view}.png"), 640, 640)
        crop.hide_render = True
        crop.location = (0.0, 0.0, 0.0)

    # Lineups: one tile per stage along Y, left to right in the game camera, ripe last.
    by_crop = {}
    for crop in crops:
        by_crop.setdefault(crop_name(crop), []).append(crop)
    extra = []
    soil.hide_render = True
    for name, stages in by_crop.items():
        for col, crop in enumerate(stages):
            column = col if crop is not stages[-1] else 4  # ripe always in the fifth column
            y = column - 2.0
            extra.append(tile_copy(soil, (0.0, y, 0.0)))
            crop.location = (0.0, y, RIDGE_TOP)
            crop.hide_render = False
        game_camera(cam, Vector((0.0, 0.0, 0.1)))
        render(scene, os.path.join(out_dir, f"Lineup_{name}_Game.png"), 1280, 720)
        for tile in extra:
            bpy.data.objects.remove(tile)
        extra.clear()
        # Close lineup: the same stages half a metre apart on three tiles.
        for crop in stages:
            crop.location.y *= LINEUP_CLOSE_SPACING
        for y in (-1.0, 0.0, 1.0):
            extra.append(tile_copy(soil, (0.0, y, 0.0)))
        cam.data.sensor_fit = "AUTO"
        cam.data.lens = 30
        aim(cam, Vector((2.0, -0.7, 1.1)), Vector((0.0, 0.0, 0.08)))
        render(scene, os.path.join(out_dir, f"Lineup_{name}.png"), 1280, 640)
        for crop in stages:
            crop.hide_render = True
        for tile in extra:
            bpy.data.objects.remove(tile)
        extra.clear()

    # The field: one row of tiles per crop (along X), stages along Y, from the game camera.
    for row, (name, stages) in enumerate(by_crop.items()):
        x = 1.0 - row
        for col, crop in enumerate(stages):
            column = col if crop is not stages[-1] else 4
            y = column - 2.0
            extra.append(tile_copy(soil, (x, y, 0.0)))
            crop.location = (x, y, RIDGE_TOP)
            crop.hide_render = False
        for column in range(len(stages) - 1, 4):
            extra.append(tile_copy(soil, (x, column - 2.0, 0.0)))
    game_camera(cam, Vector((0.0, 0.0, 0.1)))
    render(scene, os.path.join(out_dir, "Field_Game.png"), 1280, 720)


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    built = build_all()
    export_all(built)
    report_sizes(built)
    if "--preview-dir" in args:
        render_previews(built, args[args.index("--preview-dir") + 1])
