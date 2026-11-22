# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up the seed shop content (Docs/Systems/26-Shops.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpSeedShop.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

1. DA_Shop_Farm (/Game/Vestige/Shops): makes it if missing, with ContentId Base.FarmShop, display name "Farm Shop" and the
   offers Base.TurnipSeeds 10, Base.PotatoSeeds 20, Base.CabbageSeeds 40, in that order.
2. DA_FeedbackResponses: adds the rows Vestige.Feedback.Shop.Bought and .NotEnoughMoney (no sound). A row that exists is
   kept as it is, with a warning when its text differs.
3. BP_ShopDispenser (/Game/Vestige/Blueprints, parent AFarmShopDispenser): makes it if missing, sets ShopId to
   VestigeShop:Base.FarmShop and DispenserMesh to SM_ShopDispenser at scale 1 and location 0 when that mesh exists, otherwise
   to the stand-in /Engine/BasicShapes/Cube at scale (0.9, 0.9, 1.5) and location Z 75 (the cube is 100 cm with its pivot at
   its centre). Then its children BP_ShopDispenser_TurnipSeeds, _PotatoSeeds and _CabbageSeeds, each with its ItemId, and the
   same mesh settings, so a rerun after the real mesh is imported reaches them too.
4. L_Farm: places the three children at X 1880, Y 900 / 1000 / 1100, Z 0, yaw 180 if the map has no dispenser, and saves
   the map.

It changes and saves those assets and the map, and nothing else. Run it again at any time, and after SM_ShopDispenser is
imported: it only writes what differs. Every line it prints starts with "SetUpSeedShop:".
"""

import unreal

SHOP_FOLDER = "/Game/Vestige/Shops"
SHOP_NAME = "DA_Shop_Farm"
SHOP_CONTENT_ID = "Base.FarmShop"
SHOP_DISPLAY_NAME = "Farm Shop"
# The prices: the game designer's (Docs/GameDesign.md, section 6 and section 9, Economy), in the order a shop screen shows.
OFFERS = [
    ("Base.TurnipSeeds", 10),
    ("Base.PotatoSeeds", 20),
    ("Base.CabbageSeeds", 40),
]

FEEDBACK_RESPONSES = "/Game/Vestige/UI/DA_FeedbackResponses"
TEXT_NAMESPACE = "VestigeFeedback"
# The texts are the ones in Docs/Systems/26-Shops.md's feedback table; keep them in step.
FEEDBACK_ROWS = {
    "Vestige.Feedback.Shop.Bought": "Bought: {Item} ×{Count} for {Price}.",
    "Vestige.Feedback.Shop.NotEnoughMoney": "Not enough coins. These seeds cost {Price}.",
}

BLUEPRINT_FOLDER = "/Game/Vestige/Blueprints"
BLUEPRINT_NAME = "BP_ShopDispenser"
# Each child: its name suffix, the item it sells and its place in L_Farm (Y, at X 1880, Z 0, yaw 180).
CHILDREN = [
    ("TurnipSeeds", "Base.TurnipSeeds", 900.0),
    ("PotatoSeeds", "Base.PotatoSeeds", 1000.0),
    ("CabbageSeeds", "Base.CabbageSeeds", 1100.0),
]
MESH_PATH = "/Game/Vestige/Meshes/SM_ShopDispenser"
STAND_IN_PATH = "/Engine/BasicShapes/Cube"
# The stand-in cube is 100 cm with its pivot at its centre; this fills the 0.9 by 0.9 by 1.5 m floor-pivot cabinet.
STAND_IN_SCALE = unreal.Vector(0.9, 0.9, 1.5)
STAND_IN_LOCATION = unreal.Vector(0.0, 0.0, 75.0)
MESH_SCALE = unreal.Vector(1.0, 1.0, 1.0)
MESH_LOCATION = unreal.Vector(0.0, 0.0, 0.0)

MAP_PATH = "/Game/Maps/L_Farm"
ROW_X = 1880.0
ROW_ROTATION = unreal.Rotator(0.0, 0.0, 180.0)


def log(message):
    unreal.log("SetUpSeedShop: " + message)


def fail(message):
    unreal.log_error("SetUpSeedShop: " + message)
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


def make_id(asset_type, name):
    return unreal.PrimaryAssetId(unreal.PrimaryAssetType(asset_type), name)


def id_text(asset_id):
    return "{0}:{1}".format(asset_id.get_editor_property("primary_asset_type").get_editor_property("name"),
                            asset_id.get_editor_property("primary_asset_name"))


def offers_text(offers):
    return ", ".join("{0} {1}".format(id_text(offer.get_editor_property("item_id")), offer.get_editor_property("price"))
                     for offer in offers)


def set_up_shop():
    path = SHOP_FOLDER + "/" + SHOP_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        shop = load(path)
        log(path + " exists")
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.VestigeShopData)
        shop = unreal.AssetToolsHelpers.get_asset_tools().create_asset(SHOP_NAME, SHOP_FOLDER, unreal.VestigeShopData, factory)
        if shop is None:
            fail("Can't create " + path)
        log("created " + path)

    changed = False
    if str(shop.get_editor_property("content_id")) != SHOP_CONTENT_ID:
        shop.modify()
        shop.set_editor_property("content_id", SHOP_CONTENT_ID)
        log("set the ContentId to " + SHOP_CONTENT_ID)
        changed = True
    if str(shop.get_editor_property("display_name")) != SHOP_DISPLAY_NAME:
        shop.modify()
        shop.set_editor_property("display_name", SHOP_DISPLAY_NAME)
        log("set the display name to " + SHOP_DISPLAY_NAME)
        changed = True

    wanted = []
    for item_name, price in OFFERS:
        offer = unreal.VestigeShopOffer()
        offer.set_editor_property("item_id", make_id("VestigeItem", item_name))
        offer.set_editor_property("price", price)
        wanted.append(offer)
    if offers_text(shop.get_editor_property("offers")) != offers_text(wanted):
        shop.modify()
        shop.set_editor_property("offers", wanted)
        log("set the offers to " + offers_text(wanted))
        changed = True

    if changed:
        save(shop)
    else:
        log(SHOP_NAME + " already has its ID, name and offers")


def add_feedback_rows():
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    by_name = {}
    for tag, response in responses.items():
        by_name[str(unreal.GameplayTagLibrary.get_tag_name(tag))] = response
    changed = False

    for tag_name, text in sorted(FEEDBACK_ROWS.items()):
        if tag_name in by_name:
            if str(by_name[tag_name].get_editor_property("message")) != text:
                unreal.log_warning("SetUpSeedShop: kept the row " + tag_name + " as it is; its text differs from the designer's")
            continue
        response = unreal.VestigeFeedbackResponse()
        key = tag_name[len("Vestige.Feedback."):]
        response.import_text("(Message={0})".format(text_literal(TEXT_NAMESPACE, key, text)))
        responses[make_tag(tag_name)] = response
        log("added the row " + tag_name)
        changed = True

    if not changed:
        log("DA_FeedbackResponses already has the shop rows")
        return
    asset.modify()
    asset.set_editor_property("responses", responses)
    save(asset)


def find_or_create_blueprint(name, parent_class):
    path = BLUEPRINT_FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(path + " exists")
        return load(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BLUEPRINT_FOLDER, unreal.Blueprint, factory)
    if blueprint is None:
        fail("Can't create " + path)
    log("created " + path)
    return blueprint


def pick_mesh():
    """The real mesh at scale 1 and location 0 when it is imported, otherwise the stand-in cube."""
    if unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH):
        log("using " + MESH_PATH)
        return load(MESH_PATH), MESH_SCALE, MESH_LOCATION
    log("no " + MESH_PATH + " yet, so the stand-in " + STAND_IN_PATH + " is used")
    return load(STAND_IN_PATH), STAND_IN_SCALE, STAND_IN_LOCATION


def set_property(target, name, value, label):
    if target.get_editor_property(name) == value:
        return False
    target.modify()
    target.set_editor_property(name, value)
    log("set {0} {1} to {2}".format(label, name, value))
    return True


def set_up_mesh(blueprint, label, mesh, scale, location):
    dispenser = unreal.get_default_object(blueprint.generated_class())
    component = dispenser.get_editor_property("dispenser_mesh")
    if component is None:
        fail(label + "'s class has no DispenserMesh component")
    changed = set_property(component, "static_mesh", mesh, label)
    changed = set_property(component, "relative_scale3d", scale, label) or changed
    changed = set_property(component, "relative_location", location, label) or changed
    return changed


def set_up_id(blueprint, label, property_name, asset_id):
    dispenser = unreal.get_default_object(blueprint.generated_class())
    if id_text(dispenser.get_editor_property(property_name)) == id_text(asset_id):
        return False
    dispenser.modify()
    dispenser.set_editor_property(property_name, asset_id)
    log("set {0} {1} to {2}".format(label, property_name, id_text(asset_id)))
    return True


def compile_and_save(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)


def set_up_blueprints():
    mesh, scale, location = pick_mesh()
    parent = find_or_create_blueprint(BLUEPRINT_NAME, unreal.FarmShopDispenser)
    set_up_id(parent, BLUEPRINT_NAME, "shop_id", make_id("VestigeShop", SHOP_CONTENT_ID))
    set_up_mesh(parent, BLUEPRINT_NAME, mesh, scale, location)
    compile_and_save(parent)

    children = []
    for suffix, item_name, y in CHILDREN:
        name = BLUEPRINT_NAME + "_" + suffix
        child = find_or_create_blueprint(name, parent.generated_class())
        set_up_id(child, name, "item_id", make_id("VestigeItem", item_name))
        # Set on each child too: an inherited component's value isn't always carried to an existing child by a script.
        set_up_mesh(child, name, mesh, scale, location)
        compile_and_save(child)
        children.append((name, child, y))
    return children


def place_dispensers(children):
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail("Can't load " + MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    dispensers = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.FarmShopDispenser)]
    if dispensers:
        log("L_Farm already has {0} dispenser(s), none placed".format(len(dispensers)))
        return
    for name, child, y in children:
        dispenser = actors.spawn_actor_from_class(child.generated_class(), unreal.Vector(ROW_X, y, 0.0), ROW_ROTATION)
        if dispenser is None:
            fail("Can't spawn " + name)
        dispenser.set_actor_label(name)
        log("placed {0} at {1}".format(name, dispenser.get_actor_location()))
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("Can't save " + MAP_PATH)
    log("saved " + MAP_PATH)


def print_facts(children):
    shop = load(SHOP_FOLDER + "/" + SHOP_NAME)
    log("fact: {0} content_id = {1}, display_name = {2}, offers = {3}".format(
        SHOP_NAME, shop.get_editor_property("content_id"), shop.get_editor_property("display_name"),
        offers_text(shop.get_editor_property("offers"))))
    responses = load(FEEDBACK_RESPONSES).get_editor_property("responses")
    for tag, response in responses.items():
        name = str(unreal.GameplayTagLibrary.get_tag_name(tag))
        if name in FEEDBACK_ROWS:
            log("fact: DA_FeedbackResponses {0} = {1}".format(name, response.export_text()))
    for name, child, y in children:
        dispenser = unreal.get_default_object(child.generated_class())
        component = dispenser.get_editor_property("dispenser_mesh")
        mesh = component.get_editor_property("static_mesh")
        log("fact: {0} shop_id = {1}, item_id = {2}, mesh = {3}, scale = {4}, location = {5}".format(
            name, id_text(dispenser.get_editor_property("shop_id")), id_text(dispenser.get_editor_property("item_id")),
            mesh.get_path_name() if mesh else "None", component.get_editor_property("relative_scale3d"),
            component.get_editor_property("relative_location")))
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.FarmShopDispenser):
            log("fact: L_Farm dispenser {0} ({1}) at {2}, yaw {3}".format(
                actor.get_actor_label(), actor.get_class().get_name(), actor.get_actor_location(),
                actor.get_actor_rotation().yaw))


def main():
    set_up_shop()
    add_feedback_rows()
    children = set_up_blueprints()
    place_dispensers(children)
    print_facts(children)
    log("done")


if __name__ == "__main__":
    main()
