# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""The gate before the export. validate_asset() prints what the asset is and raises
ValidationError, listing every problem, if it breaks a rule. run() turns the error into a
non-zero exit status, so a broken asset is never exported by a script that "succeeds".

What it checks (the rules are in CONTRIBUTING.md, section 5):
- the scene is metric with unit scale 1.0, and holds only this asset's objects;
- the main object is named like the Unreal asset (SM_...), with no parent and an identity
  transform (everything is in the vertices), and its pivot on the floor in the middle of its
  footprint;
- the size, if the script says what it must be;
- triangles against the budget of its kind, or a stated reason for going over;
- every polygon is a triangle, no zero-area faces, no doubled vertices, no loose vertices, no
  flipped face next to a neighbour, no edge shared by more than two faces, no closed piece that is
  inside out;
- material slots: every one has a material named MI_..., and every one is used;
- collision: UCX_<Mesh>_NN shapes, convex, closed, simple, no material, identity transform, the
  highest one reaching 0.46 m so the farmer can't step onto it (and none if the script says the
  player walks through the asset).

Only the kinds that stand on the ground, "prop" and "building", are known so far. A crop (the pivot
at the soil, no collision, under 1,000 triangles) and a tool (the pivot at the grip, a SOCKET_ empty,
no collision, under 2,000) need their own pivot rule and socket check: add them with the first
script for one of them.
"""

import hashlib
import re

import bmesh
import bpy
from mathutils import Matrix, Vector

from .geometry import signed_volume

# Triangles an asset of each kind must stay under: CONTRIBUTING.md, section 5, Budgets ("hundreds" for a
# small prop, under about 10,000 for a building).
BUDGETS = {"prop": 1000, "building": 10000}

UCX_MAX_VERTS = 32
# The farmer steps up anything 0.45 m high or lower: that is Unreal's default MaxStepHeight for a character,
# and the project sets none (VestigeCharacter.cpp only sets the capsule size). A solid thing whose
# highest collision shape ends at or under it would be climbed onto, not bumped into.
FARMER_STEP_HEIGHT = 0.45
MIN_COLLISION_TOP = FARMER_STEP_HEIGHT + 0.01   # m
EPS_AREA = 1e-9    # m^2
EPS_DOUBLE = 1e-5  # m
EPS_TRANSFORM = 1e-6


class ValidationError(Exception):
    pass


def _fmt(values):
    return "(" + ", ".join(f"{v:.3f}" for v in values) + ")"


def _identity_problem(obj, label):
    if obj.parent is not None:
        return f"{label} has a parent ({obj.parent.name})"
    delta = obj.matrix_world - Matrix.Identity(4)
    if max(abs(c) for row in delta for c in row) > EPS_TRANSFORM:
        return f"{label} has a location, rotation or scale; build it at the origin in the vertices"
    return None


def _islands(bm):
    """Groups of faces joined by shared edges."""
    seen, islands = set(), []
    for start in bm.faces:
        if start in seen:
            continue
        island, stack = [], [start]
        seen.add(start)
        while stack:
            face = stack.pop()
            island.append(face)
            for edge in face.edges:
                for other in edge.link_faces:
                    if other not in seen:
                        seen.add(other)
                        stack.append(other)
        islands.append(island)
    return islands


def _is_closed(faces):
    return all(len(edge.link_faces) == 2 for face in faces for edge in face.edges)


def _fingerprint(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    tris = []
    for t in mesh.loop_triangles:
        pts = [tuple(round(c, 5) for c in mesh.vertices[i].co) for i in t.vertices]
        tris.append((tuple(sorted(pts)), t.material_index))
    tris.sort()
    slots = [s.material.name if s.material else None for s in obj.material_slots]
    return hashlib.sha1(repr((tris, slots)).encode()).hexdigest()[:12]


def _check_collision(asset, problems, report):
    name = asset.main.name
    tops = []
    for index, obj in enumerate(asset.collision):
        label = obj.name
        if obj.name != f"UCX_{name}_{index:02d}":
            problems.append(f"{label}: expected the name UCX_{name}_{index:02d}")
        if obj.type != "MESH":
            problems.append(f"{label}: a collision shape must be a mesh")
            continue
        problem = _identity_problem(obj, label)
        if problem:
            problems.append(problem)
        if len(obj.material_slots):
            problems.append(f"{label}: a collision shape must have no material")
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bm.normal_update()
        if len(bm.verts) > UCX_MAX_VERTS:
            problems.append(f"{label}: {len(bm.verts)} vertices; keep a collision shape simple (at most {UCX_MAX_VERTS})")
        if not bm.faces or not _is_closed(bm.faces):
            problems.append(f"{label}: must be a closed solid")
        elif signed_volume(bm.faces) <= 0:
            problems.append(f"{label}: is inside out")
        for face in bm.faces:
            origin = face.verts[0].co
            if any((v.co - origin).dot(face.normal) > 1e-4 for v in bm.verts):
                problems.append(f"{label}: is not convex (Unreal makes a convex hull of it, which would differ)")
                break
        report["collision"].append(f"{label}: {len(bm.verts)} verts, {len(bm.faces)} tris")
        tops.append(max((v.co.z for v in bm.verts), default=0.0))
        bm.free()
    report["collision_top"] = max(tops, default=None)


def validate_asset(asset, kind, size=None, size_tol=0.01, pivot_tol=0.02, collision=True,
                   budget_reason=None):
    """Checks the asset, prints a summary and raises ValidationError listing every problem.

    kind           "prop" or "building": picks the triangle budget. Both stand on the ground, so the
                   origin must be where the asset touches it, in the middle of its footprint.
    size           the size the asset must have, (x, y, z) in metres, None for an axis that is free.
                   Use it when a size is a promise to something else (the footprint the game's
                   interaction box is made for).
    collision      True (the default): the player bumps into it, so UCX_ shapes are required, and the
                   highest must reach 0.46 m, since the farmer steps up 0.45 m.
                   False: the player walks through it, so there must be none.
    budget_reason  the reason to go over the budget; without one, going over is an error.
    """
    if kind not in BUDGETS:
        raise ValueError(f"kind must be one of {sorted(BUDGETS)}, not {kind!r}")
    problems, notes = [], []
    main = asset.main
    name = main.name
    scene = bpy.context.scene
    bpy.context.view_layer.update()  # so matrix_world is current, whatever was changed last
    report = {"name": name, "kind": kind, "collision": []}

    if not re.fullmatch(r"SM_[A-Za-z0-9]+(_[A-Za-z0-9]+)*", name):
        problems.append(f"the main object is called {name}; name it like the Unreal asset, SM_<Name> (CodingGuidelines.md, 14.2)")
    if scene.unit_settings.system != "METRIC" or abs(scene.unit_settings.scale_length - 1.0) > 1e-9:
        problems.append("the scene units must be metric with unit scale 1.0, so one Blender metre is one Unreal metre")
    stray = [o.name for o in scene.objects if o not in asset.objects]
    if stray:
        problems.append(f"objects that are not part of the asset are in the scene: {stray}")
    if main.type != "MESH":
        raise ValidationError(f"{name} is not a mesh")
    problem = _identity_problem(main, name)
    if problem:
        problems.append(problem)

    mesh = main.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.normal_update()
    if not bm.faces:
        problems.append(f"{name} has no faces")
    xs = [v.co.x for v in bm.verts] or [0.0]
    ys = [v.co.y for v in bm.verts] or [0.0]
    zs = [v.co.z for v in bm.verts] or [0.0]
    lo, hi = Vector((min(xs), min(ys), min(zs))), Vector((max(xs), max(ys), max(zs)))
    dims = hi - lo
    tris = sum(len(f.verts) - 2 for f in bm.faces)
    report.update(tris=tris, verts=len(bm.verts), bounds=(tuple(lo), tuple(hi)), size=tuple(dims))

    if any(len(f.verts) != 3 for f in bm.faces):
        problems.append("not every polygon is a triangle; finish_object() triangulates, so the mesh was changed after it")
    if abs(lo.z) > 0.002:
        problems.append(f"the pivot should be at the bottom: the lowest point is at z = {lo.z:.4f}")
    center = (lo + hi) / 2
    if abs(center.x) > pivot_tol or abs(center.y) > pivot_tol:
        problems.append(f"the pivot should be in the middle of the footprint: its centre is at x = {center.x:.3f}, y = {center.y:.3f}")
    if size is not None:
        for axis, want, have in zip("XYZ", size, dims):
            if want is not None and abs(have - want) > size_tol:
                problems.append(f"size {axis} is {have:.3f} m but must be {want:.3f} m (+/- {size_tol})")

    limit = BUDGETS[kind]
    if tris >= limit:
        if budget_reason:
            notes.append(f"{tris} triangles is over the {kind} budget of under {limit}; reason: {budget_reason}")
        else:
            problems.append(f"{tris} triangles is over the {kind} budget of under {limit} (CONTRIBUTING.md, Budgets); "
                            "simplify it or pass budget_reason= with a reason")
    elif budget_reason:
        notes.append("budget_reason is set but the asset is within its budget; remove it")

    zero = [f for f in bm.faces if f.calc_area() < EPS_AREA]
    if zero:
        problems.append(f"{len(zero)} zero-area faces")
    doubles = bmesh.ops.find_doubles(bm, verts=bm.verts, dist=EPS_DOUBLE)["targetmap"]
    if doubles:
        problems.append(f"{len(doubles)} doubled vertices (closer than {EPS_DOUBLE} m)")
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        problems.append(f"{len(loose)} loose vertices")
    flipped = 0
    for edge in bm.edges:
        if len(edge.link_loops) == 2 and edge.link_loops[0].vert == edge.link_loops[1].vert:
            flipped += 1
    if flipped:
        problems.append(f"{flipped} edges where neighbouring faces wind the opposite way (a flipped face)")
    non_manifold = sum(1 for edge in bm.edges if len(edge.link_faces) > 2)
    if non_manifold:
        problems.append(f"{non_manifold} edges shared by more than two faces (parts that overlap or touch along an edge)")
    islands = _islands(bm)
    inside_out = sum(1 for island in islands if _is_closed(island) and signed_volume(island) <= 0)
    if inside_out:
        problems.append(f"{inside_out} closed pieces are inside out")
    report["islands"] = len(islands)
    bm.free()

    per_slot = {}
    for poly in mesh.polygons:
        per_slot[poly.material_index] = per_slot.get(poly.material_index, 0) + len(poly.vertices) - 2
    slots = []
    if not main.material_slots:
        problems.append("the mesh has no material slot")
    for index, slot in enumerate(main.material_slots):
        material = slot.material
        label = material.name if material else None
        slots.append((label, per_slot.get(index, 0)))
        if material is None:
            problems.append(f"slot {index} has no material")
        elif not re.fullmatch(r"MI_[A-Za-z0-9_]+", material.name):
            problems.append(f"slot {index}: the material {material.name!r} should be named MI_<Name>, the Unreal instance it becomes")
        if per_slot.get(index, 0) == 0:
            problems.append(f"slot {index} ({label}) has no faces: remove it, an unused slot is a wasted draw call")
    if any(i >= len(main.material_slots) for i in per_slot):
        problems.append("faces use a material slot that does not exist")
    report["slots"] = slots
    report["uv_layers"] = [layer.name for layer in mesh.uv_layers]
    shaded_flat = sum(1 for p in mesh.polygons if not p.use_smooth)
    report["flat_polys"], report["smooth_polys"] = shaded_flat, len(mesh.polygons) - shaded_flat

    if collision and not asset.collision:
        problems.append(f"a {kind} needs UCX_{name}_00 collision (the player bumps into it); "
                        "Unreal would make its own convex hull otherwise")
    if not collision and asset.collision:
        problems.append("collision=False says the player walks through it, but it has UCX_ shapes")
    _check_collision(asset, problems, report)
    top = report["collision_top"]
    if collision and top is not None and top < MIN_COLLISION_TOP - 1e-6:
        problems.append(f"the highest collision shape ends at z = {top:.3f} m, but the farmer steps up {FARMER_STEP_HEIGHT:.2f} m "
                        "(Unreal's default MaxStepHeight, which the project keeps), so they would climb onto it instead "
                        f"of being stopped: make a shape reach at least {MIN_COLLISION_TOP:.2f} m, or, if the player "
                        "should walk over it, give it no collision (collision=False)")

    report["fingerprint"] = _fingerprint(main)
    report["notes"] = notes
    report["problems"] = problems
    _print_report(report, limit)
    if problems:
        raise ValidationError(f"{name} failed {len(problems)} check(s):\n  - " + "\n  - ".join(problems))
    return report


def _print_report(report, limit):
    print(f"[validate] {report['name']} ({report['kind']})")
    print(f"[validate]   triangles {report['tris']} (budget under {limit}), vertices {report['verts']}, "
          f"separate pieces {report['islands']}")
    lo, hi = report["bounds"]
    print(f"[validate]   size X x Y x Z {_fmt(report['size'])} m, bounds {_fmt(lo)} to {_fmt(hi)}, origin (0, 0, 0)")
    print("[validate]   material slots: " + ", ".join(f"{n} {t} tris" for n, t in report["slots"]))
    print(f"[validate]   shading: {report['flat_polys']} flat / {report['smooth_polys']} smooth polygons; "
          f"UV maps: {report['uv_layers'] or 'none'}")
    top = report["collision_top"]
    print(f"[validate]   collision: {report['collision'] or 'none'}" + ("" if top is None else f", highest top z = {top:.3f} m"))
    print(f"[validate]   geometry fingerprint {report['fingerprint']} (Blender {bpy.app.version_string})")
    for note in report["notes"]:
        print(f"[validate]   NOTE {note}")
    for problem in report["problems"]:
        print(f"[validate]   PROBLEM {problem}")
    if not report["problems"]:
        print("[validate]   OK")
