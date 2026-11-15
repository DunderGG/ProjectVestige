# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""run(): the whole job of an asset script, in the order that keeps it safe.

    parse the command line -> empty scene -> build -> validate -> (check-only: stop) -> export the
    FBX -> save the .blend if asked -> render previews if asked

Any error stops the run with exit status 1 (a bad command line with 2), as long as Blender was
started with --python-exit-code 1; run() also exits with 1 itself, so a failed run never looks
like a success.
"""

import sys
import traceback

import bpy

from . import cli, export, preview, validate


def reset_scene():
    """An empty scene in metric units, scale 1.0. It wipes everything, so it only runs headless."""
    if not bpy.app.background:
        raise RuntimeError("run this script with blender --background: it empties the open file")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    units = bpy.context.scene.unit_settings
    units.system = "METRIC"
    units.scale_length = 1.0


def run(name, build, kind, **checks):
    """Builds, checks and exports one asset.

    name    the Unreal asset name, SM_Bed: also the FBX name, the main object and the default .blend.
    build   a function with no arguments that returns the geometry.Asset (after reset_scene()).
    kind    "prop" or "building"; see validate.validate_asset.
    checks  the other options of validate.validate_asset (size, size_tol, pivot_tol, collision,
            budget_reason).
    """
    args = cli.parse_args(name)
    try:
        reset_scene()
        asset = build()
        if asset.main.name != name:
            raise validate.ValidationError(f"the script builds {asset.main.name} but is run as {name}")
        validate.validate_asset(asset, kind, **checks)
        if args.check_only:
            print(f"[run] {name}: checked, nothing written (--check-only)")
            return
        export.export_fbx(asset, args.export_dir)
        if args.save_blend:
            export.save_blend(args.save_blend)
        if args.preview_dir:
            preview.render_previews(asset, args.preview_dir)
        print(f"[run] {name}: done")
    except Exception:  # noqa: BLE001 - report every kind of failure the same way
        traceback.print_exc()
        print(f"[run] {name}: FAILED", file=sys.stderr)
        sys.stdout.flush()
        sys.exit(1)
