# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up Hob's placeholder body and place Hob (Docs/Systems/10-Robots.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpHobBody.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

1. M_HobEyes (/Game/Vestige/Materials): an emissive cyan that blinks, driven by the material's time (no tick), so Hob is found
   (parameters EyeColor, EyeStrength and BlinkPeriod, set on every run; the eyes cast no shadow),
   after dark. The cyan is @DunderGG's choice (2026-11-25), visibly greener than water's azure 58AEFF.
2. MI_Hob_Cream and MI_Hob_Olive: instances of M_Prop, the paint and the panels.
3. BP_Hob (/Game/Vestige/Characters, parent AVestigeNpcCharacter): CharacterId VestigeCharacter:Base.Hob, and the body from engine
   basic shapes: a body block with an olive band, a boxy head with a dark face screen and two eyes, about 1.45 m tall. Each part is found by its name and its
   shape, place and material set on every run, so a rerun fixes a part that was placed wrong.
4. L_Farm: places one BP_Hob at X 500, Y -350 (off the field, north of its middle, 6 m from the Player Start, 35 degrees off its
   facing), turned toward the Player Start, if the map has none, and saves the map. Not yet the farmhouse side of the plan: the
   map has no house, and a spot west of the field would be behind the camera (Docs/Systems/10-Robots.md).

It changes and saves those assets and the map, and nothing else. Every line it prints starts with "SetUpHobBody:".
"""

import math

import unreal

MATERIAL_FOLDER = "/Game/Vestige/Materials"
CHARACTER_FOLDER = "/Game/Vestige/Characters"
BLUEPRINT_NAME = "BP_Hob"
MAP_PATH = "/Game/Maps/L_Farm"
HOB_LOCATION = unreal.Vector(500.0, -350.0, 80.0)
PLAYER_START = unreal.Vector(0.0, 0.0, 0.0)

# Linear colours: cream paint F2E6C8 and olive panels 6B7A3A, as the concept art.
CREAM = unreal.LinearColor(0.89, 0.79, 0.58, 1.0)
OLIVE = unreal.LinearColor(0.147, 0.195, 0.042, 1.0)
# A cyan with more green than water's azure (58AEFF): about 00E6C8, as emissive light.
EYE_COLOR = unreal.LinearColor(0.0, 0.8, 0.6, 1.0)
# Low enough that the cyan shows: the centre of a brighter eye reads white, and whiter still at night.
EYE_STRENGTH = 2.5
BLINK_PERIOD_SECONDS = 4.0
EYE_PARAMETERS = {"EyeColor": EYE_COLOR, "EyeStrength": EYE_STRENGTH, "BlinkPeriod": BLINK_PERIOD_SECONDS}
# Small lit parts that glow; they cast no shadow on the face.
NO_SHADOW_PARTS = ("EyeLeft", "EyeRight")
BLINK_FRACTION = 0.04

CUBE = "/Engine/BasicShapes/Cube.Cube"
DARK_METAL = MATERIAL_FOLDER + "/MI_Prop_DarkMetal"

# Relative to the capsule (35 by 75 half height), whose centre is 75 cm above the ground, so z = -75 is the ground. The cube is 100 cm.
PARTS = [
    # name, scale, location, material
    ("Body", (0.5, 0.45, 1.0), (0.0, 0.0, -25.0), MATERIAL_FOLDER + "/MI_Hob_Cream"),
    ("Band", (0.52, 0.47, 0.25), (0.0, 0.0, -35.0), MATERIAL_FOLDER + "/MI_Hob_Olive"),
    ("Head", (0.45, 0.4, 0.45), (0.0, 0.0, 47.5), MATERIAL_FOLDER + "/MI_Hob_Cream"),
    ("FaceScreen", (0.02, 0.34, 0.3), (23.5, 0.0, 47.5), DARK_METAL),
    ("EyeLeft", (0.01, 0.08, 0.04), (25.0, -9.0, 53.0), MATERIAL_FOLDER + "/M_HobEyes"),
    ("EyeRight", (0.01, 0.08, 0.04), (25.0, 9.0, 53.0), MATERIAL_FOLDER + "/M_HobEyes"),
]


def log(message):
    unreal.log("SetUpHobBody: " + message)


def fail(message):
    unreal.log_error("SetUpHobBody: " + message)
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


def set_up_eyes():
    path = MATERIAL_FOLDER + "/M_HobEyes"
    lib = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        material = load(path)
        log(path + " exists")
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_HobEyes", MATERIAL_FOLDER, unreal.Material,
                                                                           unreal.MaterialFactoryNew())
        build_eyes(material)
        log("made " + path)

    # The look is three parameters, set on every run, so the model's eyes can reuse the material and a rerun tunes it.
    for expression in lib.get_material_expressions(material):
        name = str(expression.get_editor_property("parameter_name")) if hasattr(expression, "parameter_name") else ""
        if name in EYE_PARAMETERS:
            expression.set_editor_property("default_value", EYE_PARAMETERS[name])
    lib.recompile_material(material)
    save(material)


def build_eyes(material):
    lib = unreal.MaterialEditingLibrary

    def node(expression_class, x, y):
        return lib.create_material_expression(material, expression_class, x, y)

    def parameter(expression_class, name, x, y):
        expression = node(expression_class, x, y)
        expression.set_editor_property("parameter_name", name)
        return expression

    # blink = 1 - floor(frac(time / period) + fraction): 0 for the last fraction of each period, 1 the rest.
    time = node(unreal.MaterialExpressionTime, -1600, 0)
    period = parameter(unreal.MaterialExpressionScalarParameter, "BlinkPeriod", -1600, 120)
    divide = node(unreal.MaterialExpressionDivide, -1300, 0)
    frac = node(unreal.MaterialExpressionFrac, -1100, 0)
    add = node(unreal.MaterialExpressionAdd, -900, 0)
    add.set_editor_property("const_b", BLINK_FRACTION)
    floor = node(unreal.MaterialExpressionFloor, -700, 0)
    blink = node(unreal.MaterialExpressionOneMinus, -500, 0)
    color = parameter(unreal.MaterialExpressionVectorParameter, "EyeColor", -700, 200)
    glow = node(unreal.MaterialExpressionMultiply, -300, 100)
    strength = parameter(unreal.MaterialExpressionScalarParameter, "EyeStrength", -300, 260)
    lit = node(unreal.MaterialExpressionMultiply, -100, 100)

    lib.connect_material_expressions(time, "", divide, "A")
    lib.connect_material_expressions(period, "", divide, "B")
    lib.connect_material_expressions(divide, "", frac, "")
    lib.connect_material_expressions(frac, "", add, "A")
    lib.connect_material_expressions(add, "", floor, "")
    lib.connect_material_expressions(floor, "", blink, "")
    lib.connect_material_expressions(color, "", glow, "A")
    lib.connect_material_expressions(blink, "", glow, "B")
    lib.connect_material_expressions(glow, "", lit, "A")
    lib.connect_material_expressions(strength, "", lit, "B")
    lib.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def set_up_paint(name, color):
    path = MATERIAL_FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(path + " exists")
        return
    factory = unreal.MaterialInstanceConstantFactoryNew()
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MATERIAL_FOLDER, unreal.MaterialInstanceConstant, factory)
    if instance is None:
        fail("Can't create " + path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, load(MATERIAL_FOLDER + "/M_Prop"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, "BaseColor", color)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "Roughness", 0.5)
    log("made " + path)
    save(instance)


def get_blueprint():
    path = CHARACTER_FOLDER + "/" + BLUEPRINT_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(path + " exists")
        return load(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.VestigeNpcCharacter)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(BLUEPRINT_NAME, CHARACTER_FOLDER, unreal.Blueprint, factory)
    if blueprint is None:
        fail("Can't create " + path)
    log("made " + path)
    return blueprint


def build_body(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    capsule_handle = None
    existing = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        obj = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
        if isinstance(obj, unreal.CapsuleComponent):
            capsule_handle = handle
        elif isinstance(obj, unreal.StaticMeshComponent):
            existing[obj.get_name().split("_GEN_VARIABLE")[0]] = obj
    if capsule_handle is None:
        fail("BP_Hob has no capsule to attach the body to")

    cube = load(CUBE)
    for name, scale, location, material_path in PARTS:
        component = existing.get(name)
        if component is None:
            params = unreal.AddNewSubobjectParams(parent_handle=capsule_handle, new_class=unreal.StaticMeshComponent,
                                                  blueprint_context=blueprint)
            handle, reason = subsystem.add_new_subobject(params)
            if not library.is_handle_valid(handle):
                fail("Can't add " + name + ": " + str(reason))
            subsystem.rename_subobject(handle, name)
            component = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
            log("added " + name)
        # Set every run, so a rerun fixes a part that was placed wrong.
        component.set_editor_property("static_mesh", cube)
        component.set_editor_property("relative_scale3d", unreal.Vector(*scale))
        component.set_editor_property("relative_location", unreal.Vector(*location))
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        component.set_editor_property("cast_shadow", name not in NO_SHADOW_PARTS)
        component.set_material(0, load(material_path))
        log("fact: {0} at {1} scale {2} material {3}".format(
            name, component.get_editor_property("relative_location"), component.get_editor_property("relative_scale3d"),
            component.get_material(0).get_name()))


def set_up_blueprint():
    blueprint = get_blueprint()
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property("character_id", unreal.PrimaryAssetId(unreal.PrimaryAssetType("VestigeCharacter"), "Base.Hob"))
    build_body(blueprint)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)
    return blueprint


def place_hob(blueprint):
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail("Can't load " + MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.VestigeNpcCharacter)]:
        log("L_Farm already has a character, none placed")
        return
    to_start = PLAYER_START - HOB_LOCATION
    yaw = math.degrees(math.atan2(to_start.y, to_start.x))
    hob = actors.spawn_actor_from_class(blueprint.generated_class(), HOB_LOCATION, unreal.Rotator(0.0, 0.0, yaw))
    if hob is None:
        fail("Can't spawn BP_Hob")
    hob.set_actor_label(BLUEPRINT_NAME)
    log("placed BP_Hob at {0}, yaw {1}".format(hob.get_actor_location(), yaw))
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        fail("Can't save " + MAP_PATH)
    log("saved " + MAP_PATH)


def main():
    set_up_eyes()
    set_up_paint("MI_Hob_Cream", CREAM)
    set_up_paint("MI_Hob_Olive", OLIVE)
    place_hob(set_up_blueprint())
    log("done")


if __name__ == "__main__":
    main()
