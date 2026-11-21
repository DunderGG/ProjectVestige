# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up the clean water content (Docs/Systems/24-Water.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpWell.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

1. DA_Item_WateringCan: adds a Farm Water Container Fragment with Capacity 8, if it has none (it fails if the can stacks).
2. DA_FeedbackResponses: adds the five Vestige.Feedback.Water.* rows. A row that is already there is left as it is.
3. BP_Well (/Game/Vestige/Blueprints, parent AFarmWell): makes it if missing and sets WellMesh to SM_Well.
4. L_Farm: places one BP_Well at X 1880, Y 360, Z 0, yaw 0 if the map has none, and saves the map.

It changes and saves those four assets and the map, and nothing else (not SM_Well or the MI_Prop_ instances). Run it again
at any time: it only writes what differs. Every line it prints starts with "SetUpWell:".
"""

import unreal

ITEM_PATH = "/Game/Vestige/Items/DA_Item_WateringCan"
CAN_CAPACITY = 8
FEEDBACK_RESPONSES = "/Game/Vestige/UI/DA_FeedbackResponses"
BLUEPRINT_FOLDER = "/Game/Vestige/Blueprints"
BLUEPRINT_NAME = "BP_Well"
MESH_PATH = "/Game/Vestige/Meshes/SM_Well"
MAP_PATH = "/Game/Maps/L_Farm"
WELL_LOCATION = unreal.Vector(1880.0, 360.0, 0.0)
WELL_ROTATION = unreal.Rotator(0.0, 0.0, 0.0)

TEXT_NAMESPACE = "VestigeFeedback"
# The texts are the game designer's (Docs/GameDesign.md, section 6, Water); keep them in step.
FEEDBACK_ROWS = {
    "Vestige.Feedback.Water.CanEmpty": "Your can is empty. Fill it at the well.",
    "Vestige.Feedback.Water.CanFull": "Your can is already full.",
    "Vestige.Feedback.Water.CanFilled": "Can filled.",
    "Vestige.Feedback.Water.WellDry": "The well is dry. It refills overnight.",
    "Vestige.Feedback.Water.WellRanDry": "The well ran dry before your can was full.",
}


def log(message):
    unreal.log("SetUpWell: " + message)


def fail(message):
    unreal.log_error("SetUpWell: " + message)
    raise RuntimeError(message)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        fail("Can't load " + path)
    return asset


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        fail("Can't save " + asset.get_path_name())
    log("saved " + asset.get_path_name())


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="{0}")'.format(name))
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        fail("The tag manager doesn't know " + name + "; build VestigeEditor first")
    return tag


def text_literal(key, source):
    for value in (TEXT_NAMESPACE, key, source):
        if '"' in value or "\\" in value:
            fail("A text for import_text holds a quote or a backslash: " + value)
    return 'NSLOCTEXT("{0}", "{1}", "{2}")'.format(TEXT_NAMESPACE, key, source)


def set_up_can():
    item = load(ITEM_PATH)
    if item.get_editor_property("max_stack_size") != 1:
        fail("DA_Item_WateringCan stacks; a water container must have Max Stack Size 1")
    fragments = list(item.get_editor_property("fragments"))
    for fragment in fragments:
        if fragment is not None and isinstance(fragment, unreal.FarmWaterContainerFragment):
            if fragment.get_editor_property("capacity") == CAN_CAPACITY:
                log("DA_Item_WateringCan already has its water container fragment")
                return
            fragment.modify()
            fragment.set_editor_property("capacity", CAN_CAPACITY)
            save(item)
            return
    fragment = unreal.new_object(unreal.FarmWaterContainerFragment, outer=item)
    fragment.set_editor_property("capacity", CAN_CAPACITY)
    fragments.append(fragment)
    item.modify()
    item.set_editor_property("fragments", fragments)
    log("added the water container fragment, Capacity {0}".format(CAN_CAPACITY))
    save(item)


def add_feedback_rows():
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    existing = {}
    for tag, response in responses.items():
        existing[str(unreal.GameplayTagLibrary.get_tag_name(tag))] = str(response.get_editor_property("message"))
    changed = False
    for tag_name, text in sorted(FEEDBACK_ROWS.items()):
        if tag_name in existing:
            if existing[tag_name] != text:
                unreal.log_warning("SetUpWell: kept the row " + tag_name + " as it is; its text differs from the designer's")
            continue
        response = unreal.VestigeFeedbackResponse()
        key = tag_name[len("Vestige.Feedback."):]
        response.import_text("(Message={0})".format(text_literal(key, text)))
        responses[make_tag(tag_name)] = response
        log("added the row " + tag_name)
        changed = True
    if not changed:
        log("DA_FeedbackResponses already has the water rows")
        return
    asset.modify()
    asset.set_editor_property("responses", responses)
    save(asset)


def set_up_blueprint():
    path = BLUEPRINT_FOLDER + "/" + BLUEPRINT_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        blueprint = load(path)
        log(path + " exists")
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.FarmWell)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            BLUEPRINT_NAME, BLUEPRINT_FOLDER, unreal.Blueprint, factory)
        if blueprint is None:
            fail("Can't create " + path)
        log("created " + path)
    mesh = load(MESH_PATH)
    well = unreal.get_default_object(blueprint.generated_class())
    component = well.get_editor_property("well_mesh")
    if component is None:
        fail("BP_Well's class has no WellMesh component")
    if component.get_editor_property("static_mesh") == mesh:
        log("BP_Well's WellMesh already shows SM_Well")
    else:
        component.modify()
        component.set_editor_property("static_mesh", mesh)
        log("set WellMesh to SM_Well")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)
    return blueprint


def place_well(blueprint):
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail("Can't load " + MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    wells = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.FarmWell)]
    if wells:
        log("L_Farm already has {0} well(s), none placed".format(len(wells)))
        return
    well = actors.spawn_actor_from_class(blueprint.generated_class(), WELL_LOCATION, WELL_ROTATION)
    if well is None:
        fail("Can't spawn BP_Well")
    well.set_actor_label(BLUEPRINT_NAME)
    log("placed BP_Well at {0}".format(well.get_actor_location()))
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("Can't save " + MAP_PATH)
    log("saved " + MAP_PATH)


def print_facts(blueprint):
    item = load(ITEM_PATH)
    for fragment in item.get_editor_property("fragments"):
        detail = fragment.get_editor_property("capacity") if isinstance(fragment, unreal.FarmWaterContainerFragment) else ""
        log("fact: DA_Item_WateringCan fragment {0} {1}".format(fragment.get_class().get_name(), detail))
    log("fact: DA_Item_WateringCan max_stack_size = {0}".format(item.get_editor_property("max_stack_size")))
    well = unreal.get_default_object(blueprint.generated_class())
    mesh = well.get_editor_property("well_mesh").get_editor_property("static_mesh")
    log("fact: BP_Well WellMesh = {0}".format(mesh.get_path_name() if mesh else "None"))
    responses = load(FEEDBACK_RESPONSES).get_editor_property("responses")
    for tag, response in responses.items():
        name = str(unreal.GameplayTagLibrary.get_tag_name(tag))
        if name in FEEDBACK_ROWS:
            log("fact: DA_FeedbackResponses {0} = {1}".format(name, response.export_text()))
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.FarmWell):
            log("fact: L_Farm well {0} at {1}, yaw {2}".format(
                actor.get_actor_label(), actor.get_actor_location(), actor.get_actor_rotation().yaw))


def main():
    set_up_can()
    add_feedback_rows()
    blueprint = set_up_blueprint()
    place_well(blueprint)
    print_facts(blueprint)
    log("done")


if __name__ == "__main__":
    main()
