# Creates / rebuilds every IA_* and IMC_* asset from spec chapter 10. Safe to re-run.
# Run in editor: exec(open(r"D:/UnrealEngine/AccessedNone/Scripts/Editor/create_input_assets.py", encoding="utf-8").read())
import unreal

FOLDER = "/Game/Core/Input"
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
settings = unreal.get_default_object(unreal.load_class(None, "/Script/AccessedNone.AccessedNoneSettings"))
TAP_HOLD = settings.get_editor_property("dash_hold_threshold")

BOOL = unreal.InputActionValueType.BOOLEAN
AXIS2D = unreal.InputActionValueType.AXIS2D


def get_or_create(name, cls, factory):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return tools.create_asset(name, FOLDER, cls, factory)


def action(name, value_type=BOOL):
    ia = get_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", value_type)
    return ia


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


def mapping(imc, ia, key_name, triggers=(), modifiers=()):
    m = unreal.EnhancedActionKeyMapping()
    m.set_editor_property("action", ia)
    m.set_editor_property("key", key(key_name))
    m.set_editor_property("triggers", list(triggers))
    m.set_editor_property("modifiers", list(modifiers))
    return m


def new(cls, outer, **props):
    obj = unreal.new_object(cls, outer)
    for k, v in props.items():
        obj.set_editor_property(k, v)
    return obj


def context(name, build):
    imc = get_or_create(name, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    data = imc.get_editor_property("default_key_mappings")
    data.set_editor_property("mappings", build(imc))
    imc.set_editor_property("default_key_mappings", data)
    return imc


ia = {n: action(n) for n in [
    "IA_Dodge", "IA_Dash", "IA_Jump", "IA_LockOn", "IA_ModifierShift", "IA_LightAttack", "IA_HeavyAttack",
    "IA_UseItem", "IA_Interact", "IA_Menu", "IA_Console", "IA_ConsoleSubmit", "IA_ConsoleAutocomplete"]}
ia["IA_Move"] = action("IA_Move", AXIS2D)
ia["IA_Look"] = action("IA_Look", AXIS2D)

pressed = lambda o: new(unreal.InputTriggerPressed, o)
swizzle = lambda o: new(unreal.InputModifierSwizzleAxis, o)
negate = lambda o: new(unreal.InputModifierNegate, o)
negate_y = lambda o: new(unreal.InputModifierNegate, o, x=False, y=True, z=False)


def gameplay(o):
    return [
        mapping(o, ia["IA_Move"], "W", modifiers=[swizzle(o)]),
        mapping(o, ia["IA_Move"], "S", modifiers=[swizzle(o), negate(o)]),
        mapping(o, ia["IA_Move"], "A", modifiers=[negate(o)]),
        mapping(o, ia["IA_Move"], "D"),
        mapping(o, ia["IA_Look"], "Mouse2D", modifiers=[negate_y(o)]),
        mapping(o, ia["IA_Dodge"], "SpaceBar", triggers=[new(unreal.InputTriggerTap, o, tap_release_time_threshold=TAP_HOLD)]),
        mapping(o, ia["IA_Dash"], "SpaceBar", triggers=[new(unreal.InputTriggerHold, o, hold_time_threshold=TAP_HOLD)]),
        mapping(o, ia["IA_Jump"], "F", triggers=[pressed(o)]),
        mapping(o, ia["IA_LockOn"], "MiddleMouseButton", triggers=[pressed(o)]),
        mapping(o, ia["IA_ModifierShift"], "LeftShift", triggers=[new(unreal.InputTriggerDown, o)]),
        mapping(o, ia["IA_LightAttack"], "LeftMouseButton", triggers=[pressed(o), new(unreal.InputTriggerChordBlocker, o, chord_action=ia["IA_ModifierShift"])]),
        mapping(o, ia["IA_HeavyAttack"], "LeftMouseButton", triggers=[pressed(o), new(unreal.InputTriggerChordAction, o, chord_action=ia["IA_ModifierShift"])]),
        mapping(o, ia["IA_UseItem"], "R", triggers=[pressed(o)]),
        mapping(o, ia["IA_Interact"], "E", triggers=[pressed(o)]),
        mapping(o, ia["IA_Menu"], "Escape", triggers=[pressed(o)]),
        mapping(o, ia["IA_Console"], "Tilde", triggers=[pressed(o)]),
    ]


context("IMC_Gameplay", gameplay)
context("IMC_Menu", lambda o: [mapping(o, ia["IA_Menu"], "Escape", triggers=[pressed(o)])])
context("IMC_UIManipulation", lambda o: [])
context("IMC_Console", lambda o: [
    mapping(o, ia["IA_Console"], "Tilde", triggers=[pressed(o)]),
    mapping(o, ia["IA_ConsoleSubmit"], "Enter", triggers=[pressed(o)]),
    mapping(o, ia["IA_ConsoleAutocomplete"], "Tab", triggers=[pressed(o)]),
])

eal.save_directory(FOLDER, only_if_is_dirty=False)
print("input assets:", sorted(str(p) for p in eal.list_assets(FOLDER)))
