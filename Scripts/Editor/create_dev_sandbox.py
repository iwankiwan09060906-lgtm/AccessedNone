# S1-T08: DA_Boss_Test, BP_TestBoss and L_Dev_Sandbox. Re-running rebuilds the level from scratch.
# Headless: UnrealEditor-Cmd AccessedNone.uproject -run=pythonscript -script=Scripts/Editor/create_dev_sandbox.py
import unreal

DEV = "/Game/Dev"
MAP = "/Game/Maps/L_Dev_Sandbox"
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
tuning = unreal.get_default_object(unreal.load_class(None, "/Script/AccessedNone.AccessedNoneSettings"))


def cls(name):
    return unreal.load_class(None, f"/Script/AccessedNone.{name}")


def pattern(pid, max_range, windup, active, recovery, cooldown, damage, weight=1.0):
    p = unreal.BossPattern()
    for k, v in dict(pattern_id=pid, min_range=0.0, max_range=max_range, windup=windup, active=active,
                     recovery=recovery, cooldown=cooldown, weight=weight, fixed_damage=damage).items():
        p.set_editor_property(k, v)
    return p


# --- DA_Boss_Test ---
da_path = f"{DEV}/DA_Boss_Test"
if eal.does_asset_exist(da_path):
    da = eal.load_asset(da_path)
else:
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", cls("BossDataAsset"))
    da = tools.create_asset("DA_Boss_Test", DEV, None, f)
normal, heavy = tuning.get_editor_property("boss_normal_damage"), tuning.get_editor_property("boss_heavy_damage")
da.set_editor_property("debug_name", "BP_TestBoss_C")
da.set_editor_property("armored_effects", [cls("GE_Boss_Armored")])
da.set_editor_property("pattern_set_armored", [
    pattern("Slash", 250.0, 0.8, 0.2, 1.0, 2.0, normal, 2.0),
    pattern("Smash", 300.0, 1.4, 0.2, 1.4, 5.0, heavy, 1.0),
])
da.set_editor_property("pattern_set_vulnerable", [
    pattern("Slash", 250.0, 1.0, 0.2, 2.0, 3.0, normal),
])
eal.save_loaded_asset(da)

# --- BP_TestBoss ---
bp_path = f"{DEV}/BP_TestBoss"
if not eal.does_asset_exist(bp_path):
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", cls("DevTestBoss"))
    tools.create_asset("BP_TestBoss", DEV, unreal.Blueprint, f)
bp = eal.load_asset(bp_path)
# Compile first: compiling later (e.g. on spawn) regenerates the CDO and the placed instance would save BossData=None.
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
bp_class = eal.load_blueprint_class(bp_path)
unreal.get_default_object(bp_class).set_editor_property("boss_data", da)
eal.save_asset(bp_path)

# --- L_Dev_Sandbox ---
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if eal.does_asset_exist(MAP):
    les.load_level(MAP)
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.WorldSettings):
            eas.destroy_actor(a)
else:
    les.new_level(MAP)


def spawn(actor_class, loc, rot=(0, 0, 0), scale=None, label=None):
    a = eas.spawn_actor_from_class(actor_class, unreal.Vector(*loc), unreal.Rotator(*rot))
    if scale:
        a.set_actor_scale3d(unreal.Vector(*scale))
    if label:
        a.set_actor_label(label)
    return a


floor = spawn(unreal.StaticMeshActor, (0, 0, -50), scale=(60, 60, 1), label="Floor")
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -45, 30), label="Sun")
spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

boss = spawn(bp_class, (1200, 0, 100), rot=(0, 0, 180), label="TestBoss")
if boss.get_editor_property("boss_data") != da:
    raise RuntimeError("TestBoss instance lost BossData")
trigger = spawn(cls("EncounterTriggerBox"), (1200, 0, 100), scale=(12, 12, 4), label="Trigger_Dev")
trigger.set_editor_property("encounter_id", "Dev")
trigger.set_editor_property("bosses", [boss])

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", cls("MetaGameMode"))

les.save_current_level()
# The map may store actors as external packages (one file per actor); save those too.
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("sandbox:", MAP, "actors:", len(eas.get_all_level_actors()))
