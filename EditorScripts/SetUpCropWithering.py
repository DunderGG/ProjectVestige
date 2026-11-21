# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Make the withered crop look and its two feedback texts (Systems.md, sections 15 and 22).

1. Saves Used with Instanced Static Meshes on M_Crop, if it isn't saved already.
2. Makes MI_Crop_Withered, an instance of M_Crop in a dry straw-brown with a high roughness, or updates its two values.
3. Sets it as WitheredCropMaterial in BP_FarmGrid's look settings (its UFarmFieldMeshesComponent, Looks).
4. Adds the DA_FeedbackResponses rows Vestige.Feedback.Farm.Withered and Vestige.Feedback.Farm.ClearFirst. A row that is
   already there is left as it is, so a text changed by hand in the editor is never overwritten; the log says so.

It changes and saves those four assets and nothing else. Run it again at any time: it only writes what differs.
With the editor closed, from the project folder:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpCropWithering.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

or in the editor with Tools > Execute Python Script. Every line it prints starts with "SetUpCropWithering:".
"""

import unreal


PARENT_MATERIAL = "/Game/Vestige/Materials/M_Crop"
WITHERED_MATERIAL_FOLDER = "/Game/Vestige/Materials"
WITHERED_MATERIAL_NAME = "MI_Crop_Withered"
FARM_GRID_BLUEPRINT = "/Game/Vestige/Blueprints/BP_FarmGrid"
FEEDBACK_RESPONSES = "/Game/Vestige/UI/DA_FeedbackResponses"

# Linear colour, the same dry straw-brown as the placeholder's FarmTileLooks::WitheredColor (sRGB about #897650, chosen by
# @DunderGG on 2026-11-20): lighter and yellower than the tilled soil (#6B4A2B), so a dead plant stands out against dry
# and wet soil alike, warmer than the well's grey stone (#9A9384) so it doesn't read as rubble, and far from the water
# (azure) and contamination (magenta) cues and from the living crops' greens, yellows and purples.
WITHERED_BASE_COLOR = (0.25, 0.18, 0.08)
WITHERED_ROUGHNESS = 0.95

TEXT_NAMESPACE = "VestigeFeedback"
# The texts are the ones in Docs/Systems/15-EffectsAndFeedback.md's table; keep them in step.
FEEDBACK_ROWS = {
    "Vestige.Feedback.Farm.Withered": (
        "{Count} {Count}|plural(one=crop,other=crops) withered. "
        "{Count}|plural(one=It doesn't,other=They don't) grow in {SeasonInSentence}."
    ),
    "Vestige.Feedback.Farm.ClearFirst": "Clear the withered crop first.",
}


def log(message):
    unreal.log("SetUpCropWithering: " + message)


def fail(message):
    unreal.log_error("SetUpCropWithering: " + message)
    raise RuntimeError(message)


def rows_to_add(existing_messages, wanted_rows):
    """The wanted rows whose tag has no row yet, as (tag, text) sorted by tag, and the tags whose row has another text.

    existing_messages maps a tag name to the text of its row.
    """
    to_add = []
    different = []
    for tag, text in sorted(wanted_rows.items()):
        if tag not in existing_messages:
            to_add.append((tag, text))
        elif existing_messages[tag] != text:
            different.append(tag)
    return to_add, different


def text_literal(namespace, key, source):
    """An NSLOCTEXT literal for import_text, so the text keeps a stable key for localization."""
    for value in (namespace, key, source):
        if '"' in value or "\\" in value:
            fail("A text for import_text holds a quote or a backslash: " + value)
    return 'NSLOCTEXT("{0}", "{1}", "{2}")'.format(namespace, key, source)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        fail("Can't load " + path)
    return asset


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        fail("Can't save " + asset.get_path_name())
    log("saved " + asset.get_path_name())


def ensure_instanced_usage(parent):
    if parent.get_editor_property("used_with_instanced_static_meshes"):
        log(PARENT_MATERIAL + " already has Used with Instanced Static Meshes saved")
        return
    parent.modify()
    parent.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.MaterialEditingLibrary.recompile_material(parent)
    save(parent)


def check_parameters(parent):
    vectors = [str(name) for name in unreal.MaterialEditingLibrary.get_vector_parameter_names(parent)]
    scalars = [str(name) for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(parent)]
    if "BaseColor" not in vectors or "Roughness" not in scalars:
        fail("M_Crop lacks its BaseColor or Roughness parameter (vectors {0}, scalars {1})".format(vectors, scalars))


def is_withered_material_current(instance, parent):
    library = unreal.MaterialEditingLibrary
    color = library.get_material_instance_vector_parameter_value(instance, "BaseColor")
    roughness = library.get_material_instance_scalar_parameter_value(instance, "Roughness")
    wanted = WITHERED_BASE_COLOR
    return (
        instance.get_editor_property("parent") == parent
        and all(abs(a - b) < 1e-4 for a, b in zip((color.r, color.g, color.b), wanted))
        and abs(roughness - WITHERED_ROUGHNESS) < 1e-4
    )


def make_withered_material(parent):
    path = WITHERED_MATERIAL_FOLDER + "/" + WITHERED_MATERIAL_NAME
    library = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = load(path)
        if is_withered_material_current(instance, parent):
            log(path + " already has its parent and values")
            return instance
        log("updating " + path)
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            WITHERED_MATERIAL_NAME,
            WITHERED_MATERIAL_FOLDER,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        if instance is None:
            fail("Can't create " + path)
        log("created " + path)
    instance.modify()
    library.set_material_instance_parent(instance, parent)
    library.set_material_instance_vector_parameter_value(
        instance, "BaseColor", unreal.LinearColor(*WITHERED_BASE_COLOR, 1.0)
    )
    library.set_material_instance_scalar_parameter_value(instance, "Roughness", WITHERED_ROUGHNESS)
    library.update_material_instance(instance)
    save(instance)
    return instance


def set_grid_material(material):
    blueprint = load(FARM_GRID_BLUEPRINT)
    grid = unreal.get_default_object(blueprint.generated_class())
    meshes = grid.get_editor_property("meshes")
    if meshes is None:
        fail("BP_FarmGrid has no Meshes component")
    looks = meshes.get_editor_property("looks")
    current = looks.get_editor_property("withered_crop_material")
    if current == material:
        log("BP_FarmGrid already uses " + material.get_path_name())
        return
    looks.set_editor_property("withered_crop_material", material)
    meshes.modify()
    meshes.set_editor_property("looks", looks)
    save(blueprint)


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="{0}")'.format(name))
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        fail("The tag manager doesn't know " + name + "; build VestigeEditor first")
    return tag


def add_feedback_rows():
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    existing = {}
    for tag, response in responses.items():
        existing[str(unreal.GameplayTagLibrary.get_tag_name(tag))] = str(response.get_editor_property("message"))
    to_add, different = rows_to_add(existing, FEEDBACK_ROWS)
    for tag in different:
        unreal.log_warning("SetUpCropWithering: kept the row " + tag + " as it is; its text differs from the guide's")
    if not to_add:
        log("DA_FeedbackResponses already has both rows")
        return
    for tag_name, text in to_add:
        response = unreal.VestigeFeedbackResponse()
        key = tag_name[len("Vestige.Feedback."):]
        response.import_text("(Message={0})".format(text_literal(TEXT_NAMESPACE, key, text)))
        responses[make_tag(tag_name)] = response
        log("added the row " + tag_name)
    asset.modify()
    asset.set_editor_property("responses", responses)
    save(asset)


def print_facts(parent, material):
    """What the assets hold now, for the art review."""
    library = unreal.MaterialEditingLibrary
    log("fact: M_Crop used_with_instanced_static_meshes = {0}".format(
        parent.get_editor_property("used_with_instanced_static_meshes")))
    log("fact: {0} parent = {1}".format(material.get_name(), material.get_editor_property("parent").get_path_name()))
    log("fact: {0} BaseColor = {1}".format(
        material.get_name(), library.get_material_instance_vector_parameter_value(material, "BaseColor")))
    log("fact: {0} Roughness = {1}".format(
        material.get_name(), library.get_material_instance_scalar_parameter_value(material, "Roughness")))
    grid = unreal.get_default_object(load(FARM_GRID_BLUEPRINT).generated_class())
    looks = grid.get_editor_property("meshes").get_editor_property("looks")
    chosen = looks.get_editor_property("withered_crop_material")
    log("fact: BP_FarmGrid WitheredCropMaterial = {0}".format(chosen.get_path_name() if chosen else "None"))
    log("fact: BP_FarmGrid WitheredCropHeightScale = {0}".format(looks.get_editor_property("withered_crop_height_scale")))
    responses = load(FEEDBACK_RESPONSES).get_editor_property("responses")
    for tag, response in responses.items():
        name = str(unreal.GameplayTagLibrary.get_tag_name(tag))
        if name in FEEDBACK_ROWS:
            log("fact: DA_FeedbackResponses {0} = {1}".format(name, response.export_text()))
    log("fact: DA_FeedbackResponses has {0} rows".format(len(responses)))


def main():
    parent = load(PARENT_MATERIAL)
    check_parameters(parent)
    ensure_instanced_usage(parent)
    material = make_withered_material(parent)
    set_grid_material(material)
    add_feedback_rows()
    print_facts(parent, material)
    log("done")


if __name__ == "__main__":
    main()
