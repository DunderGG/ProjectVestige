# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Writing the files: the FBX for Unreal and, only when asked, a .blend file to open in Blender."""

import os

import bpy


def export_fbx(asset, export_dir):
    """Writes <export_dir>/<Mesh>.fbx with the project's settings (CONTRIBUTING.md, section 5):
    Forward X, Up Z, Apply Scalings FBX Units Scale, the file named like the Unreal asset. The
    selection is the main mesh together with its UCX_ shapes, so Unreal gets the collision from the
    file instead of making its own. Returns the path.

    Other settings that matter, written out so a person exporting by hand can match them: mesh and
    empties only (the empties are for SOCKET_ objects, which no script makes yet), no animation, no
    leaf bones, no custom properties, no vertex colours, no tangents (Unreal makes them), smoothing
    by face for flat-shaded meshes and by edge for smooth-by-angle ones, and Blender's default
    global scale of 1.0. The exporter writes the time into the file header, so two runs give the
    same mesh but not the same bytes: compare the geometry fingerprint run() prints, and commit the
    FBX only when the asset changed.
    """
    os.makedirs(export_dir, exist_ok=True)
    path = os.path.join(export_dir, asset.main.name + ".fbx")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in asset.objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = asset.main
    result = bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH", "EMPTY"},
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="X",
        axis_up="Z",
        mesh_smooth_type=asset.smooth_type,
        use_mesh_modifiers=True,
        use_custom_props=False,
        colors_type="NONE",
        use_tspace=False,
        add_leaf_bones=False,
        bake_anim=False,
    )
    bpy.ops.object.select_all(action="DESELECT")
    if "FINISHED" not in result or not os.path.isfile(path):
        raise RuntimeError(f"the FBX export did not finish: {result}")
    print(f"[export] wrote {path} ({os.path.getsize(path)} bytes)")
    return path


def save_blend(path):
    """Saves the scene as a .blend file. Only called when --save-blend is given."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=path)
    print(f"[export] wrote {path} ({os.path.getsize(path)} bytes)")
    return path
