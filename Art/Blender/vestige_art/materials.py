# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Flat-colour Blender materials made from the shared palette (palette.py) and the slots of a mesh."""

import bpy

from .palette import PALETTE, PREFIX, hex_to_linear


def make_material(name, hex_color, roughness):
    """A flat Principled BSDF material. Works on Blender 4.x and 5.x: from 5.0 a new material
    already has its node tree and Material.use_nodes is deprecated."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    if bpy.app.version < (5, 0, 0):
        mat.use_nodes = True
    rgb = hex_to_linear(hex_color)
    mat.diffuse_color = (*rgb, 1.0)
    mat.roughness = roughness
    tree = mat.node_tree
    bsdf = tree.nodes.get("Principled BSDF") if tree else None
    if bsdf is None:
        raise RuntimeError(f"material {name}: no 'Principled BSDF' node (Blender {bpy.app.version_string})")
    bsdf.inputs["Base Color"].default_value = (*rgb, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    return mat


def palette_material(key):
    if key not in PALETTE:
        raise KeyError(f"'{key}' is not in the palette ({', '.join(PALETTE)}); add it to vestige_art/palette.py")
    hex_color, roughness = PALETTE[key]
    return make_material(PREFIX + key, hex_color, roughness)


class Slots:
    """The material slots of one mesh, in order. slots["Brass"] is the index to give a face."""

    def __init__(self, keys):
        self.keys = list(keys)
        if len(set(self.keys)) != len(self.keys):
            raise ValueError(f"a slot is listed twice: {self.keys}")
        self.materials = [palette_material(key) for key in self.keys]

    def __getitem__(self, key):
        return self.keys.index(key)
