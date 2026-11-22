# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up the selling content (Docs/Systems/25-Selling.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpShippingBin.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

1. Sell prices: DA_Item_Turnip 25, DA_Item_Potato 18, DA_Item_Cabbage 110, and 0 on the three seed items, DA_Item_Hoe and
   DA_Item_WateringCan. Saves each item it changed.
2. DA_FeedbackResponses: adds the rows Vestige.Feedback.Shipping.Shipped and .NothingToShip (no sound), and the row .Paid,
   and adds " {Paid}" to the end of the six Vestige.Feedback.Day.* rows' texts (a row whose text already has {Paid} is left
   as it is, so this runs once).
3. BP_ShippingBin (/Game/Vestige/Blueprints, parent AFarmShippingBin): makes it if missing and sets BinMesh to SM_ShippingBin.
   Fails if the mesh isn't there.
4. L_Farm: places one BP_ShippingBin at X 1880, Y 600, Z 0, yaw 180 if the map has none, and saves the map.

It changes and saves those assets and the map, and nothing else. Run it again at any time: it only writes what differs.
Every line it prints starts with "SetUpShippingBin:". The WBP_Hud MoneyText is a hand step (the design's step 5).
"""

import re

import unreal

ITEM_FOLDER = "/Game/Vestige/Items/"
# The sell prices: the game designer's (Docs/GameDesign.md, section 6 and section 9, Economy). 0 means the bin won't take it.
SELL_PRICES = {
    "DA_Item_Turnip": 25,
    "DA_Item_Potato": 18,
    "DA_Item_Cabbage": 110,
    "DA_Item_TurnipSeeds": 0,
    "DA_Item_PotatoSeeds": 0,
    "DA_Item_CabbageSeeds": 0,
    "DA_Item_Hoe": 0,
    "DA_Item_WateringCan": 0,
}

FEEDBACK_RESPONSES = "/Game/Vestige/UI/DA_FeedbackResponses"
TEXT_NAMESPACE = "VestigeFeedback"
# The texts are the ones in Docs/Systems/15-EffectsAndFeedback.md's table; keep them in step.
FEEDBACK_ROWS = {
    "Vestige.Feedback.Shipping.Shipped": "Shipped: {Item} ×{Count}. The bin pays in the morning.",
    "Vestige.Feedback.Shipping.NothingToShip": "Hold a crop to ship it.",
    "Vestige.Feedback.Shipping.Paid": "The bin paid {Amount}.",
}
DAY_ROWS = [
    "Vestige.Feedback.Day.Slept",
    "Vestige.Feedback.Day.StayedUp",
    "Vestige.Feedback.Day.Slept.LastDay",
    "Vestige.Feedback.Day.StayedUp.LastDay",
    "Vestige.Feedback.Day.Slept.NewSeason",
    "Vestige.Feedback.Day.StayedUp.NewSeason",
]
PAID_SUFFIX = " {Paid}"

BLUEPRINT_FOLDER = "/Game/Vestige/Blueprints"
BLUEPRINT_NAME = "BP_ShippingBin"
MESH_PATH = "/Game/Vestige/Meshes/SM_ShippingBin"
MAP_PATH = "/Game/Maps/L_Farm"
BIN_LOCATION = unreal.Vector(1880.0, 600.0, 0.0)
BIN_ROTATION = unreal.Rotator(0.0, 0.0, 180.0)


def log(message):
    unreal.log("SetUpShippingBin: " + message)


def fail(message):
    unreal.log_error("SetUpShippingBin: " + message)
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


def text_literal(namespace, key, source):
    for value in (namespace, key, source):
        if '"' in value or "\\" in value:
            fail("A text for import_text holds a quote or a backslash: " + value)
    return 'NSLOCTEXT("{0}", "{1}", "{2}")'.format(namespace, key, source)


def set_up_prices():
    for name, price in sorted(SELL_PRICES.items()):
        item = load(ITEM_FOLDER + name)
        if item.get_editor_property("sell_price") == price:
            log(name + " already sells for {0}".format(price))
            continue
        item.modify()
        item.set_editor_property("sell_price", price)
        log("set the sell price of {0} to {1}".format(name, price))
        save(item)


def add_feedback_rows():
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    by_name = {}
    for tag, response in responses.items():
        by_name[str(unreal.GameplayTagLibrary.get_tag_name(tag))] = (tag, response)
    changed = False

    for tag_name, text in sorted(FEEDBACK_ROWS.items()):
        if tag_name in by_name:
            if str(by_name[tag_name][1].get_editor_property("message")) != text:
                unreal.log_warning("SetUpShippingBin: kept the row " + tag_name + " as it is; its text differs from the designer's")
            continue
        response = unreal.VestigeFeedbackResponse()
        key = tag_name[len("Vestige.Feedback."):]
        response.import_text("(Message={0})".format(text_literal(TEXT_NAMESPACE, key, text)))
        responses[make_tag(tag_name)] = response
        log("added the row " + tag_name)
        changed = True

    for tag_name in DAY_ROWS:
        if tag_name not in by_name:
            fail("DA_FeedbackResponses has no row " + tag_name)
        tag, response = by_name[tag_name]
        message = str(response.get_editor_property("message"))
        if "{Paid}" in message:
            continue
        # Keep the text's namespace and key (the editor made them), so only its source changes.
        # export_text escapes an apostrophe as \', which the message itself doesn't have.
        match = re.search(r'NSLOCTEXT\("([^"]*)", "([^"]*)", "(.*)"\)', response.export_text())
        if match is None or match.group(3).replace("\\'", "'") != message:
            fail("Can't read the text of the row " + tag_name + ": " + response.export_text())
        response.import_text("(Message={0})".format(
            text_literal(match.group(1), match.group(2), message + PAID_SUFFIX)))
        responses[tag] = response
        log("added {Paid} to the row " + tag_name)
        changed = True

    if not changed:
        log("DA_FeedbackResponses already has the shipping rows and the {Paid} clause")
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
        factory.set_editor_property("parent_class", unreal.FarmShippingBin)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            BLUEPRINT_NAME, BLUEPRINT_FOLDER, unreal.Blueprint, factory)
        if blueprint is None:
            fail("Can't create " + path)
        log("created " + path)
    mesh = load(MESH_PATH)
    shipping_bin = unreal.get_default_object(blueprint.generated_class())
    component = shipping_bin.get_editor_property("bin_mesh")
    if component is None:
        fail("BP_ShippingBin's class has no BinMesh component")
    if component.get_editor_property("static_mesh") == mesh:
        log("BP_ShippingBin's BinMesh already shows SM_ShippingBin")
    else:
        component.modify()
        component.set_editor_property("static_mesh", mesh)
        log("set BinMesh to SM_ShippingBin")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)
    return blueprint


def place_bin(blueprint):
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail("Can't load " + MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    bins = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.FarmShippingBin)]
    if bins:
        log("L_Farm already has {0} shipping bin(s), none placed".format(len(bins)))
        return
    shipping_bin = actors.spawn_actor_from_class(blueprint.generated_class(), BIN_LOCATION, BIN_ROTATION)
    if shipping_bin is None:
        fail("Can't spawn BP_ShippingBin")
    shipping_bin.set_actor_label(BLUEPRINT_NAME)
    log("placed BP_ShippingBin at {0}".format(shipping_bin.get_actor_location()))
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("Can't save " + MAP_PATH)
    log("saved " + MAP_PATH)


def print_facts(blueprint):
    for name in sorted(SELL_PRICES):
        log("fact: {0} sell_price = {1}".format(name, load(ITEM_FOLDER + name).get_editor_property("sell_price")))
    shipping_bin = unreal.get_default_object(blueprint.generated_class())
    mesh = shipping_bin.get_editor_property("bin_mesh").get_editor_property("static_mesh")
    log("fact: BP_ShippingBin BinMesh = {0}".format(mesh.get_path_name() if mesh else "None"))
    responses = load(FEEDBACK_RESPONSES).get_editor_property("responses")
    for tag, response in responses.items():
        name = str(unreal.GameplayTagLibrary.get_tag_name(tag))
        if name in FEEDBACK_ROWS or name in DAY_ROWS:
            log("fact: DA_FeedbackResponses {0} = {1}".format(name, response.export_text()))
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.FarmShippingBin):
            log("fact: L_Farm bin {0} at {1}, yaw {2}".format(
                actor.get_actor_label(), actor.get_actor_location(), actor.get_actor_rotation().yaw))


def main():
    set_up_prices()
    add_feedback_rows()
    blueprint = set_up_blueprint()
    place_bin(blueprint)
    print_facts(blueprint)
    log("done")


if __name__ == "__main__":
    main()
