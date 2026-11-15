# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Primitives for building a mesh with bmesh, and the step that turns the bmesh into an object.

Every primitive takes the bmesh first and a material slot index (slots["Brass"], see
materials.Slots), adds its vertices and faces with outward-facing windings and returns its faces.
Sizes are in metres; Z is up and the asset's front is +X.

A part that rests on another can leave out the face they share (skip=...), so most of the geometry
nobody could see is gone. Some stays: the chamfers around a joint, and a face that a neighbour only
partly covers. A primitive that is closed when it is made is checked to be inside-out free: its
signed volume must be positive.

finish_object() welds, triangulates, sets the shading and makes the Blender object. After it
every polygon is a triangle, so what the render shows is what Unreal imports.
"""

import math
from dataclasses import dataclass, field

import bmesh
import bpy
from mathutils import Vector

_AXES = {"X": 0, "Y": 1, "Z": 2}
_SKIP = {"+x": Vector((1, 0, 0)), "-x": Vector((-1, 0, 0)), "+y": Vector((0, 1, 0)),
         "-y": Vector((0, -1, 0)), "+z": Vector((0, 0, 1)), "-z": Vector((0, 0, -1))}


def _frame(axis):
    """Indices (u, v, a) of the coordinates around the axis a, so that u x v = a. A ring that runs
    counter-clockwise in (u, v), seen from +a, then has its normal toward +a."""
    a = _AXES[axis]
    return (a + 1) % 3, (a + 2) % 3, a


def _point(axis, u, v, a):
    i_u, i_v, i_a = _frame(axis)
    co = [0.0, 0.0, 0.0]
    co[i_u], co[i_v], co[i_a] = u, v, a
    return Vector(co)


def signed_volume(faces):
    """The volume a closed set of faces encloses; negative if it is inside out."""
    volume = 0.0
    for face in faces:
        vs = [v.co for v in face.verts]
        for k in range(1, len(vs) - 1):
            volume += vs[0].dot(vs[k].cross(vs[k + 1])) / 6.0
    return volume


def _check_outward(faces, what):
    volume = signed_volume(faces)
    if volume <= 0.0:
        raise ValueError(f"{what}: faces point inwards (signed volume {volume:.6f})")


def _delete_face_towards(bm, faces, name):
    """Deletes the face of `faces` whose normal points along `name` ("+x", "-z"...), the largest if
    several do. Used to leave out the face where two parts touch."""
    axis = _SKIP[name]
    best = None
    for face in faces:
        if face.is_valid:
            face.normal_update()
            if face.normal.dot(axis) > 0.99 and (best is None or face.calc_area() > best.calc_area()):
                best = face
    if best is None:
        raise ValueError(f"skip={name!r}: the part has no face pointing that way")
    bmesh.ops.delete(bm, geom=[best], context="FACES_ONLY")


def _cube(bm, lo, hi):
    """Adds a box from corner lo to corner hi (six quads, facing outwards). Returns its vertices."""
    lo, hi = Vector(lo), Vector(hi)
    center, half = (lo + hi) / 2, (hi - lo) / 2
    corners = bmesh.ops.create_cube(bm, size=2.0)["verts"]
    for v in corners:
        v.co = Vector((v.co.x * half.x, v.co.y * half.y, v.co.z * half.z)) + center
    return corners


def add_box(bm, lo, hi, mat, bevel=0.0, segments=1, skip=(), taper=0.0):
    """A box from corner lo to corner hi. bevel rounds or chamfers every edge by that distance
    (segments=1 a chamfer, 2 a rounded edge). skip names faces to leave out: "+x", "-z"...
    taper moves the top face's edges in by that many metres on every side, so the sides lean in
    (a negative taper flares the box out toward the top: a hopper). A leaning side no longer
    points along an axis, so skip works only for "+z" and "-z" on a tapered box."""
    lo, hi = Vector(lo), Vector(hi)
    size = hi - lo
    if min(size) <= 0.0:
        raise ValueError(f"box: hi must be above lo in every axis, got {tuple(lo)} {tuple(hi)}")
    if bevel * 2 >= min(size):
        raise ValueError(f"box: a bevel of {bevel} is too large for a box {tuple(size)}")
    if taper * 2 >= min(size.x, size.y):
        raise ValueError(f"box: a taper of {taper} leaves nothing of the top of a box {tuple(size)}")
    before = set(bm.faces)
    corners = _cube(bm, lo, hi)
    if taper:
        middle = (lo + hi) / 2
        for v in corners:
            if v.co.z > middle.z:
                v.co.x -= taper if v.co.x > middle.x else -taper
                v.co.y -= taper if v.co.y > middle.y else -taper
    if bevel > 0.0:
        edges = list(dict.fromkeys(e for v in corners for e in v.link_edges))
        bmesh.ops.bevel(bm, geom=edges, offset=bevel, offset_type="OFFSET", segments=segments,
                        profile=0.5, affect="EDGES")
    faces = [f for f in bm.faces if f not in before]
    for f in faces:
        f.material_index = mat
    _check_outward(faces, "box")
    for name in skip:
        _delete_face_towards(bm, faces, name)
    return [f for f in faces if f.is_valid]


def add_prism(bm, outline, axis, lo, hi, mat, skip=()):
    """A flat outline pushed along an axis from lo to hi. The outline is (u, v) points in the plane
    around the axis, counter-clockwise as seen from +axis, in the order of the axes after it: for
    "X" they are (y, z), for "Y" (z, x), for "Z" (x, y). skip may contain "lo" and "hi", the two end
    caps, and the number k of an outline edge (the side face from point k to point k+1). Skipping
    every face but one cap leaves a flat, one-sided disc (a water surface, a dial's face)."""
    pts = [tuple(p) for p in outline]
    count = len(pts)
    if count < 3:
        raise ValueError("prism: an outline needs at least 3 points")
    area2 = sum(pts[k][0] * pts[(k + 1) % count][1] - pts[(k + 1) % count][0] * pts[k][1] for k in range(count))
    if area2 <= 0:
        raise ValueError("prism: give the outline counter-clockwise as seen from +axis (u to the right, v up)")
    lo_ring = [bm.verts.new(_point(axis, u, v, lo)) for u, v in pts]
    hi_ring = [bm.verts.new(_point(axis, u, v, hi)) for u, v in pts]
    sides = []
    for k in range(count):
        k2 = (k + 1) % count
        sides.append(bm.faces.new((lo_ring[k], lo_ring[k2], hi_ring[k2], hi_ring[k])))
    cap_hi = bm.faces.new(hi_ring)
    cap_lo = bm.faces.new(list(reversed(lo_ring)))
    faces = sides + [cap_hi, cap_lo]
    for f in faces:
        f.material_index = mat
    _check_outward(faces, "prism")
    doomed = [face for k, face in enumerate(sides) if k in skip]
    doomed += [face for name, face in (("hi", cap_hi), ("lo", cap_lo)) if name in skip]
    if doomed:
        bmesh.ops.delete(bm, geom=doomed, context="FACES_ONLY")
        # Skipping every face but one (a flat disc) leaves vertices and edges that belong to no face: remove them.
        orphans = [v for v in lo_ring + hi_ring if not v.link_faces]
        if orphans:
            bmesh.ops.delete(bm, geom=orphans, context="VERTS")
    return [f for f in faces if f.is_valid]


def add_cylinder(bm, base, radius, height, sides, mat, top_radius=None, axis="Z",
                 cap_bottom=True, cap_top=True):
    """A cylinder, or a tapered cylinder (another top_radius), standing on the point `base` and
    growing along the axis by `height`. Use 12 or more sides if it should read as round with
    smooth-by-angle shading. (No cone yet: top_radius must be above 0.)"""
    top_radius = radius if top_radius is None else top_radius
    if sides < 3 or radius <= 0.0 or top_radius <= 0.0 or height <= 0.0:
        raise ValueError("cylinder: needs 3 or more sides, positive radii and a positive height")
    base = Vector(base)
    i_u, i_v, i_a = _frame(axis)

    def ring(r, offset):
        verts = []
        for k in range(sides):
            angle = 2 * math.pi * k / sides
            co = base.copy()
            co[i_u] += r * math.cos(angle)
            co[i_v] += r * math.sin(angle)
            co[i_a] += offset
            verts.append(bm.verts.new(co))
        return verts

    bottom = ring(radius, 0.0)
    top = ring(top_radius, height)
    faces = []
    for k in range(sides):
        k2 = (k + 1) % sides
        faces.append(bm.faces.new((bottom[k], bottom[k2], top[k2], top[k])))
    cap_top_face = bm.faces.new(top)
    cap_bottom_face = bm.faces.new(list(reversed(bottom)))
    faces += [cap_top_face, cap_bottom_face]
    for f in faces:
        f.material_index = mat
    _check_outward(faces, "cylinder")
    for face, wanted in ((cap_top_face, cap_top), (cap_bottom_face, cap_bottom)):
        if not wanted:
            bmesh.ops.delete(bm, geom=[face], context="FACES_ONLY")
    return [f for f in faces if f.is_valid]


def add_ring(bm, centre, r_inner, r_outer, height, sides, mat, cap_bottom=True):
    """A ring standing on the point `centre`: a flat-walled tube around the Z axis, with an outer
    wall, an inner wall, a top and a bottom (an annulus of `sides` flat faces, a vertex of each
    on the +X side). The faces of a 12-sided ring meet at 30 degrees, which finish_object
    smooths at its default angle of 35: pass a smaller angle to keep them crisp."""
    if sides < 3 or r_inner <= 0.0 or r_outer <= r_inner or height <= 0.0:
        raise ValueError("ring: needs 3 or more sides, 0 < r_inner < r_outer and a positive height")
    centre = Vector(centre)

    def circle(r, z):
        return [bm.verts.new(centre + Vector((r * math.cos(2 * math.pi * k / sides),
                                              r * math.sin(2 * math.pi * k / sides), z)))
                for k in range(sides)]

    out_b, out_t = circle(r_outer, 0.0), circle(r_outer, height)
    in_b, in_t = circle(r_inner, 0.0), circle(r_inner, height)
    faces, top, bottom = [], [], []
    for k in range(sides):
        k2 = (k + 1) % sides
        faces.append(bm.faces.new((out_b[k], out_b[k2], out_t[k2], out_t[k])))   # outer wall, outwards
        faces.append(bm.faces.new((in_b[k], in_t[k], in_t[k2], in_b[k2])))       # inner wall, towards the axis
        top.append(bm.faces.new((out_t[k], out_t[k2], in_t[k2], in_t[k])))
        bottom.append(bm.faces.new((out_b[k], in_b[k], in_b[k2], out_b[k2])))
    faces += top + bottom
    for f in faces:
        f.material_index = mat
    _check_outward(faces, "ring")
    if not cap_bottom:
        bmesh.ops.delete(bm, geom=bottom, context="FACES_ONLY")
    return [f for f in faces if f.is_valid]


def add_tube(bm, points, radii, sides, mat, caps=(True, True)):
    """A tube swept along a polyline, for a pipe, a lever or an arm. radii is one number or one per
    point (a tapered or thickened tube). caps says whether to close the start and the end. Bends
    should be gentle (under about 45 degrees at a point): the rings are not mitred."""
    pts = [Vector(p) for p in points]
    if len(pts) < 2 or sides < 3:
        raise ValueError("tube: needs at least 2 points and 3 sides")
    radius = [radii] * len(pts) if isinstance(radii, (int, float)) else list(radii)
    if len(radius) != len(pts) or min(radius) <= 0.0:
        raise ValueError("tube: give one positive radius or one per point")
    tangents = []
    for i in range(len(pts)):
        if i == 0:
            t = pts[1] - pts[0]
        elif i == len(pts) - 1:
            t = pts[-1] - pts[-2]
        else:
            t = (pts[i] - pts[i - 1]).normalized() + (pts[i + 1] - pts[i]).normalized()
        if t.length < 1e-9:
            raise ValueError("tube: two points coincide, or the tube folds back on itself")
        tangents.append(t.normalized())
    # Each ring's frame is turned from the one before with as little twist as possible, so the tube never twists.
    normal = Vector((0, 0, 1)) if abs(tangents[0].z) < 0.9 else Vector((1, 0, 0))
    rings = []
    for i, t in enumerate(tangents):
        normal = (normal - t * normal.dot(t)).normalized()
        binormal = t.cross(normal)
        rings.append([bm.verts.new(pts[i] + (normal * math.cos(2 * math.pi * k / sides)
                                             + binormal * math.sin(2 * math.pi * k / sides)) * radius[i])
                      for k in range(sides)])
    faces = []
    for i in range(len(rings) - 1):
        for k in range(sides):
            k2 = (k + 1) % sides
            faces.append(bm.faces.new((rings[i][k], rings[i][k2], rings[i + 1][k2], rings[i + 1][k])))
    start = bm.faces.new(list(reversed(rings[0])))
    end = bm.faces.new(rings[-1])
    faces += [start, end]
    for f in faces:
        f.material_index = mat
    _check_outward(faces, "tube")
    for wanted, face in zip(caps, (start, end)):
        if not wanted:
            bmesh.ops.delete(bm, geom=[face], context="FACES_ONLY")
    return [f for f in faces if f.is_valid]


def add_dome(bm, base, radius, sides, rings, mat):
    """A hemisphere of the given radius standing on the point `base`, open at the bottom (it sits on
    something). rings is the number of bands from the rim to the top (3 gives bands of 30
    degrees); the top is a fan of triangles. Give a dome the same `sides` as what it sits on and
    its rim welds to that part's edge."""
    if sides < 3 or rings < 2 or radius <= 0.0:
        raise ValueError("dome: needs 3 or more sides, 2 or more rings and a positive radius")
    base = Vector(base)
    levels = []
    for j in range(rings):
        elevation = (math.pi / 2) * j / rings
        levels.append([bm.verts.new(base + Vector((radius * math.cos(elevation) * math.cos(2 * math.pi * k / sides),
                                                   radius * math.cos(elevation) * math.sin(2 * math.pi * k / sides),
                                                   radius * math.sin(elevation))))
                       for k in range(sides)])
    apex = bm.verts.new(base + Vector((0.0, 0.0, radius)))
    faces = []
    for j in range(rings - 1):
        for k in range(sides):
            k2 = (k + 1) % sides
            faces.append(bm.faces.new((levels[j][k], levels[j][k2], levels[j + 1][k2], levels[j + 1][k])))
    for k in range(sides):
        faces.append(bm.faces.new((levels[-1][k], levels[-1][(k + 1) % sides], apex)))
    rim = bm.faces.new(list(reversed(levels[0])))   # closes it for the check, then is removed
    for f in faces + [rim]:
        f.material_index = mat
    _check_outward(faces + [rim], "dome")
    bmesh.ops.delete(bm, geom=[rim], context="FACES_ONLY")
    return [f for f in faces if f.is_valid]


# ---------------------------------------------------------------------------
# The finished asset

@dataclass
class Asset:
    """The main mesh (named like the Unreal asset, SM_Bed) and its UCX_ collision shapes, all in
    one collection."""
    main: bpy.types.Object
    collection: bpy.types.Collection
    smooth_type: str  # the FBX exporter's mesh_smooth_type: "FACE" for flat shading, "EDGE" otherwise
    collision: list = field(default_factory=list)

    @property
    def objects(self):
        return [self.main, *self.collision]


def new_collection(name):
    collection = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(collection)
    return collection


def _apply_shading(bm, shade, angle):
    if shade == "flat":
        for face in bm.faces:
            face.smooth = False
        for edge in bm.edges:
            edge.smooth = False
        return
    limit = math.radians(angle)
    for edge in bm.edges:
        edge.smooth = len(edge.link_faces) == 2 and edge.calc_face_angle(math.pi) < limit
    for face in bm.faces:
        face.smooth = True


def finish_object(name, bm, slots, shade="flat", angle=35.0, weld=1e-5):
    """Makes the asset's main object from the bmesh and frees the bmesh. The mesh has no UV map,
    which is right for flat-colour meshes (and saves 8 bytes a vertex); the project has no baked
    lighting, so no lightmap UVs either.

    shade   "flat": every face flat, the faceted look of the crops. "angle": smooth, with a hard
            edge wherever two faces meet at more than `angle` degrees (smooth-by-angle). Boxes
            stay crisp and a cylinder of 12 or more sides reads as round; the default 35 degrees
            sits just above the 30 degrees of a 12-sided cylinder.
    weld    vertices closer than this (metres) are merged, so parts that touch share vertices.
    """
    if shade not in ("flat", "angle"):
        raise ValueError(f"shade must be 'flat' or 'angle', not {shade!r}")
    if bm.faces and max(f.material_index for f in bm.faces) >= len(slots.materials):
        raise ValueError(f"{name}: a face uses a material slot that the mesh doesn't have")
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=weld)
    # One fixed diagonal for quads, so the render and the Unreal import cut the faces the same way.
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="SHORT_EDGE", ngon_method="BEAUTY")
    bm.normal_update()
    _apply_shading(bm, shade, angle)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for material in slots.materials:
        mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    collection = new_collection(name)
    collection.objects.link(obj)
    return Asset(obj, collection, "FACE" if shade == "flat" else "EDGE")


def _add_collision_object(asset, bm):
    name = f"UCX_{asset.main.name}_{len(asset.collision):02d}"
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="SHORT_EDGE", ngon_method="BEAUTY")
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.display_type = "WIRE"
    obj.hide_render = True
    asset.collection.objects.link(obj)
    asset.collision.append(obj)
    return obj


def add_collision_box(asset, lo, hi):
    """The next simple collision shape, UCX_<Mesh>_NN: a box from lo to hi. Unreal turns each
    UCX_ mesh into one convex shape of the static mesh. Make the shapes simple: a few boxes."""
    bm = bmesh.new()
    _cube(bm, lo, hi)
    return _add_collision_object(asset, bm)


def add_collision_hull(asset, points):
    """The next UCX_ shape as the convex hull of some points (a cylinder or an arch, say). Keep it
    under about 32 points."""
    bm = bmesh.new()
    verts = [bm.verts.new(p) for p in points]
    result = bmesh.ops.convex_hull(bm, input=verts)
    unused = result["geom_interior"] + result["geom_unused"]
    bmesh.ops.delete(bm, geom=[g for g in unused if isinstance(g, bmesh.types.BMVert)], context="VERTS")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return _add_collision_object(asset, bm)
