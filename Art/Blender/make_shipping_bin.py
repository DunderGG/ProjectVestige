# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Builds SM_ShippingBin, the bin the player puts crops in to be sold overnight, and exports
Art/Export/SM_ShippingBin.fbx.

Run (from the project folder; Blender 5.0 is tested):
    blender --background --python-exit-code 1 --python Art/Blender/make_shipping_bin.py
This writes only Art/Export/SM_ShippingBin.fbx. The options (--check-only, --export-dir,
--save-blend, --preview-dir) are the same as for make_bed.py; see Docs/Setup.md, section 11.
A broken asset stops the run with a non-zero exit status before anything is written.

The bin is a hip-high teal painted-metal hopper: 0.90 m by 0.90 m on the ground and 0.90 m high, which
leaves 5 cm to the edge of its 1 m farm tile (GameDesign.md, section 6 and the open question in section
15 on the first machines; the sizes and the look there are the recommended options, assumed until the
maintainer decides). It stands on four dark-metal feet. The lower body is straight; above it the hopper
flares out to the lid, which is hinged at the back (-X, a dark-metal hinge barrel) and shut: the bin
shows no state in the slice. A brass pull sits on the lid's front edge. The front (+X) carries a brass
gear plate, with no lettering until the writer decides what the emblem means; a stepped rust patch
wraps the lower front corner. Nothing is held and the bin is used from any side (an interaction volume
around it, as for the bed), so it has no socket.
Pipeline (CONTRIBUTING.md, section 5): metres, Z up, front +X, the pivot on the floor in the middle of
the footprint.
Which side is which in Unreal: the importer flips Y (Art/README.md, "Left and right"), so this file's
-Y side is the +Y side there. The rust patch is on the front's -Y corner for that reason: it ends up on
Unreal's +Y/+X low corner; turn the actor in the level to taste. The gear plate is in the middle and the
lid is the same left and right, so nothing else is side-specific.

Parts and materials (the slot names are the Unreal material instances the slots become):
    MI_Prop_PaintTeal   the body, the hopper, the lid and its raised panel
    MI_Prop_DarkMetal   the feet, the hinge barrel and the gear's hub
    MI_Prop_Brass       the gear plate and the lid's pull
    MI_Prop_Rust        the patch on the lower front corner
Collision: UCX_SM_ShippingBin_00, one solid box 0.9 x 0.9 x 0.9 m (the farmer steps up 0.45 m, so it
can't be climbed; a box over the whole footprint is cheap and the lid overhangs the body anyway).
Nothing is textured and there is no UV map: every colour is a flat material.
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

NAME = "SM_ShippingBin"

# Sizes in metres. The bin is built from the floor up; X is the front (+X) to the back.
HALF = 0.45                          # the footprint, 0.90 x 0.90 m, is the lid's
HEIGHT = 0.90                        # the top of the lid's pull
FOOT_CENTRE, FOOT_HALF, FOOT_TOP = 0.27, 0.07, 0.10
BODY_HALF = 0.36                     # the straight lower body, from the feet to BODY_TOP
BODY_TOP = 0.50
HOPPER_TOP = 0.78                    # the hopper flares from BODY_HALF to BODY_HALF + HOPPER_FLARE
HOPPER_FLARE = 0.08
LID_TOP = 0.84
PANEL_HALF, PANEL_TOP = 0.30, 0.875
GEAR_CENTRE_Z = 0.30
GEAR_TEETH, GEAR_ROOT, GEAR_TIP = 8, 0.115, 0.15


def gear_outline(teeth, root, tip, centre_z):
    """The outline of a gear in the (y, z) plane, counter-clockwise as seen from +X, four points a tooth:
    the base and the tip of its left side, then the tip and the base of its right side."""
    pitch = 2 * math.pi / teeth
    points = []
    for k in range(teeth):
        angle = k * pitch
        for turn, radius in ((-0.22, root), (-0.11, tip), (0.11, tip), (0.22, root)):
            a = angle + turn * pitch
            points.append((radius * math.cos(a), centre_z + radius * math.sin(a)))
    return points


def build():
    slots = materials.Slots(["PaintTeal", "DarkMetal", "Brass", "Rust"])
    teal, dark, brass, rust = slots["PaintTeal"], slots["DarkMetal"], slots["Brass"], slots["Rust"]
    bm = bmesh.new()

    # Four plain dark-metal feet, under the corners of the body.
    for x in (-FOOT_CENTRE, FOOT_CENTRE):
        for y in (-FOOT_CENTRE, FOOT_CENTRE):
            geo.add_box(bm, (x - FOOT_HALF, y - FOOT_HALF, 0.0), (x + FOOT_HALF, y + FOOT_HALF, FOOT_TOP),
                        dark, skip=("-z",))

    # The lower body and the hopper above it. Where they meet, their chamfers make a seam.
    geo.add_box(bm, (-BODY_HALF, -BODY_HALF, FOOT_TOP), (BODY_HALF, BODY_HALF, BODY_TOP), teal,
                bevel=0.02, skip=("-z", "+z"))
    geo.add_box(bm, (-BODY_HALF, -BODY_HALF, BODY_TOP), (BODY_HALF, BODY_HALF, HOPPER_TOP), teal,
                bevel=0.02, skip=("-z", "+z"), taper=-HOPPER_FLARE)

    # The lid, shut, with a raised panel. Its hinge barrel is along the back edge (-X); the lid has no
    # front or side that differs, so it reads the same from every side.
    geo.add_box(bm, (-HALF, -HALF, HOPPER_TOP), (HALF, HALF, LID_TOP), teal, bevel=0.02, skip=("-z",))
    geo.add_box(bm, (-PANEL_HALF, -PANEL_HALF, LID_TOP), (PANEL_HALF, PANEL_HALF, PANEL_TOP), teal,
                bevel=0.01, skip=("-z",))
    geo.add_cylinder(bm, (-HALF + 0.03, -0.24, LID_TOP), 0.03, 0.48, 12, dark, axis="Y")
    # The brass pull on the front edge of the lid, the highest point of the bin.
    geo.add_box(bm, (0.33, -0.12, LID_TOP), (0.41, 0.12, HEIGHT), brass, bevel=0.01, skip=("-z",))

    # The brass gear plate on the front (+X) of the lower body, with a dark hub: no lettering.
    face = BODY_HALF
    geo.add_prism(bm, gear_outline(GEAR_TEETH, GEAR_ROOT, GEAR_TIP, GEAR_CENTRE_Z), "X", face - 0.005, face + 0.025,
                  brass, skip=("lo",))
    geo.add_cylinder(bm, (face + 0.025, 0.0, GEAR_CENTRE_Z), 0.045, 0.012, 12, dark, axis="X", cap_bottom=False)

    # The rust patch on the lower front corner (this file's +X, -Y: see the top). Three L-shaped steps round
    # the corner, each a little proud of the body and sunk into it a little, and each with shorter arms than the
    # one below, so no step has a bottom to draw. The arms are different lengths on the two faces: a stepped,
    # ragged shape reads as a worn patch from the game camera, and costs 16 triangles a step.
    out, inn = BODY_HALF + 0.012, BODY_HALF - 0.02
    steps = (  # z from, z to, where the arm along +X's face ends (y), where the arm along -Y's face ends (x)
        (FOOT_TOP + 0.005, 0.19, -0.08, 0.16),
        (0.19, 0.25, -0.20, 0.24),
        (0.25, 0.30, -0.28, 0.30),
    )
    for z_lo, z_hi, along_y, along_x in steps:
        outline = [(out, -out), (out, along_y), (inn, along_y), (inn, -inn), (along_x, -inn), (along_x, -out)]
        geo.add_prism(bm, outline, "Z", z_lo, z_hi, rust, skip=("lo",))

    asset = geo.finish_object(NAME, bm, slots, shade="angle")

    # Collision: one solid box over the footprint, the whole height.
    geo.add_collision_box(asset, (-HALF, -HALF, 0.0), (HALF, HALF, HEIGHT))
    return asset


if __name__ == "__main__":
    run(NAME, build, "prop", size=(0.9, 0.9, 0.9), size_tol=0.005)
