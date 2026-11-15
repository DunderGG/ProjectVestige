# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Shared helpers for the Blender scripts that build Project Vestige's meshes (make_bed.py,
make_shipping_bin.py, make_well.py, make_sprinkler.py and the scripts after them).
make_farm_assets.py is older and does not use them yet.

A script adds its own folder to sys.path, builds one asset and hands it to run():

    import os, sys
    sys.dont_write_bytecode = True
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from vestige_art import geometry as geo, materials
    from vestige_art.run import run

Modules:
    cli        the command line after "--" (--export-dir, --save-blend, --check-only, --preview-dir)
    palette    the colours the machines and furniture share (no Blender in it, so the editor can read it)
    materials  Blender 5.0 safe materials made from the palette, and the slots of one mesh
    geometry   box (also tapered), prism, cylinder, ring, tube and dome primitives; finish_object (weld,
               triangulate, shading, no UV map); UCX_ collision shapes
    validate   the gate before the export: raises, and Blender then exits with a non-zero status
    export     the FBX with the project's settings (the UCX_ collision included), and the .blend
    preview    the game camera views (from the foot and from the side) and three close views, with a
               1 m tile for scale
    run        one call that does all of the above

Not built yet, add it with the first script that needs it: cones, UV maps, SOCKET_ empties (the grip
of a held tool), and the crop and tool kinds of validate (their pivot and collision rules).
All sizes are in metres. Nothing here writes a file unless the command line asks for it.
"""
