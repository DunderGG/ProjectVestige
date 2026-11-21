# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds SM_Well, the farm's well: the clean water the watering can is filled from, and exports
Art/Export/SM_Well.fbx.

Run (from the project folder; Blender 5.0 is tested):
    blender --background --python-exit-code 1 --python Art/Blender/make_well.py
This writes only Art/Export/SM_Well.fbx. The options (--check-only, --export-dir, --save-blend,
--preview-dir) are the same as for make_bed.py; see Docs/Setup.md, section 11.
A broken asset stops the run with a non-zero exit status before anything is written.

The well is a round stone ring, 0.90 m across and 0.55 m high, with the water just under its rim, and a
teal-and-brass lever pump on its +X side that makes the whole well 1.30 m high (GameDesign.md, section 6
and the open question in section 15 on the first machines; the sizes and the look there are the recommended
options, assumed until the maintainer decides). The pump has a spout toward +X, a lever that reaches back
over the water, and a brass dial on its column. The dial is static: its azure ring is the water cue and it
will not move, since a moving gauge would need a per-instance material value and the water supply is not
settled; the interaction prompt gives the supply. Nothing is held and the well is used from any side (an
interaction volume around it, as for the bed), so it has no socket.
Pipeline (CONTRIBUTING.md, section 5): metres, Z up, front +X, the pivot on the floor in the middle of the
ring. Everything stays inside the 0.9 m circle, so the well fits a tile with 5 cm to spare.
The importer flips Y (Art/README.md, "Left and right"), but the pump, the spout and the lever are all in
the X-Z plane, so nothing here is side-specific.

Parts and materials (the slot names are the Unreal material instances the slots become):
    MI_Prop_Stone       the ring: two courses of 12 flat faces, the upper one and the coping turned half a face
                        so the joints stagger and the coping overhangs the upper course evenly. The faces stay
                        crisp (finish_object's angle is 26 degrees, under the 30 degrees at which the 12 faces
                        meet), so the ring reads as cut stone
    MI_Prop_PaintTeal   the pump's column and head
    MI_Prop_Brass       the dial's bezel, the spout and the lever's grip
    MI_Prop_DarkMetal   the pump's base plate, the lever's pivot and bar, and the dial's centre
    MI_Prop_Rust        a crust round the foot of the column: the machine's one big patch of wear
    MI_Prop_Water       the water in the ring and the ring on the dial: the water cue (palette.py)
Collision: UCX_SM_Well_00, the ring as a 12-sided hull turned with the coping (0.55 m high, over the farmer's
0.45 m step; the lower course's corners reach 1.5 cm past it at the floor, which nobody can feel), and
UCX_SM_Well_01, a box round the pump's base, column and head. The lever and the spout have none.
Nothing is textured and there is no UV map: every colour is a flat material.
"""

import math
import os
import sys

import bmesh
from mathutils import Matrix

sys.dont_write_bytecode = True  # no __pycache__ folder in the repository
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from vestige_art import geometry as geo  # noqa: E402
from vestige_art import materials  # noqa: E402
from vestige_art.run import run  # noqa: E402

NAME = "SM_Well"

# Sizes in metres, built from the floor up.
RING_RADIUS = 0.45                      # the coping's, so the well is 0.90 m across
COURSE_1_RADIUS, COURSE_1_TOP = 0.45, 0.26   # its flat sides (0.435) cover the upper course's corners
COURSE_2_RADIUS, COURSE_2_TOP = 0.43, 0.48
RING_TOP = 0.55                         # the coping's top
STAGGER_DEGREES = 15.0                  # the upper course and the coping, turned half a face
OPENING_RADIUS = 0.24                   # the hole in the coping
WATER_Z = 0.50                          # 5 cm under the rim, so the game camera sees most of the water
RING_SIDES = 12
PUMP_X = 0.30                           # the pump's column stands on the ring's +X side
PLATE_TOP = 0.59
RUST_TOP = 0.67                         # the rust crust round the column's foot
COLUMN_TOP = 0.99
HEAD_TOP = 1.13
DIAL_Z = 0.78
COLLISION_PUMP_TOP = 1.14


def polygon(radius, sides, start_degrees=0.0, centre=(0.0, 0.0)):
    """The points of a regular polygon, counter-clockwise, for add_prism."""
    return [(centre[0] + radius * math.cos(math.radians(start_degrees) + 2 * math.pi * k / sides),
             centre[1] + radius * math.sin(math.radians(start_degrees) + 2 * math.pi * k / sides))
            for k in range(sides)]


def add_disc(bm, outline, axis, at, mat):
    """A flat one-sided disc facing along +axis, 2 mm over `at`: the water's surface, the dial's face."""
    geo.add_prism(bm, outline, axis, at, at + 0.002, mat, skip=("lo", *range(len(outline))))


def build():
    keys = ("Stone", "PaintTeal", "Brass", "DarkMetal", "Rust", "Water")
    slots = materials.Slots(list(keys))
    stone, teal, brass, dark, rust, water = (slots[k] for k in keys)
    bm = bmesh.new()

    # The ring. The two courses are solid under the water (nobody sees inside them), so they are only
    # their outer walls: the upper course's top is covered by the coping and the water, and no bottom
    # is drawn. The lower course keeps its top, which shows as a ledge where it is wider than the upper
    # one. The coping is a ring: its inner wall shows above the water.
    geo.add_prism(bm, polygon(COURSE_1_RADIUS, RING_SIDES), "Z", 0.0, COURSE_1_TOP, stone, skip=("lo",))
    geo.add_prism(bm, polygon(COURSE_2_RADIUS, RING_SIDES, start_degrees=STAGGER_DEGREES), "Z", COURSE_1_TOP,
                  COURSE_2_TOP, stone, skip=("lo", "hi"))
    # The coping is turned like the upper course, so it overhangs that course by an even 2 cm all round
    # (0.45 - 0.43) and its shadow on the course is a level band, not a sawtooth.
    coping = geo.add_ring(bm, (0.0, 0.0, COURSE_2_TOP), OPENING_RADIUS, RING_RADIUS, RING_TOP - COURSE_2_TOP,
                          RING_SIDES, stone, cap_bottom=False)
    bmesh.ops.rotate(bm, verts=list({v for f in coping for v in f.verts}), cent=(0.0, 0.0, 0.0),
                     matrix=Matrix.Rotation(math.radians(STAGGER_DEGREES), 3, "Z"))

    # The water: a one-sided disc a little wider than the opening, so its edge is inside the coping.
    add_disc(bm, polygon(OPENING_RADIUS + 0.015, RING_SIDES), "Z", WATER_Z, water)

    # The pump: a base plate on the coping, a tapering column, a head, a spout toward +X.
    geo.add_box(bm, (0.235, -0.10, RING_TOP), (0.43, 0.10, PLATE_TOP), dark, bevel=0.008, skip=("-z",))
    # Rust: a crust round the column's foot, where the spilt water stands (GameDesign.md, section 12: wear and
    # rust are big shapes of their own faces). Eight sides and a wider base, so it reads as a lumpy crust, not
    # a machined collar; the column starts on its top.
    geo.add_cylinder(bm, (PUMP_X, 0.0, PLATE_TOP), 0.082, RUST_TOP - PLATE_TOP, 8, rust, top_radius=0.068,
                     cap_bottom=False)
    column_foot_radius = 0.065 - 0.010 * (RUST_TOP - PLATE_TOP) / (COLUMN_TOP - PLATE_TOP)
    geo.add_cylinder(bm, (PUMP_X, 0.0, RUST_TOP), column_foot_radius, COLUMN_TOP - RUST_TOP, 16, teal,
                     top_radius=0.055, cap_bottom=False, cap_top=False)
    geo.add_box(bm, (PUMP_X - 0.07, -0.08, COLUMN_TOP), (PUMP_X + 0.07, 0.08, HEAD_TOP), teal, bevel=0.015)
    geo.add_tube(bm, [(0.32, 0.0, 1.06), (0.385, 0.0, 1.06), (0.43, 0.0, 1.02)], [0.034, 0.032, 0.028], 8, brass,
                 caps=(False, True))

    # The dial on the column's +X side: a brass bezel, then the azure ring of the water cue, then a dark centre.
    # The two discs are one-sided and a hair apart, so they never fight over the same plane.
    geo.add_cylinder(bm, (PUMP_X + 0.02, 0.0, DIAL_Z), 0.08, 0.06, 16, brass, axis="X", cap_bottom=False)
    face = PUMP_X + 0.02 + 0.06
    add_disc(bm, polygon(0.058, 16, centre=(0.0, DIAL_Z)), "X", face, water)
    add_disc(bm, polygon(0.03, 8, centre=(0.0, DIAL_Z)), "X", face + 0.002, dark)

    # The lever: a pivot pin through the head, a dark bar rising back over the water, a brass grip.
    geo.add_cylinder(bm, (PUMP_X - 0.01, -0.105, 1.10), 0.028, 0.21, 8, dark, axis="Y")
    geo.add_tube(bm, [(PUMP_X - 0.01, 0.0, 1.10), (0.02, 0.0, 1.22)], 0.022, 8, dark, caps=(False, False))
    geo.add_tube(bm, [(0.02, 0.0, 1.22), (-0.16, 0.0, 1.27)], 0.03, 8, brass)

    asset = geo.finish_object(NAME, bm, slots, shade="angle", angle=26.0)

    # Collision: the ring as a hull of the coping's 12-sided outline (turned like the coping), and a box round
    # the pump. A hull round the lower course's corners as well would need 48 points, over the 32 allowed.
    outline = polygon(RING_RADIUS, RING_SIDES, start_degrees=STAGGER_DEGREES)
    geo.add_collision_hull(asset, [(x, y, z) for x, y in outline for z in (0.0, RING_TOP)])
    geo.add_collision_box(asset, (PUMP_X - 0.07, -0.10, RING_TOP), (0.44, 0.10, COLLISION_PUMP_TOP))
    return asset


if __name__ == "__main__":
    run(NAME, build, "prop", size=(0.9, 0.9, 1.3), size_tol=0.01)
