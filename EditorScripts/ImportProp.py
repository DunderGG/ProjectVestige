# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Import (or reimport) one prop mesh built on Art/Blender/vestige_art/ (Art/README.md, "Importing into Unreal"),
headless, with the editor closed:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script="EditorScripts/ImportProp.py SM_ShippingBin"
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

The asset name is the argument; the FBX is Art/Export/<name>.fbx and the mesh /Game/Vestige/Meshes/<name>.

1. Imports the FBX through the Interchange manager with a pipeline override, or reimports the existing mesh with
   the same override (a plain editor reimport would bring back generated materials and collision): static mesh only,
   Nanite on, no LODs, no lightmap UVs, collision from the UCX_<name>_NN shapes only, no new materials.
2. Gives each material slot the instance of its name, /Game/Vestige/Materials/MI_Prop_<Key>. A missing instance is
   made from M_Prop with the key's colour and roughness from Art/Blender/vestige_art/palette.py; an existing one
   is left as it is, and a value that differs from the palette is reported, not changed.
3. Nanite with its fallback at 100% of the triangles (FallbackTarget PercentTriangles, FallbackPercentTriangles 1).
4. Cast shadow on every section, except for the props in NO_SHADOW (CONTRIBUTING.md, Budgets).
5. Checks what Unreal made against the FBX (as many convex hulls as UCX_ shapes, none for a collision=False prop) and
   prints the facts: triangles, bounds, Nanite, simple collision shapes, sockets,
   slots and their materials, cast shadow per section. A reimport fails unless the mesh was imported from
   Art/Export/<name>.fbx.

It saves the mesh and any instance it made, only after the checks pass, and nothing else: it places nothing in a
level and makes no Blueprint.
Every line it prints starts with "ImportProp:".
"""

import os
import re
import sys

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT_DIR = os.path.join(PROJECT_DIR, "Art", "Export")
MESHES_FOLDER = "/Game/Vestige/Meshes"
MATERIALS_FOLDER = "/Game/Vestige/Materials"
PARENT_MATERIAL = MATERIALS_FOLDER + "/M_Prop"
COLOR_PARAMETER = "BaseColor"
ROUGHNESS_PARAMETER = "Roughness"
# Props whose sections don't cast shadows (Art/README.md, "Importing into Unreal", step 4).
NO_SHADOW = {"SM_Sprinkler"}
# The palette's linear colours are rounded when they are typed or stored; a difference below this isn't reported.
COLOR_TOLERANCE = 0.002

sys.dont_write_bytecode = True  # no __pycache__ folder in the repository
sys.path.append(os.path.join(PROJECT_DIR, "Art", "Blender"))
from vestige_art.palette import PALETTE, PREFIX, hex_to_linear  # noqa: E402


def log(message):
    unreal.log("ImportProp: " + message)


def warn(message):
    unreal.log_warning("ImportProp: " + message)


def fail(message):
    unreal.log_error("ImportProp: " + message)
    raise RuntimeError(message)


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        fail("Can't save " + asset.get_path_name())
    log("saved " + asset.get_path_name())


def asset_name_from_arguments():
    names = [arg for arg in sys.argv[1:] if not arg.startswith("-")]
    if len(names) != 1 or not names[0].startswith("SM_"):
        fail("Give one mesh name, such as SM_ShippingBin, after the script's path (got " + str(sys.argv[1:]) + ")")
    return names[0]


def make_pipeline(has_collision):
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("use_source_name_for_asset", True)
    common = pipeline.get_editor_property("common_meshes_properties")
    common.set_editor_property("import_lods", False)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("import_static_meshes", True)
    meshes.set_editor_property("import_skeletal_meshes", False)
    meshes.set_editor_property("import_geometry_caches", False)
    meshes.set_editor_property("build_nanite", True)
    meshes.set_editor_property("generate_lightmap_u_vs", False)
    meshes.set_editor_property("collision", has_collision)  # off without UCX_ shapes, or a hull is generated
    meshes.set_editor_property("import_collision_according_to_mesh_name", True)
    meshes.set_editor_property("force_collision_primitive_generation", False)
    materials = pipeline.get_editor_property("material_pipeline")
    materials.set_editor_property("import_materials", True)
    materials.set_editor_property("reuse_existing_materials", True)
    materials.set_editor_property("search_location", unreal.InterchangeMaterialSearchLocation.UNDER_PARENT)
    materials.set_editor_property("create_new_materials", False)
    return pipeline


def import_parameters(pipeline, reimport_asset):
    parameters = unreal.ImportAssetParameters()
    parameters.set_editor_property("is_automated", True)
    parameters.set_editor_property("replace_existing", True)
    parameters.set_editor_property("override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())])
    if reimport_asset is not None:
        parameters.set_editor_property("reimport_asset", reimport_asset)
    return parameters


def fbx_path(name):
    fbx = os.path.join(EXPORT_DIR, name + ".fbx")
    if not os.path.isfile(fbx):
        fail("Can't find " + fbx + "; run its script in Art/Blender first")
    return fbx


def count_collision_shapes(fbx, name):
    """The number of distinct UCX_<name>_NN objects in the FBX: their names are stored as plain bytes, binary or
    ASCII FBX alike. Zero for a prop built with collision=False (Art/README.md, "Adding an asset")."""
    with open(fbx, "rb") as stream:
        data = stream.read()
    return len(set(re.findall(rb"UCX_" + re.escape(name.encode("ascii")) + rb"_\d+", data)))


def same_file(first, second):
    return os.path.normcase(os.path.abspath(first)) == os.path.normcase(os.path.abspath(second))


def check_reimport_source(mesh, fbx):
    import_data = mesh.get_editor_property("asset_import_data")
    source = import_data.get_first_filename() if import_data is not None else ""
    if not source or not same_file(source, fbx):
        fail(mesh.get_path_name() + " was imported from " + (source or "an unknown file") + ", not " + fbx
             + "; reimport it by hand or delete it first")


def import_mesh(name, fbx, has_collision):
    mesh_path = MESHES_FOLDER + "/" + name
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    pipeline = make_pipeline(has_collision)  # kept referenced until the import is done
    existing = None
    if unreal.EditorAssetLibrary.does_asset_exist(mesh_path):
        existing = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if existing is not None:
        check_reimport_source(existing, fbx)
        result = manager.reimport_asset(existing, import_parameters(pipeline, existing))
        verb = "reimported"
    else:
        source = unreal.InterchangeManager.create_source_data(fbx)
        result = manager.import_asset(MESHES_FOLDER, source, import_parameters(pipeline, None))
        verb = "imported"
    succeeded = result[0] if isinstance(result, tuple) else result
    if not succeeded:
        fail("Interchange couldn't import " + fbx)
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if mesh is None or not isinstance(mesh, unreal.StaticMesh):
        fail("Importing " + fbx + " didn't give a StaticMesh at " + mesh_path)
    log(verb + " " + fbx + " as " + mesh_path)
    return mesh


def check_palette_values(instance, key):
    hex_color, roughness = PALETTE[key]
    expected = hex_to_linear(hex_color)
    color = unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(instance, COLOR_PARAMETER)
    actual = (color.r, color.g, color.b)
    if any(abs(a - e) > COLOR_TOLERANCE for a, e in zip(actual, expected)):
        warn("%s's %s is %s, the palette says %s (linear %s)" % (instance.get_name(), COLOR_PARAMETER,
             tuple(round(c, 4) for c in actual), hex_color, tuple(round(c, 4) for c in expected)))
    actual_roughness = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
        instance, ROUGHNESS_PARAMETER)
    if abs(actual_roughness - roughness) > COLOR_TOLERANCE:
        warn("%s's %s is %.3f, the palette says %.3f" % (instance.get_name(), ROUGHNESS_PARAMETER,
             actual_roughness, roughness))


def find_or_make_instance(slot_name, made):
    if not slot_name.startswith(PREFIX) or slot_name[len(PREFIX):] not in PALETTE:
        fail("The slot " + slot_name + " isn't " + PREFIX + "<a key of vestige_art/palette.py>")
    key = slot_name[len(PREFIX):]
    path = MATERIALS_FOLDER + "/" + slot_name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.EditorAssetLibrary.load_asset(path)
        check_palette_values(instance, key)
        return instance
    parent = unreal.EditorAssetLibrary.load_asset(PARENT_MATERIAL)
    if parent is None:
        fail("Can't load " + PARENT_MATERIAL + " (Art/README.md, \"Importing into Unreal\", step 1)")
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        slot_name, MATERIALS_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if instance is None:
        fail("Can't create " + path)
    hex_color, roughness = PALETTE[key]
    red, green, blue = hex_to_linear(hex_color)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, COLOR_PARAMETER, unreal.LinearColor(red, green, blue, 1.0))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, ROUGHNESS_PARAMETER, roughness)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    # Saved by main() after the check, with the mesh.
    log("made " + path + " from " + hex_color + ", roughness " + str(roughness))
    made.append(instance)
    return instance


def assign_materials(mesh):
    """Returns the instances it made, unsaved."""
    made = []
    slots = list(mesh.get_editor_property("static_materials"))
    changed = False
    for index, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        instance = find_or_make_instance(slot_name, made)
        if slot.get_editor_property("material_interface") != instance:
            slot.set_editor_property("material_interface", instance)
            slots[index] = slot
            changed = True
    if changed:
        mesh.modify()
        mesh.set_editor_property("static_materials", slots)
        log("set the slots' materials by slot name")
    return made


def set_nanite(mesh, editor):
    settings = editor.get_nanite_settings(mesh)
    settings.set_editor_property("enabled", True)
    settings.set_editor_property("fallback_target", unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
    settings.set_editor_property("fallback_percent_triangles", 1.0)
    settings.set_editor_property("keep_percent_triangles", 1.0)
    editor.set_nanite_settings(mesh, settings, apply_changes=True)


def set_shadows(mesh, editor, cast_shadow):
    for section in range(mesh.get_num_sections(0)):
        if editor.is_section_cast_shadow_enabled(mesh, 0, section) != cast_shadow:
            editor.enable_section_cast_shadow(mesh, cast_shadow, 0, section)


def check_and_report(mesh, editor, cast_shadow, expected_hulls):
    body = mesh.get_editor_property("body_setup")
    shapes = body.get_editor_property("agg_geom")
    convex = len(shapes.get_editor_property("convex_elems"))
    other = (len(shapes.get_editor_property("box_elems")) + len(shapes.get_editor_property("sphere_elems"))
             + len(shapes.get_editor_property("sphyl_elems")))
    nanite = editor.get_nanite_settings(mesh)
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    log("triangles (LOD 0, the 100%% fallback): %d, vertices %d, LODs %d" % (
        mesh.get_num_triangles(0), mesh.get_num_vertices(0), editor.get_lod_count(mesh)))
    log("bounds (cm): min (%.1f, %.1f, %.1f) max (%.1f, %.1f, %.1f), size %.1f x %.1f x %.1f" % (
        bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z, size.x, size.y, size.z))
    log("Nanite %s, fallback target %s at %.2f, keep %.2f" % (
        "on" if nanite.get_editor_property("enabled") else "off", nanite.get_editor_property("fallback_target"),
        nanite.get_editor_property("fallback_percent_triangles"), nanite.get_editor_property("keep_percent_triangles")))
    log("simple collision: %d convex hull(s), %d other shape(s), complexity %s" % (
        convex, other, body.get_editor_property("collision_trace_flag")))
    # Sockets is protected in Python; the sockets an FBX makes have no tag, so this lists them all.
    log("sockets: %d" % len(mesh.get_sockets_by_tag("")))
    for slot in mesh.get_editor_property("static_materials"):
        material = slot.get_editor_property("material_interface")
        log("slot %s -> %s" % (slot.get_editor_property("material_slot_name"),
                               material.get_path_name() if material else "none"))
    shadows = [editor.is_section_cast_shadow_enabled(mesh, 0, s) for s in range(mesh.get_num_sections(0))]
    log("cast shadow per section: %s" % shadows)
    if not nanite.get_editor_property("enabled"):
        fail("Nanite is off")
    if convex != expected_hulls or other != 0:
        fail("The collision should be the FBX's %d UCX_ hull(s) and nothing else" % expected_hulls)
    if any(shadow != cast_shadow for shadow in shadows):
        fail("A section's cast shadow isn't " + str(cast_shadow))


def main():
    name = asset_name_from_arguments()
    unreal.load_module("StaticMeshEditor")  # the subsystem isn't loaded in a commandlet until its module is
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if editor is None:
        fail("No StaticMeshEditorSubsystem")
    cast_shadow = name not in NO_SHADOW
    fbx = fbx_path(name)
    hulls = count_collision_shapes(fbx, name)
    log("%s has %d UCX_ shape(s)" % (fbx, hulls))
    mesh = import_mesh(name, fbx, hulls > 0)
    made = assign_materials(mesh)
    set_nanite(mesh, editor)
    set_shadows(mesh, editor, cast_shadow)
    # Check before saving: a failed check leaves the mesh and any new instance unsaved, and the headless editor
    # exits without saving them.
    check_and_report(mesh, editor, cast_shadow, hulls)
    for instance in made:
        save(instance)
    save(mesh)
    log("done")


main()
