# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""The colours the machines and furniture share. No Blender in here, so the editor's Python can read
the same table (sys.path.append("<project>/Art/Blender"), then "from vestige_art.palette import
PALETTE") when it makes the Unreal material instances: the colours then live in one place.

The Blender material name is the name of the Unreal material instance the slot gets, so the editor
import step can match slot to instance by name (the same idea as MI_Crop_<Crop><Part> for crops, see
Docs/Systems/22-FarmFields.md): "MI_Prop_" + the palette key. Several assets can use one key, so a
worn teal paint is one instance in the game. A colour only one asset uses is named after it
(BedSheet).
"""

PREFIX = "MI_Prop_"

# key: (sRGB hex, roughness). Colours are flat: no textures. Wear and rust are big shapes made
# of their own faces, in these materials, not painted detail (GameDesign.md, section 12).
PALETTE = {
    # Shared by the machines and furniture.
    "PaintTeal": ("2F7F7A", 0.45),   # painted metal; the watering can's body colour
    "Brass": ("B08D57", 0.35),       # trim, dials; the watering can's brass
    "DarkMetal": ("3A3F45", 0.55),   # legs, frames, inner parts
    "Rust": ("A4552B", 0.90),        # worn patches
    "Stone": ("9A9384", 0.90),       # the well's masonry: a warm grey, not blue-grey (the water's colour is blue)
    # The water cue (GameDesign.md, section 15). Nothing else may use this colour, not paint, trim, a
    # crop or a light: the player reads it as "clean water". It is a slot of its own on a mesh, so
    # the colour is changed in one place.
    "Water": ("58AEFF", 0.15),
    # Reserved, not added until a mesh shows it: the contamination cue, raspberry-magenta D63A9C
    # (GameDesign.md, section 15). It too is used for nothing else.
    # The bed.
    "BedSheet": ("E9DEC4", 0.90),
    "BedBlanket": ("C8803A", 0.90),
    "BedBlanketBand": ("93402F", 0.90),
}


def srgb_to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_to_linear(hex_color):
    return [srgb_to_linear(int(hex_color[i:i + 2], 16) / 255.0) for i in (0, 2, 4)]
