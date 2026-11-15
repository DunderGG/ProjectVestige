# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Self-test of the vestige_art gate and helpers: checks that the checks still check. Run it before and
after changing anything in vestige_art/ (validate.py, geometry.py, cli.py, palette.py ...), and before
approving such a change. It is not an asset script and writes nothing in the repository.

Run it from the project folder with plain Python 3 (no Blender add-ons, no packages):

    python Art/Blender/check_vestige_art.py                          all cases, about 1 to 2 minutes
    python Art/Blender/check_vestige_art.py bad_defect_inside_out    only the named cases (and "palette")

It prints PASS or FAIL for each case and "N of M passed", and exits with status 1 if any failed.
Blender: $BLENDER if set, else "blender" on the PATH, else C:\\Program Files\\Blender Foundation\\Blender 5.0.

Two modes in one file:
  python check_vestige_art.py [case ...]     the driver: runs each case in its own headless Blender and
        checks the exit status, whether a file was written and, for the cases that name one, that the
        error says the right thing. No case: all of them.
  (inside Blender, started by the driver)    builds the case named in $VESTIGE_CASE and calls run().

Every "bad" case is a good little asset with exactly one defect, so each check of validate.py, each
argument check of the primitives and the command line is seen to fail on its own, with exit status 1 (a
validation or build error) or 2 (a bad command line) and no FBX written. The "ok" cases must exit 0 and write
the FBX. A case that was meant to fail can still fail for the wrong reason, so MESSAGES lists the text the
output of some of them must contain; add an entry when you add a case.
"""

import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True  # the driver imports vestige_art.palette: leave no __pycache__ in the repository
ART = HERE  # the folder with vestige_art/
BLENDER = os.environ.get("BLENDER") or shutil.which("blender") or r"C:\Program Files\Blender Foundation\Blender 5.0\blender.exe"

try:
    import bpy  # noqa: F401
    IN_BLENDER = True
except ImportError:
    IN_BLENDER = False

# ---------------------------------------------------------------------------------------------------------
# Inside Blender: the cases

if IN_BLENDER:
    import bmesh
    from mathutils import Vector

    sys.dont_write_bytecode = True
    sys.path.insert(0, ART)
    from vestige_art import geometry as geo  # noqa: E402
    from vestige_art import materials  # noqa: E402
    from vestige_art.run import run  # noqa: E402

    def good(name="SM_Test", slot_keys=("PaintTeal",), collision=True, mutate=None, collision_top=0.5):
        """A 0.6 x 0.6 x 0.4 chamfered box with a collision box. mutate(bm, slots) may add or break things first."""
        slots = materials.Slots(slot_keys)
        bm = bmesh.new()
        geo.add_box(bm, (-0.3, -0.3, 0.0), (0.3, 0.3, 0.4), slots["PaintTeal"], bevel=0.02)
        if mutate:
            mutate(bm, slots)
        asset = geo.finish_object(name, bm, slots, shade="angle")
        if collision:
            geo.add_collision_box(asset, (-0.3, -0.3, 0.0), (0.3, 0.3, collision_top))
        return asset

    def ok_helpers():
        """Every primitive of the module, including the ones added for the props. Its low hull (0.3 m)
        next to a tall box (0.6 m) also shows that only the highest collision shape has to clear the step."""
        slots = materials.Slots(["PaintTeal", "Brass", "Water", "Stone"])
        bm = bmesh.new()
        geo.add_box(bm, (-0.4, -0.4, 0.0), (0.4, 0.4, 0.3), slots["PaintTeal"], bevel=0.02, taper=-0.05, skip=("-z", "+z"))
        geo.add_ring(bm, (0.0, 0.0, 0.3), 0.1, 0.3, 0.1, 12, slots["Stone"], cap_bottom=False)
        geo.add_tube(bm, [(0.0, 0.0, 0.4), (0.1, 0.0, 0.5), (0.3, 0.0, 0.55)], [0.03, 0.02, 0.01], 8, slots["Brass"])
        geo.add_dome(bm, (0.0, 0.0, 0.4), 0.08, 12, 3, slots["Water"])
        outline = [(0.1 * math.cos(2 * math.pi * k / 12), 0.1 * math.sin(2 * math.pi * k / 12)) for k in range(12)]
        geo.add_prism(bm, outline, "Z", 0.6, 0.602, slots["Water"], skip=("lo", *range(12)))  # a one-sided disc
        geo.add_cylinder(bm, (0.2, 0.2, 0.3), 0.04, 0.1, 12, slots["Brass"], top_radius=0.03, axis="Y")
        asset = geo.finish_object("SM_Test", bm, slots, shade="angle", angle=26.0)
        geo.add_collision_box(asset, (-0.4, -0.4, 0.0), (0.4, 0.4, 0.6))
        geo.add_collision_hull(asset, [(0.3 * math.cos(2 * math.pi * k / 8), 0.3 * math.sin(2 * math.pi * k / 8), z)
                                       for k in range(8) for z in (0.0, 0.3)])
        return asset

    def one_piece_defect(defect):
        def mutate(bm, slots):
            if defect == "off_centre":
                geo.add_box(bm, (0.4, -0.3, 0.0), (1.2, 0.3, 0.1), slots["PaintTeal"])
            elif defect == "over_budget":   # one 300-sided cylinder: about 1,200 triangles on top of the box
                geo.add_cylinder(bm, (0.0, 0.0, 0.4), 0.1, 0.1, 300, slots["PaintTeal"])
        return mutate

    def build_defect(defect):
        if defect in ("off_centre", "over_budget"):
            return good(mutate=one_piece_defect(defect))
        if defect == "unused_slot":
            return good(slot_keys=("PaintTeal", "Rust"))
        if defect == "no_collision":
            return good(collision=False)
        if defect == "collision_too_low":
            return good(collision_top=0.45)   # exactly the height the farmer steps up
        asset = good()
        mesh = asset.main.data
        if defect == "loose_vertex":
            mesh.vertices.add(1)
        elif defect == "off_pivot":
            for v in mesh.vertices:   # the whole mesh floats 10 cm above the floor
                v.co.z += 0.1
        elif defect == "transform":
            asset.main.location = (0.0, 0.0, 0.01)
        elif defect == "bad_material_name":
            mesh.materials[0].name = "Teal"
        elif defect == "no_slot":
            mesh.materials.clear()
        elif defect == "stray_object":
            bpy.context.scene.collection.objects.link(bpy.data.objects.new("Stray", None))
        elif defect == "units":
            bpy.context.scene.unit_settings.scale_length = 0.01
        elif defect in ("inside_out", "flipped_face", "doubled_vertex", "zero_area", "non_manifold", "not_triangulated"):
            b = bmesh.new()
            b.from_mesh(mesh)
            if defect == "inside_out":
                bmesh.ops.reverse_faces(b, faces=b.faces[:])
            elif defect == "flipped_face":
                bmesh.ops.reverse_faces(b, faces=[b.faces[0]])
            elif defect == "doubled_vertex":
                v = b.verts[0]
                b.faces.new([b.verts.new(v.co), b.verts.new(v.co + Vector((0.1, 0, 0))), b.verts.new(v.co + Vector((0, 0.1, 0)))])
            elif defect == "zero_area":
                b.faces.new([b.verts.new((0.0, 0.0, 0.1)), b.verts.new((0.1, 0.0, 0.1)), b.verts.new((0.2, 0.0, 0.1))])
            elif defect == "non_manifold":   # a third face on an edge that already has two
                b.edges.ensure_lookup_table()
                b.faces.new([*b.edges[0].verts, b.verts.new((0.0, 0.0, 0.2))])
            elif defect == "not_triangulated":   # a quad, as if the mesh was changed after finish_object()
                b.faces.new([b.verts.new(p) for p in ((-0.05, -0.05, 0.1), (0.05, -0.05, 0.1), (0.05, 0.05, 0.1), (-0.05, 0.05, 0.1))])
            b.to_mesh(mesh)
            b.free()
        elif defect == "ucx_not_convex":
            lb = bmesh.new()
            geo.add_prism(lb, [(0, 0), (1, 0), (1, 0.4), (0.4, 0.4), (0.4, 1), (0, 1)], "Z", 0.0, 0.5, 0)
            geo._add_collision_object(asset, lb)
        elif defect == "ucx_bad_name":
            asset.collision[0].name = "UCX_Wrong_00"
        elif defect == "ucx_too_many_verts":
            asset.collision[0].data.vertices.add(40)
        elif defect == "ucx_material":
            asset.collision[0].data.materials.append(bpy.data.materials.new("MI_Prop_Test"))
        elif defect == "ucx_open":   # the shape loses a face
            shape = asset.collision[0].data
            lb = bmesh.new()
            lb.from_mesh(shape)
            bmesh.ops.delete(lb, geom=lb.faces[:1], context="FACES_ONLY")
            lb.to_mesh(shape)
            lb.free()
        return asset

    def build_arguments(case):
        """Calls a primitive with arguments it must refuse."""
        slots = materials.Slots(["PaintTeal"])
        bm = bmesh.new()
        if case == "box_bevel_too_big":
            geo.add_box(bm, (0, 0, 0), (0.1, 0.1, 0.1), 0, bevel=0.06)
        elif case == "box_taper_too_big":
            geo.add_box(bm, (0, 0, 0), (0.2, 0.2, 0.2), 0, taper=0.1)
        elif case == "box_skip_leaning_side":
            geo.add_box(bm, (0, 0, 0), (0.2, 0.2, 0.2), 0, taper=0.05, skip=("+x",))
        elif case == "ring_inner_over_outer":
            geo.add_ring(bm, (0, 0, 0), 0.3, 0.2, 0.1, 12, 0)
        elif case == "tube_one_point":
            geo.add_tube(bm, [(0, 0, 0)], 0.01, 8, 0)
        elif case == "tube_zero_radius":
            geo.add_tube(bm, [(0, 0, 0), (0, 0, 1)], [0.01, 0.0], 8, 0)
        elif case == "tube_same_points":
            geo.add_tube(bm, [(0, 0, 0), (0, 0, 0)], 0.01, 8, 0)
        elif case == "dome_one_ring":
            geo.add_dome(bm, (0, 0, 0), 0.1, 12, 1, 0)
        elif case == "cylinder_cone":
            geo.add_cylinder(bm, (0, 0, 0), 0.1, 0.1, 12, 0, top_radius=0.0)
        elif case == "prism_clockwise":
            geo.add_prism(bm, [(0, 0), (0, 1), (1, 1), (1, 0)], "Z", 0, 1, 0)
        elif case == "slots_twice":
            materials.Slots(["Brass", "Brass"])
        elif case == "slot_not_in_palette":
            materials.Slots(["Chartreuse"])
        elif case == "face_uses_missing_slot":
            geo.add_box(bm, (0, 0, 0), (0.1, 0.1, 0.1), 3)
            geo.finish_object("SM_Test", bm, slots)
        raise AssertionError(f"{case}: the primitive accepted arguments it must refuse")

    CASE = os.environ["VESTIGE_CASE"]
    if CASE == "ok_helpers":
        run("SM_Test", ok_helpers, "prop")
    elif CASE == "ok_plain":
        run("SM_Test", good, "prop", size=(0.6, 0.6, 0.4), size_tol=0.005)
    elif CASE == "ok_hull":
        def hull_asset():
            slots = materials.Slots(["PaintTeal"])
            bm = bmesh.new()
            geo.add_cylinder(bm, (0, 0, 0), 0.4, 0.8, 16, slots["PaintTeal"], cap_bottom=False)
            asset = geo.finish_object("SM_Test", bm, slots, shade="angle")
            geo.add_collision_hull(asset, [(0.4 * math.cos(2 * math.pi * k / 12), 0.4 * math.sin(2 * math.pi * k / 12), z)
                                           for k in range(12) for z in (0.0, 0.8)])
            return asset
        run("SM_Test", hull_asset, "prop")
    elif CASE == "ok_budget_reason":
        run("SM_Test", lambda: good(mutate=one_piece_defect("over_budget")), "prop", budget_reason="a test")
    elif CASE == "ok_building":
        run("SM_Test", good, "building")
    elif CASE == "ok_collision_top_at_limit":
        run("SM_Test", lambda: good(collision_top=0.46), "prop")   # 1 cm over the farmer's step: enough
    elif CASE == "ok_no_collision":
        run("SM_Test", lambda: good(collision=False), "prop", collision=False)   # walked through: no step rule
    elif CASE == "bad_wrong_name":
        run("SM_Other", good, "prop")
    elif CASE == "bad_kind_tool":
        run("SM_Test", good, "tool")
    elif CASE == "bad_size":
        run("SM_Test", good, "prop", size=(1.0, 1.0, None), size_tol=0.005)
    elif CASE == "bad_collision_forbidden":
        run("SM_Test", good, "prop", collision=False)
    elif CASE == "bad_name_not_sm":
        run("Bed", lambda: good(name="Bed"), "prop")
    elif CASE.startswith("bad_defect_"):
        defect = CASE[len("bad_defect_"):]
        run("SM_Test", lambda: build_defect(defect), "prop")
    elif CASE.startswith("bad_args_"):
        run("SM_Test", lambda: build_arguments(CASE[len("bad_args_"):]), "prop")
    elif CASE.startswith("cli_"):
        run("SM_Test", good, "prop")   # the command line is what fails, before anything is built
    else:
        raise SystemExit(f"unknown case {CASE}")
    sys.exit(0)

# ---------------------------------------------------------------------------------------------------------
# Outside Blender: the driver

DEFECTS = ("off_pivot", "off_centre", "over_budget", "unused_slot", "no_slot", "no_collision", "loose_vertex",
           "transform", "bad_material_name", "stray_object", "units", "inside_out", "flipped_face", "doubled_vertex",
           "zero_area", "non_manifold", "not_triangulated", "ucx_not_convex", "ucx_bad_name", "ucx_too_many_verts",
           "ucx_material", "ucx_open", "collision_too_low")
ARGUMENT_CASES = ("box_bevel_too_big", "box_taper_too_big", "box_skip_leaning_side", "ring_inner_over_outer",
                  "tube_one_point", "tube_zero_radius", "tube_same_points", "dome_one_ring", "cylinder_cone",
                  "prism_clockwise", "slots_twice", "slot_not_in_palette", "face_uses_missing_slot")
# A folder the preview guard must refuse to write into. If the guard ever breaks, the case writes the previews
# there: the driver then reports it and deletes the folder, so nothing is left in the checkout.
PREVIEW_TRAP = os.path.join(ART, "_selftest_previews_must_not_exist")
# name: (expected exit status, FBX expected, extra command line after the temp export dir)
CASES = {name: (0, True, []) for name in ("ok_helpers", "ok_plain", "ok_hull", "ok_budget_reason", "ok_building",
                                          "ok_collision_top_at_limit", "ok_no_collision")}
CASES.update({"bad_wrong_name": (1, False, []), "bad_kind_tool": (1, False, []), "bad_size": (1, False, []),
              "bad_collision_forbidden": (1, False, []), "bad_name_not_sm": (1, False, [])})
CASES.update({f"bad_defect_{d}": (1, False, []) for d in DEFECTS})
CASES.update({f"bad_args_{a}": (1, False, []) for a in ARGUMENT_CASES})
CASES.update({
    "cli_typo": (2, False, ["--exportdir", "x"]),
    "cli_check_only_with_export": (2, False, ["--check-only"]),
    "cli_preview_inside_repo": (2, False, ["--preview-dir", PREVIEW_TRAP]),
})
# What the output of a case must contain, so it can't pass by failing for another reason.
MESSAGES = {
    "bad_defect_over_budget": "is over the prop budget",
    "bad_defect_non_manifold": "edges shared by more than two faces",
    "bad_defect_collision_too_low": "the farmer steps up",
    "bad_defect_ucx_material": "must have no material",
    "bad_defect_ucx_open": "must be a closed solid",
    "bad_defect_not_triangulated": "not every polygon is a triangle",
    "bad_defect_no_slot": "has no material slot",
}


def run_case(name):
    status, fbx, extra = CASES[name]
    if name == "cli_preview_inside_repo" and os.path.exists(PREVIEW_TRAP):
        return [f"{PREVIEW_TRAP} exists already: delete it first"], ""
    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, VESTIGE_CASE=name)
        args = ["--export-dir", tmp] if not extra or extra[0] != "--exportdir" else []
        command = [BLENDER, "--background", "--factory-startup", "--python-exit-code", "1", "--python",
                   os.path.abspath(__file__), "--", *args, *extra]
        done = subprocess.run(command, env=env, capture_output=True, text=True, errors="replace")
        wrote = [f for f in os.listdir(tmp) if f.endswith(".fbx")]
    output = done.stdout + done.stderr
    problems = []
    if done.returncode != status:
        problems.append(f"exit status {done.returncode}, wanted {status}")
    if bool(wrote) != fbx:
        problems.append(f"FBX written: {bool(wrote)}, wanted {fbx}")
    if MESSAGES.get(name, "") not in output:
        problems.append(f"the output does not say {MESSAGES[name]!r}")
    if name == "cli_preview_inside_repo" and os.path.exists(PREVIEW_TRAP):
        shutil.rmtree(PREVIEW_TRAP)
        problems.append("previews were written into the checkout (the folder is deleted again): the preview-dir guard is broken")
    return problems, output[-1500:]


def palette_check():
    """No Blender needed: the water cue is far from every other palette colour."""
    sys.path.insert(0, ART)
    from vestige_art.palette import PALETTE

    def lab(hexc):
        def lin(c):
            c /= 255.0
            return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
        r, g, b = (lin(int(hexc[i:i + 2], 16)) for i in (0, 2, 4))
        x, y, z = (0.4124564 * r + 0.3575761 * g + 0.1804375 * b) / 0.95047, 0.2126729 * r + 0.7151522 * g + 0.0721750 * b, \
                  (0.0193339 * r + 0.1191920 * g + 0.9503041 * b) / 1.08883
        f = lambda t: t ** (1 / 3) if t > 216 / 24389 else (24389 / 27 * t + 16) / 116  # noqa: E731
        return 116 * f(y) - 16, 500 * (f(x) - f(y)), 200 * (f(y) - f(z))

    problems = []
    water = lab(PALETTE["Water"][0])
    for key, (hexc, _) in PALETTE.items():
        if key != "Water" and math.dist(water, lab(hexc)) < 30:
            problems.append(f"{key} {hexc} is within 30 (CIELAB, 1976) of the water cue")
    return problems


def main():
    wanted = sys.argv[1:] or list(CASES) + ["palette"]
    unknown = [name for name in wanted if name not in CASES and name != "palette"]
    if unknown:
        sys.exit(f"unknown case(s): {', '.join(unknown)}")
    failed = 0
    for name in wanted:
        problems, log = (palette_check(), "") if name == "palette" else run_case(name)
        print(f"{'FAIL' if problems else 'PASS'}  {name}  {'; '.join(problems)}")
        if problems:
            failed += 1
            print(log)
    print(f"\n{len(wanted) - failed} of {len(wanted)} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
