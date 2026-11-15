# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds SM_Bed, the single bed the player sleeps in, and exports Art/Export/SM_Bed.fbx.

Run (from the project folder; Blender 5.0 is tested):
    blender --background --python-exit-code 1 --python Art/Blender/make_bed.py
This writes only Art/Export/SM_Bed.fbx. Options go after a double dash:
    -- --check-only                  build and check it, write nothing
    -- --export-dir DIR              write the FBX into DIR instead of Art/Export
    -- --save-blend [FILE]           also save the .blend source (default Art/Blender/SM_Bed.blend)
    -- --preview-dir DIR             render the preview images into DIR (outside the repository, or in
                                     its git-ignored Saved folder, for example Saved/AgentWork/bed)
A broken asset stops the run with a non-zero exit status before anything is written.

The bed is 2.00 m long (X) and 1.00 m wide (Y), the footprint of the placeholder AVestigeBed draws
and the one its interaction box (20 cm bigger all round) is made for; keep it, or change the game
side with it. It is 0.90 m high at the top of the headboard.
Pipeline (CONTRIBUTING.md, section 5): metres, Z up, front +X. The pivot is on the floor in the
middle of the footprint (so the actor's origin is the floor under the bed, as the placeholder's is).
The foot of the bed, with the low footboard, faces +X; the head, with the tall arched headboard,
is at -X. Sleeping is instant in the game, so the bed has no "side the player gets in from".
Which side is which in Unreal: the importer flips Y (checked on the imported potato: Blender y of
-0.2245 m is Unreal y of +22.45 cm; X and Z stay), so this file's -Y side is the +Y side there. In
L_Farm the bed has yaw 90 degrees, which turns its +Y side toward world -X, the side of the farmer's
start. The mending patch is on the blanket's -Y half for that reason, and the GameSide preview is
rendered from -Y.

Parts and materials (the slot names are the Unreal material instances the slots become):
    MI_Prop_PaintTeal       frame and boards (the watering can's body colour, shared by the machines)
    MI_Prop_DarkMetal       legs and the dial's centre
    MI_Prop_Brass           the dial on the headboard (the watering can's brass)
    MI_Prop_BedSheet        mattress, pillow and the folded-back cuff
    MI_Prop_BedBlanket      the blanket
    MI_Prop_BedBlanketBand  the band across the foot of the blanket, and the mending patch on it
Collision: UCX_SM_Bed_00 (the body, up to the top of the blanket) and UCX_SM_Bed_01 (the headboard).
Nothing is textured and there is no UV map: every colour is a flat material. Parts that touch leave
out the face they share, but the chamfers at the joints, and a face that a neighbour only partly
covers (the frame's top under the mattress, the brass dial under its dark centre), stay: about 28 of
the 694 triangles (4 %) cannot be seen from outside (counted by casting rays from each triangle's
centre). That is cheap, and the parts stay separate, so one can be changed on its own.
"""

import math
import os
import sys

import bmesh

sys.dont_write_bytecode = True  # no __pycache__ folder in the repository
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from vestige_art import geometry as geo  # noqa: E402
from vestige_art import materials  # noqa: E402
from vestige_art.run import run  # noqa: E402

NAME = "SM_Bed"

# Sizes in metres. The bed is built from the floor up; X is the length, the head is at -X.
HALF_LENGTH, HALF_WIDTH = 1.00, 0.50    # the footprint, 2.00 x 1.00 m
LEG_HEIGHT = 0.14
FRAME_TOP = 0.30                         # the top of the frame
MATTRESS_TOP = 0.48
LINEN_TOP = 0.53                         # the top of the blanket
BOARD_INNER = 0.88                       # |x| of the inner face of the head and foot boards
FOOTBOARD_TOP = 0.62
HEADBOARD_SIDE_TOP, HEADBOARD_PEAK = 0.76, 0.90   # the arch rises 14 cm over the width
COLLISION_TOP = 0.55                     # above the blanket's 0.53; higher than the farmer steps up (0.45)


def arch_height(y, radius, centre_z):
    return centre_z + math.sqrt(radius * radius - y * y)


def build():
    slots = materials.Slots(["PaintTeal", "DarkMetal", "Brass", "BedSheet", "BedBlanket", "BedBlanketBand"])
    teal, dark, brass = slots["PaintTeal"], slots["DarkMetal"], slots["Brass"]
    sheet, blanket, band = slots["BedSheet"], slots["BedBlanket"], slots["BedBlanketBand"]
    bm = bmesh.new()

    # Four tapered legs, open at both ends: the floor and the frame cover them.
    for x in (-0.82, 0.82):
        for y in (-0.38, 0.38):
            geo.add_cylinder(bm, (x, y, 0.0), 0.04, LEG_HEIGHT, 12, dark, top_radius=0.06,
                             cap_bottom=False, cap_top=False)

    # The frame: a chunky chamfered slab that sits on the legs.
    geo.add_box(bm, (-HALF_LENGTH, -HALF_WIDTH, LEG_HEIGHT), (HALF_LENGTH, HALF_WIDTH, FRAME_TOP), teal, bevel=0.02)

    # The headboard: a slab with an arched top, standing on the frame at the -X end.
    sagitta = HEADBOARD_PEAK - HEADBOARD_SIDE_TOP
    radius = (HALF_WIDTH ** 2 + sagitta ** 2) / (2 * sagitta)
    centre_z = HEADBOARD_PEAK - radius
    half_angle = math.asin(HALF_WIDTH / radius)
    arc_count = 8
    arc = [(radius * math.sin(a), centre_z + radius * math.cos(a))
           for a in (half_angle - 2 * half_angle * i / arc_count for i in range(arc_count + 1))]
    outline = [(-HALF_WIDTH, FRAME_TOP), (HALF_WIDTH, FRAME_TOP)] + arc   # (y, z), counter-clockwise from +X
    geo.add_prism(bm, outline, "X", -HALF_LENGTH, -BOARD_INNER, teal, skip=(0,))   # edge 0 is the bottom, on the frame

    # The brass dial on the headboard's front, with a dark centre.
    geo.add_cylinder(bm, (-BOARD_INNER, 0.0, 0.70), 0.09, 0.03, 16, brass, axis="X", cap_bottom=False)
    geo.add_cylinder(bm, (-BOARD_INNER + 0.03, 0.0, 0.70), 0.05, 0.02, 16, dark, axis="X", cap_bottom=False)

    # The footboard: lower, chamfered.
    geo.add_box(bm, (BOARD_INNER, -HALF_WIDTH, FRAME_TOP), (HALF_LENGTH, HALF_WIDTH, FOOTBOARD_TOP), teal,
                bevel=0.03, skip=("-z",))

    # The mattress at the head. Its ends are against the headboard and the cuff, and its bottom on the frame.
    geo.add_box(bm, (-BOARD_INNER, -0.45, FRAME_TOP), (-0.10, 0.45, MATTRESS_TOP), sheet,
                bevel=0.03, skip=("-z", "-x", "+x"))

    # The pillow: a soft, rounded slab. Three segments on each edge turn it 30 degrees at a step, under the
    # 35 degree smoothing limit, so it shades round; two (45 degrees) read as a chamfered slab.
    geo.add_box(bm, (-0.80, -0.32, MATTRESS_TOP), (-0.40, 0.32, MATTRESS_TOP + 0.12), sheet,
                bevel=0.05, segments=3, skip=("-z",))

    # The blanket over the foot half: the folded-back cuff, the body and a band across the foot,
    # side by side, each as wide as the mattress plus 4 cm, hanging down to the frame.
    for x_lo, x_hi, mat, skip in (
        (-0.10, 0.05, sheet, ("-z", "+x")),
        (0.05, 0.74, blanket, ("-z", "-x", "+x")),
        (0.74, BOARD_INNER, band, ("-z", "-x", "+x")),
    ):
        geo.add_box(bm, (x_lo, -0.49, FRAME_TOP), (x_hi, 0.49, LINEN_TOP), mat, bevel=0.03, skip=skip)

    # A mending patch sewn on the blanket: the caretaker robot has kept the bed made for decades. It is
    # on the -Y side (y -0.33 to -0.11) on purpose: that is Unreal's +Y side, which L_Farm's yaw 90
    # turns toward the farmer's start (see the top of this file).
    geo.add_box(bm, (0.30, -0.33, LINEN_TOP), (0.52, -0.11, LINEN_TOP + 0.015), band, bevel=0.006, skip=("-z",))

    asset = geo.finish_object(NAME, bm, slots, shade="angle")

    # Collision for the player to bump into: the body up to the blanket, and the headboard on top of
    # it (the arch, as a hull of a few points).
    geo.add_collision_box(asset, (-HALF_LENGTH, -HALF_WIDTH, 0.0), (HALF_LENGTH, HALF_WIDTH, COLLISION_TOP))
    hull = []
    for x in (-HALF_LENGTH, -BOARD_INNER):
        hull += [(x, -HALF_WIDTH, COLLISION_TOP), (x, HALF_WIDTH, COLLISION_TOP)]
        hull += [(x, y, arch_height(y, radius, centre_z)) for y in (-0.5, -0.25, 0.0, 0.25, 0.5)]
    geo.add_collision_hull(asset, hull)
    return asset


if __name__ == "__main__":
    run(NAME, build, "prop", size=(2.0, 1.0, None), size_tol=0.005)
