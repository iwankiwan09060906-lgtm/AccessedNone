# S2: materials, node texture, WBP_* (parent = C++ widget), fake boss, and maps L_Title / L_Prologue_FlowerField / L_Stage1_DarkGrove.
# Run after create_string_table.py and create_audio.py. Re-running rebuilds the maps from scratch.
# Headless: UnrealEditor-Cmd AccessedNone.uproject -run=pythonscript -script=Scripts/Editor/create_stage2_assets.py
import math
import os
import random
import struct
import zlib
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tuning = unreal.get_default_object(unreal.load_class(None, "/Script/AccessedNone.AccessedNoneSettings"))


def cls(name):
    return unreal.load_class(None, f"/Script/AccessedNone.{name}")


def recreate(name, folder, asset_class, factory):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, folder, asset_class, factory)


def custom(mat, code, inputs, out_type, x, y):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", out_type)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    node.set_editor_property("inputs", ins)
    return node


# ---------------------------------------------------------------- T_VoidNodes (hand-made "blueprint node" texture)
def make_node_png(path, size=512):
    random.seed(7)
    px = [[(4, 2, 6)] * size for _ in range(size)]

    def rect(x0, y0, x1, y1, c):
        for y in range(max(0, y0), min(size, y1)):
            row = px[y]
            for x in range(max(0, x0), min(size, x1)):
                row[x] = c

    def line(x0, y0, x1, y1, c):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(steps + 1):
            t = i / steps
            x = int(x0 + (x1 - x0) * t)
            y = int(y0 + (y1 - y0) * (3 * t * t - 2 * t * t * t))  # S-curve wire
            rect(x, y, x + 2, y + 2, c)

    nodes = []
    for _ in range(14):
        w, h = random.randint(70, 130), random.randint(50, 100)
        x, y = random.randint(0, size - w), random.randint(0, size - h)
        nodes.append((x, y, w, h))
    for (x, y, w, h), (x2, y2, w2, h2) in zip(nodes, nodes[1:]):
        line(x + w, y + 30, x2, y2 + 30, (200, 30, 40))
    for x, y, w, h in nodes:
        rect(x, y, x + w, y + h, (40, 8, 12))
        rect(x, y, x + w, y + 14, (230, 36, 59))
        for k in range(3):
            rect(x + 6, y + 24 + k * 18, x + 14, y + 32 + k * 18, (255, 90, 90))
            rect(x + w - 14, y + 24 + k * 18, x + w - 6, y + 32 + k * 18, (255, 90, 90))
    raw = b"".join(b"\x00" + bytes(c for p in row for c in p) for row in px)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


png = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "T_VoidNodes.png")
make_node_png(png)
task = unreal.AssetImportTask()
for k, v in dict(filename=png, destination_path="/Game/Core/Textures", automated=True, replace_existing=True, save=True).items():
    task.set_editor_property(k, v)
tools.import_asset_tasks([task])
node_tex = unreal.load_asset("/Game/Core/Textures/T_VoidNodes")

# ---------------------------------------------------------------- M_WorldGrid (defense entities, spec 4.3)
MAT = "/Game/Core/Materials"
grid = recreate("M_WorldGrid", MAT, unreal.Material, unreal.MaterialFactoryNew())
grid.set_editor_property("used_with_skeletal_mesh", True)
wp = mel.create_material_expression(grid, unreal.MaterialExpressionWorldPosition, -800, 0)
checker = custom(grid, "float3 c = floor(WP / 40.0); float k = fmod(abs(c.x + c.y + c.z), 2.0);"
                       " return lerp(float3(0.102, 0.102, 0.102), float3(0.212, 0.212, 0.212), k);",
                 ["WP"], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0)
mel.connect_material_expressions(wp, "", checker, "WP")
mel.connect_material_property(checker, "", unreal.MaterialProperty.MP_BASE_COLOR)
lp = mel.create_material_expression(grid, unreal.MaterialExpressionLocalPosition, -800, 300)
nrm = mel.create_material_expression(grid, unreal.MaterialExpressionVertexNormalWS, -800, 400)
amt = mel.create_material_expression(grid, unreal.MaterialExpressionScalarParameter, -800, 500)
amt.set_editor_property("parameter_name", "ExplodeAmount")
amt.set_editor_property("default_value", 0.0)
tm = mel.create_material_expression(grid, unreal.MaterialExpressionTime, -800, 600)
explode = custom(grid, "float h = frac(sin(dot(floor(LP * 0.2), float3(12.9898, 78.233, 37.719))) * 43758.5453);"
                       " return N * step(0.985, h) * Amount * 120.0 * (0.6 + 0.4 * sin(T * 3.0 + h * 20.0));",
                 ["LP", "N", "Amount", "T"], unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 400)
for src, pin in ((lp, "LP"), (nrm, "N"), (amt, "Amount"), (tm, "T")):
    mel.connect_material_expressions(src, "", explode, pin)
mel.connect_material_property(explode, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(grid)
eal.save_loaded_asset(grid, False)

# ---------------------------------------------------------------- M_SkyCrack (spec 4.2: purple rim, void with red nodes)
sky = recreate("M_SkyCrack", MAT, unreal.Material, unreal.MaterialFactoryNew())
sky.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
sky.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
sky.set_editor_property("two_sided", True)
sky.set_editor_property("use_translucency_vertex_fog", False)  # the void must stay black, not fogged sky
uv = mel.create_material_expression(sky, unreal.MaterialExpressionTextureCoordinate, -900, 0)
prog = mel.create_material_expression(sky, unreal.MaterialExpressionScalarParameter, -900, 100)
prog.set_editor_property("parameter_name", "CrackProgress")
prog.set_editor_property("default_value", 0.0)
tm2 = mel.create_material_expression(sky, unreal.MaterialExpressionTime, -900, 200)
tex = mel.create_material_expression(sky, unreal.MaterialExpressionTextureSample, -900, 300)
tex.set_editor_property("texture", node_tex)
crack = custom(sky, "float2 d = UV - 0.5; float r = length(d) * 2.0; float a = atan2(d.y, d.x);"
                    " float jag = 0.18 * sin(a * 7.0 + 1.3) + 0.12 * sin(a * 13.0 + 4.1) + 0.07 * sin(a * 29.0);"
                    " float edge = P * (0.75 + jag);"
                    " float inside = step(r, edge);"
                    " float rim = saturate(1.0 - abs(r - edge) * 14.0) * step(0.01, P);"
                    " float pulse = 0.55 + 0.45 * sin(T * 4.0);"
                    " float3 voidc = Nodes * float3(1.0, 0.25, 0.3) * pulse * 3.0;"
                    " float3 col = voidc * inside + float3(0.54, 0.25, 0.99) * rim * 2.5;"
                    " return float4(col, saturate(inside + rim));",
               ["UV", "P", "T", "Nodes"], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -500, 100)
for src, pin in ((uv, "UV"), (prog, "P"), (tm2, "T")):
    mel.connect_material_expressions(src, "", crack, pin)
mel.connect_material_expressions(tex, "RGB", crack, "Nodes")
rgb = mel.create_material_expression(sky, unreal.MaterialExpressionComponentMask, -250, 50)
for c in ("r", "g", "b"):
    rgb.set_editor_property(c, True)
alpha = mel.create_material_expression(sky, unreal.MaterialExpressionComponentMask, -250, 200)
alpha.set_editor_property("a", True)
mel.connect_material_expressions(crack, "", rgb, "")
mel.connect_material_expressions(crack, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(sky)
eal.save_loaded_asset(sky, False)

# ---------------------------------------------------------------- WBP_* (trees are built in C++, spec MCP rule)
UI = "/Game/UI"
for wbp, parent in [("WBP_HUD", "MetaHUDWidget"), ("WBP_BossBar", "MetaBossBarWidget"), ("WBP_Settings", "MetaSettingsWidget"),
                    ("WBP_Debugger", "MetaDebuggerWidget"), ("WBP_ErrorWindow", "MetaErrorWindowWidget"),
                    ("WBP_DraggableNumber", "MetaDraggableNumberWidget"), ("WBP_BossBar_Overflow", "MetaBossBarOverflowWidget"),
                    ("WBP_Title", "MetaTitleWidget")]:
    f = unreal.WidgetBlueprintFactory()
    f.set_editor_property("parent_class", cls(parent))
    bp = recreate(wbp, UI, unreal.WidgetBlueprint, f)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, False)

# ---------------------------------------------------------------- Fake boss Elbern (spec 4.1 / 3.4)
BOSS = "/Game/Characters/Bosses/Elbern"


def pattern(pid, max_range, windup, active, recovery, cooldown, damage, weight=1.0):
    p = unreal.BossPattern()
    for k, v in dict(pattern_id=pid, min_range=0.0, max_range=max_range, windup=windup, active=active,
                     recovery=recovery, cooldown=cooldown, weight=weight, fixed_damage=damage).items():
        p.set_editor_property(k, v)
    return p


f = unreal.DataAssetFactory()
f.set_editor_property("data_asset_class", cls("BossDataAsset"))
da = eal.load_asset(f"{BOSS}/DA_Boss_Elbern") if eal.does_asset_exist(f"{BOSS}/DA_Boss_Elbern") else tools.create_asset("DA_Boss_Elbern", BOSS, None, f)
dmg = tuning.get_editor_property("fake_boss_damage")
da.set_editor_property("display_name_key", "OP_P1_BOSS_NAME")
da.set_editor_property("debug_name", "BP_FakeBoss_Elbern_C")
da.set_editor_property("hp_multiplier_override", tuning.get_editor_property("fake_boss_hp_multiplier"))
da.set_editor_property("armored_effects", [])
da.set_editor_property("pattern_set_armored", [pattern("Slash", 260.0, 1.0, 0.2, 1.2, 2.5, dmg, 2.0),
                                               pattern("Smash", 320.0, 1.5, 0.25, 1.5, 5.0, dmg, 1.0)])
da.set_editor_property("pattern_set_vulnerable", [])
da.set_editor_property("mesh", unreal.load_asset("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog"))
da.set_editor_property("anim_class", unreal.load_class(None, "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sevarog_AnimBlueprint.Sevarog_AnimBlueprint_C"))
da.set_editor_property("mesh_scale", 1.0)
eal.save_loaded_asset(da, False)

bp_path = f"{BOSS}/BP_FakeBoss_Elbern"
if not eal.does_asset_exist(bp_path):
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", cls("MetaBossBase"))
    tools.create_asset("BP_FakeBoss_Elbern", BOSS, unreal.Blueprint, f)
unreal.BlueprintEditorLibrary.compile_blueprint(eal.load_asset(bp_path))
boss_class = eal.load_blueprint_class(bp_path)
unreal.get_default_object(boss_class).set_editor_property("boss_data", da)
eal.save_asset(bp_path)

# ---------------------------------------------------------------- Maps
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def fresh_level(path, game_mode):
    if eal.does_asset_exist(path):
        les.load_level(path)
        for a in eas.get_all_level_actors():
            if not isinstance(a, unreal.WorldSettings):
                eas.destroy_actor(a)
    else:
        les.new_level(path)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", cls(game_mode))


def spawn(actor_class, loc, rot=(0, 0, 0), scale=None, label=None):
    a = eas.spawn_actor_from_class(actor_class, unreal.Vector(*loc), unreal.Rotator(*rot))
    if scale:
        a.set_actor_scale3d(unreal.Vector(*scale))
    if label:
        a.set_actor_label(label)
    return a


def mesh_actor(mesh, loc, rot=(0, 0, 0), scale=(1, 1, 1), label=None, material=None):
    a = spawn(unreal.StaticMeshActor, loc, rot, scale, label)
    a.static_mesh_component.set_static_mesh(unreal.load_asset(mesh))
    if material:
        a.static_mesh_component.set_material(0, unreal.load_asset(material))
    return a


def scatter(mesh, count, radius, inner=0.0, scale=(0.8, 1.2), seed=1, z=0.0):
    # Instanced rendering needs the usage flag, or the asset-pack material falls back to the default checker.
    for slot in unreal.load_asset(mesh).static_materials:
        base = slot.material_interface.get_base_material() if slot.material_interface else None
        if base:
            unreal.MaterialEditingLibrary.set_material_usage(base, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
            eal.save_loaded_asset(base, False)
    a = spawn(cls("MetaScatterField"), (0, 0, z), label=f"Scatter_{mesh.split('/')[-1]}")
    for k, v in dict(mesh=unreal.load_asset(mesh), count=count, radius=radius, inner_radius=inner,
                     scale_range=unreal.Vector2D(*scale), seed=seed).items():
        a.set_editor_property(k, v)
    return a


def save_level():
    les.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


# L_Title: the widget covers the screen; nothing else needed.
fresh_level("/Game/Maps/L_Title", "MetaTitleGameMode")
save_level()

# L_Prologue_FlowerField: moonlit white flower field (spec 4.1).
fresh_level("/Game/Maps/L_Prologue_FlowerField", "MetaGameMode")
mesh_actor("/Engine/BasicShapes/Plane", (0, 0, 0), scale=(120, 120, 1), label="Ground",
           material="/Game/InfinityBladeGrassLands/Environments/Plains/Env_Plains_Terrain/Materials/M_Grass01")
moon = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -28, 40), label="Moonlight")
moon.light_component.set_intensity(1.5)
moon.light_component.set_light_color(unreal.LinearColor(0.55, 0.65, 1.0, 1.0))
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
sl = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
sl.light_component.set_intensity(0.6)
sl.light_component.set_editor_property("real_time_capture", True)
spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")
scatter("/Game/InfinityBladeIceLands/Environments/Ice/DR_Ice_Mountain/StaticMesh/SM_Snowy_Flowers", 1400, 5000, 0, (0.35, 0.6), 3)
for i, n in enumerate(("01", "02", "03", "04")):
    scatter(f"/Game/InfinityBladeGrassLands/Environments/Plains/Env_Plains_Flora/StaticMesh/SM_Plains_Flowers{n}", 200, 5000, 0, (0.4, 0.7), 10 + i)
scatter("/Game/InfinityBladeGrassLands/Environments/Plains/Env_Plains_Flora/StaticMesh/SM_Plains_Grass01", 400, 5500, 0, (0.2, 0.35), 20)
spawn(unreal.PlayerStart, (-2600, 0, 120), label="PlayerStart")
boss = spawn(boss_class, (900, 0, 120), rot=(0, 180, 0), label="FakeBoss_Elbern")
trigger = spawn(cls("EncounterTriggerBox"), (900, 0, 120), scale=(30, 30, 6), label="Trigger_P1")
trigger.set_editor_property("encounter_id", "P1")
trigger.set_editor_property("bosses", [boss])
holy = spawn(unreal.PointLight, (900, 0, 700), label="HolyLight")
holy.light_component.set_light_color(unreal.LinearColor(0.95, 0.82, 0.48, 1.0))  # #F2D27A
holy.light_component.set_editor_property("attenuation_radius", 4000.0)
holy.light_component.set_intensity(0.0)
holy.tags = ["HolyLight"]
crack_plane = mesh_actor("/Engine/BasicShapes/Plane", (5000, 0, 3800), rot=(90, 0, -90), scale=(70, 70, 1), label="SkyCrack",
                         material="/Game/Core/Materials/M_SkyCrack")
crack_plane.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
crack_plane.tags = ["SkyCrack"]
spawn(cls("PrologueDirector"), (0, 0, 200), label="PrologueDirector")
save_level()

# Stage 1 placeholder trees are instanced too (the map itself is only created once).
for slot in unreal.load_asset("/Game/InfinityBladeGrassLands/Environments/Plains/Env_Plains_Flora/StaticMesh/SM_Plains_Tree01").static_materials:
    unreal.MaterialEditingLibrary.set_material_usage(slot.material_interface.get_base_material(), unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    eal.save_loaded_asset(slot.material_interface.get_base_material(), False)

# L_Stage1_DarkGrove: placeholder for Stage 3 (spec S2-T10 "create an empty level if missing").
if not eal.does_asset_exist("/Game/Maps/L_Stage1_DarkGrove"):
    fresh_level("/Game/Maps/L_Stage1_DarkGrove", "MetaGameMode")
    mesh_actor("/Engine/BasicShapes/Plane", (0, 0, 0), scale=(100, 100, 1), label="Ground",
               material="/Game/EnvironmentPack2/Materials/M_Grass_Asian_Dark")
    d = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -40, 20), label="DimLight")
    d.light_component.set_intensity(0.8)
    spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
    s = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
    s.light_component.set_editor_property("real_time_capture", True)
    spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")
    scatter("/Game/InfinityBladeGrassLands/Environments/Plains/Env_Plains_Flora/StaticMesh/SM_Plains_Tree01", 60, 6000, 1200, (0.8, 1.3), 5)
    spawn(unreal.PlayerStart, (0, 0, 120), label="PlayerStart")
    save_level()

print("stage2 assets done")
