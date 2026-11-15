# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds SM_Sprinkler, the machine the player places on the farm grid to water the tiles round it, and
exports Art/Export/SM_Sprinkler.fbx.

Run (from the project folder; Blender 5.0 is tested):
    blender --background --python-exit-code 1 --python Art/Blender/make_sprinkler.py
This writes only Art/Export/SM_Sprinkler.fbx. The options (--check-only, --export-dir, --save-blend,
--preview-dir) are the same as for make_bed.py; see Docs/Setup.md, section 11.
A broken asset stops the run with a non-zero exit status before anything is written.

The sprinkler is a squat teal pedestal on a round dark-metal plate, with a brass rotary head of two short
arms and a small dome on top: 0.40 m wide and 0.40 m high (GameDesign.md, section 6 and the open question
in section 15 on the first machines; the sizes and the look there are the recommended options, assumed until
the maintainer decides). It is low on purpose: from the camera, 30 degrees down, an object hides as much
ground behind it as its height divided by the tangent of 30 degrees, so 0.4 m hides about 0.7 m and the
next tile's centre stays in view, while 0.6 m would hide about 1.04 m of the next tile.
The sprinkler is the same from every side (its range, 3 by 3 tiles round it, has no front), so nothing
here is side-specific, the pivot is on the floor in the middle of the plate, and there is no socket: the
placement ghost uses this same mesh and the player never holds it.
Pipeline (CONTRIBUTING.md, section 5): metres, Z up, front +X (it has none to speak of).

Parts and materials (the slot names are the Unreal material instances the slots become):
    MI_Prop_DarkMetal   the plate
    MI_Prop_PaintTeal   the pedestal
    MI_Prop_Brass       the head and its two arms
    MI_Prop_Water       the dome on top, and nothing else: the water cue (palette.py). It is a slot of
                        its own so the dome can later show that the sprinkler will run, with no new mesh.
Collision: UCX_SM_Sprinkler_00, one solid box 0.4 x 0.4 x 0.5 m, taller than the mesh on purpose: the
farmer steps up 0.45 m (make_bed.py), so a box under that height could be climbed. The tiles around
it have no collision, so the box never traps the farmer.
Nothing is textured and there is no UV map: every colour is a flat material.
"""

import os
import sys

import bmesh

sys.dont_write_bytecode = True  # no __pycache__ folder in the repository
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from vestige_art import geometry as geo  # noqa: E402
from vestige_art import materials  # noqa: E402
from vestige_art.run import run  # noqa: E402

NAME = "SM_Sprinkler"

# Sizes in metres, built from the floor up.
PLATE_RADIUS, PLATE_TOP = 0.20, 0.04              # the footprint, 0.40 m across
PEDESTAL_RADIUS, PEDESTAL_TOP_RADIUS, PEDESTAL_TOP = 0.13, 0.09, 0.24
HEAD_RADIUS, HEAD_TOP = 0.07, 0.33
DOME_RADIUS = 0.07                                # the top of the dome is at HEAD_TOP + DOME_RADIUS = 0.40 m
ARM_Z = 0.285                                     # the middle of the head
COLLISION_TOP = 0.50                              # above the 0.4 m mesh, and above the farmer's 0.45 m step
SIDES = 12                                        # round parts: 12 sides, smooth under the 35 degree limit


def build():
    slots = materials.Slots(["DarkMetal", "PaintTeal", "Brass", "Water"])
    dark, teal, brass, water = slots["DarkMetal"], slots["PaintTeal"], slots["Brass"], slots["Water"]
    bm = bmesh.new()

    # The plate and the pedestal, each standing on the one below, so there is no bottom to draw.
    geo.add_cylinder(bm, (0.0, 0.0, 0.0), PLATE_RADIUS, PLATE_TOP, SIDES, dark, cap_bottom=False)
    geo.add_cylinder(bm, (0.0, 0.0, PLATE_TOP), PEDESTAL_RADIUS, PEDESTAL_TOP - PLATE_TOP, SIDES, teal,
                     top_radius=PEDESTAL_TOP_RADIUS, cap_bottom=False)

    # The brass head, open at both ends: the pedestal is under it and the dome closes it.
    geo.add_cylinder(bm, (0.0, 0.0, PEDESTAL_TOP), HEAD_RADIUS, HEAD_TOP - PEDESTAL_TOP, SIDES, brass,
                     cap_bottom=False, cap_top=False)

    # Two arms, one each way along Y, tapering a little and lifting at the tip. Their inner ends sit inside
    # the head, so they have no cap there. The tips stay inside the plate's 0.2 m radius.
    for side in (-1, 1):
        points = [(0.0, side * 0.05, ARM_Z), (0.0, side * 0.13, ARM_Z), (0.0, side * 0.19, ARM_Z + 0.015)]
        geo.add_tube(bm, points, [0.026, 0.024, 0.018], 8, brass, caps=(False, True))

    # The dome, in the water cue. Same 12 sides as the head, so its rim welds to the head's top edge.
    geo.add_dome(bm, (0.0, 0.0, HEAD_TOP), DOME_RADIUS, SIDES, 3, water)

    asset = geo.finish_object(NAME, bm, slots, shade="angle")
    geo.add_collision_box(asset, (-PLATE_RADIUS, -PLATE_RADIUS, 0.0), (PLATE_RADIUS, PLATE_RADIUS, COLLISION_TOP))
    return asset


if __name__ == "__main__":
    run(NAME, build, "prop", size=(0.4, 0.4, 0.4), size_tol=0.005)
