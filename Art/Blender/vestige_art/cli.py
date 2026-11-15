# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""The command line of an asset script: everything after the "--" that Blender leaves alone."""

import argparse
import os
import sys

# Art/Blender/vestige_art/cli.py -> the folder that holds Art/. When a script is copied into
# another tree (to try it without touching the repository), that tree is the root.
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
DEFAULT_EXPORT_DIR = os.path.join(ROOT, "Art", "Export")
BLEND_DIR = os.path.join(ROOT, "Art", "Blender")
# Git ignores Saved/ (.gitignore), so previews written there, for example into Saved/AgentWork/<task>/,
# can never be committed by mistake.
SAVED_DIR = os.path.join(ROOT, "Saved")


def script_args(argv=None):
    """The arguments after "--"; none if there is no "--"."""
    argv = sys.argv if argv is None else argv
    return argv[argv.index("--") + 1:] if "--" in argv else []


def current_script(fallback):
    """The file name after --python (the script Blender was started with), for the usage text."""
    if "--python" in sys.argv:
        index = sys.argv.index("--python") + 1
        if index < len(sys.argv):
            return os.path.basename(sys.argv[index])
    return fallback


def is_inside(path, folder):
    path = os.path.normcase(os.path.realpath(path))
    folder = os.path.normcase(os.path.realpath(folder))
    try:
        return os.path.commonpath([path, folder]) == folder
    except ValueError:  # another drive
        return False


def parse_args(asset_name, argv=None):
    """Parses the options. A misspelt or unknown option stops the run (exit status 2), because
    the default is to write into the repository and a typo must not do that silently."""
    script = current_script(f"make_{asset_name}.py")
    parser = argparse.ArgumentParser(
        prog=f"blender --background --python-exit-code 1 --python Art/Blender/{script} --",
        description=f"Builds {asset_name}, checks it and exports {asset_name}.fbx. "
                    "With no options it writes only Art/Export/<asset>.fbx.",
        allow_abbrev=False,
    )
    parser.add_argument("--export-dir", metavar="DIR", default=None,
                        help="where the FBX goes (default: Art/Export)")
    parser.add_argument("--save-blend", metavar="FILE", nargs="?", default=None,
                        const=os.path.join(BLEND_DIR, asset_name + ".blend"),
                        help="also save the .blend source (default file: Art/Blender/<asset>.blend). "
                             "Without this option no .blend is written.")
    parser.add_argument("--check-only", action="store_true",
                        help="build and check the asset, write nothing")
    parser.add_argument("--preview-dir", metavar="DIR", default=None,
                        help="render the preview images into DIR (a folder outside the repository, "
                             "or one inside its git-ignored Saved folder, for example Saved/AgentWork/<task>)")
    args = parser.parse_args(script_args(argv))

    if args.check_only and (args.export_dir or args.save_blend or args.preview_dir):
        parser.error("--check-only writes nothing, so it can't be combined with "
                     "--export-dir, --save-blend or --preview-dir")
    if args.preview_dir and is_inside(args.preview_dir, ROOT) and not is_inside(args.preview_dir, SAVED_DIR):
        parser.error(f"--preview-dir must be outside the repository ({ROOT}) or inside its Saved folder, "
                     "which Git ignores: preview images are for looking at, not for committing")
    args.export_dir = os.path.abspath(args.export_dir or DEFAULT_EXPORT_DIR)
    if args.save_blend:
        args.save_blend = os.path.abspath(args.save_blend)
    if args.preview_dir:
        args.preview_dir = os.path.abspath(args.preview_dir)
    return args
