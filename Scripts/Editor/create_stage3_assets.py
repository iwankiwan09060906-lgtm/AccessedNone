# S3: E1 assassin pack + E2 thorn knight data/BPs, DA_Hint_E1/E2, and L_Stage1_DarkGrove built from InfinityBladeIceLands
# (camp, rune-stone gate, frozen dead grove for E1, long cliff pass, open ice-fortress plaza for E2, two breakpoints).
# Re-running rebuilds the level. Run after create_string_table.py.
# Headless: UnrealEditor-Cmd AccessedNone.uproject -run=pythonscript -script=Scripts/Editor/create_stage3_assets.py
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
tuning = unreal.get_default_object(unreal.load_class(None, "/Script/AccessedNone.AccessedNoneSettings"))


def cls(name):
    return unreal.load_class(None, f"/Script/AccessedNone.{name}")


def data_asset(folder, name, klass):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", cls(klass))
    return tools.create_asset(name, folder, None, f)


def pattern(pid, max_range, windup, active, recovery, cooldown, damage, weight=1.0, montage=None, instakill=False):
    p = unreal.BossPattern()
    for k, v in dict(pattern_id=pid, min_range=0.0, max_range=max_range, windup=windup, active=active, recovery=recovery,
                     cooldown=cooldown, weight=weight, fixed_damage=damage, instant_kill=instakill).items():
        p.set_editor_property(k, v)
    if montage:
        p.set_editor_property("optional_montage", unreal.load_asset(montage))
    return p


def boss_bp(folder, name, parent, da):
    path = f"{folder}/{name}"
    if not eal.does_asset_exist(path):
        f = unreal.BlueprintFactory()
        f.set_editor_property("parent_class", cls(parent))
        tools.create_asset(name, folder, unreal.Blueprint, f)
    unreal.BlueprintEditorLibrary.compile_blueprint(eal.load_asset(path))
    klass = eal.load_blueprint_class(path)
    unreal.get_default_object(klass).set_editor_property("boss_data", da)
    eal.save_asset(path)
    return klass


def hero(h, rel):
    return f"/Game/Paragon{h}/Characters/Heroes/{h}/{rel}"


# ---------------------------------------------------------------- Hints (spec 11.4)
GIM = "/Game/Gimmicks"
hints = {}
for e in ("E1", "E2"):
    h = data_asset(GIM, f"DA_Hint_{e}", "HintDataAsset")
    h.set_editor_property("tier3_key", f"HINT_{e}_3")
    h.set_editor_property("tier5_key", f"HINT_{e}_5")
    eal.save_loaded_asset(h, False)
    hints[e] = h

normal, heavy, skill = (tuning.get_editor_property(k) for k in ("boss_normal_damage", "boss_heavy_damage", "boss_skill_damage"))
GAMMA, MESHQ = unreal.MetaSettingKey.GAMMA, unreal.MetaSettingKey.MESH_QUALITY

# ---------------------------------------------------------------- E1 assassin (Paragon Sevarog, hooded)
SV = hero("Sevarog", "Animations/")
da1 = data_asset(GIM, "DA_Boss_Assassin", "BossDataAsset")
for k, v in dict(display_name_key="E1_NAME", debug_name="BP_Assassin_C", is_minion=True, hp_multiplier_override=0.0,
                 armored_effects=[cls("GE_Boss_Invulnerable")],
                 pattern_set_armored=[pattern("Ambush", 230.0, 0.5, 0.15, 1.2, 3.0, skill, 1.0, SV + "Swing1_Medium_Montage", True)],
                 # Revealed, they fight back (user revision): the pack must be beaten in melee.
                 pattern_set_vulnerable=[pattern("Slash", 230.0, 0.8, 0.2, 1.0, 2.2, normal, 2.0, SV + "Swing2_Medium_Montage"),
                                         pattern("Cleave", 300.0, 1.1, 0.25, 1.3, 4.5, normal, 1.0, SV + "Swing1_Medium_Montage")],
                 mesh=unreal.load_asset(hero("Sevarog", "Meshes/Sevarog")),
                 anim_class=unreal.load_class(None, hero("Sevarog", "Sevarog_AnimBlueprint.Sevarog_AnimBlueprint_C")),
                 mesh_scale=1.0, body_scale=1.0, death_anim=unreal.load_asset(SV + "Death_front"), hint_asset=hints["E1"],
                 restore_settings_on_defeat={GAMMA: 0.5}, restore_delay=0.0, restore_seconds=2.0, clear_message_key="E1_CLEAR").items():
    da1.set_editor_property(k, v)
eal.save_loaded_asset(da1, False)
assassin_class = boss_bp("/Game/Characters/Bosses/Assassin", "BP_Assassin", "AssassinBoss", da1)

# ---------------------------------------------------------------- E2 thorn knight (Paragon Grux, spiked) — boss = 3x body
GX = hero("Grux", "Animations/")
da2 = data_asset(GIM, "DA_Boss_ThornKnight", "BossDataAsset")
for k, v in dict(display_name_key="E2_NAME", debug_name="BP_ThornKnight_C", is_minion=False, hp_multiplier_override=0.0,
                 armored_effects=[cls("GE_Boss_Armored"), cls("GE_Boss_Reflect")],
                 pattern_set_armored=[pattern("SwingL", 280.0, 0.9, 0.2, 1.1, 2.4, normal, 2.0, GX + "PrimaryAttack_LA_Montage"),
                                      pattern("SwingR", 280.0, 0.9, 0.2, 1.1, 2.8, normal, 1.5, GX + "PrimaryAttack_RA_Montage"),
                                      pattern("FourStrikes", 320.0, 1.4, 0.3, 1.5, 6.0, heavy, 1.0, GX + "PrimaryAttack_FourStrikes_Montage")],
                 pattern_set_vulnerable=[pattern("SwingL", 280.0, 1.1, 0.2, 1.8, 3.0, normal, 1.0, GX + "PrimaryAttack_LA_Montage")],
                 mesh=unreal.load_asset(hero("Grux", "Meshes/Grux")),
                 anim_class=unreal.load_class(None, hero("Grux", "Grux_AnimBlueprint.Grux_AnimBlueprint_C")),
                 mesh_scale=1.0, body_scale=3.0, death_anim=unreal.load_asset(GX + "Death_A"), hint_asset=hints["E2"],
                 restore_settings_on_defeat={}, clear_message_key="").items():
    da2.set_editor_property(k, v)
eal.save_loaded_asset(da2, False)
knight_class = boss_bp("/Game/Characters/Bosses/ThornKnight", "BP_ThornKnight", "ThornKnightBoss", da2)

# ---------------------------------------------------------------- Level helpers
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


def scatter(mesh, count, radius, inner=0.0, scale=(0.8, 1.2), seed=1, loc=(0, 0), corridor=0.0, collision=False):
    for slot in unreal.load_asset(mesh).static_materials:
        base = slot.material_interface.get_base_material() if slot.material_interface else None
        if base:
            unreal.MaterialEditingLibrary.set_material_usage(base, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
            eal.save_loaded_asset(base, False)
    a = spawn(cls("MetaScatterField"), (loc[0], loc[1], 0), label=f"Scatter_{mesh.split('/')[-1]}_{seed}")
    for k, v in dict(mesh=unreal.load_asset(mesh), count=count, radius=radius, inner_radius=inner, corridor_half_width=corridor,
                     scale_range=unreal.Vector2D(*scale), seed=seed, collision=collision).items():
        a.set_editor_property(k, v)
    return a


def trigger(eid, loc, scale, bosses):
    t = spawn(cls("EncounterTriggerBox"), loc, scale=scale, label=f"Trigger_{eid}")
    t.set_editor_property("encounter_id", eid)
    t.set_editor_property("bosses", bosses)


def breakpoint_at(bid, loc):
    b = spawn(cls("BreakpointActor"), loc, label=f"Breakpoint_{bid}")
    b.set_editor_property("breakpoint_id", bid)


# ---------------------------------------------------------------- L_Stage1_DarkGrove (+X forward, InfinityBladeIceLands)
import math
import random

IL = "/Game/InfinityBladeIceLands/Environments/"
ICE, MISC = IL + "Ice/", IL + "Misc/"
DR, CLIFF, MOUNT = ICE + "DR_Ice_Mountain/StaticMesh/", ICE + "Env_Ice_Cliffs/StaticMesh/", ICE + "EXO_MountainSide/StaticMesh/"
GROUND, DECO, DECO2 = ICE + "Env_Ice_Ground/StaticMesh/", ICE + "Env_Ice_Deco/StaticMesh/", ICE + "Env_Ice_Deco2/StaticMesh/"
ROCKY, RUINS, FORT = ICE + "Env_Ice_Rocky/StaticMesh/", ICE + "EXO_RockyRuins/StaticMesh/", ICE + "Ice_Fortress/StaticMesh/"
ENV, BLD, IMNT = ICE + "EX_EnvAssets/Meshes/", ICE + "Env_Ice_Buildings/StaticMesh/", ICE + "Env_Ice_Mountain/StaticMesh/"
EXO1 = MISC + "Exo_Deco01/StaticMesh/"
FXI, FXB = "/Game/InfinityBladeIceLands/Effects/FX_Ambient/", "/Game/InfinityBladeEffects/Effects/FX_Ambient/"
FIRE, MIST = FXI + "Fire/P_Fire_Torch_01", FXI + "Fog/P_MistRising_light"
SNOW_FALL, SNOW_BLOW, SNOW_GROUND, SNOW_SPARKLE = (FXB + "Snow/P_SnowFall_01", FXI + "Snow/P_Snow_BlowingLarge_particulates",
                                                   FXI + "Snow/P_Snow_BlowingGroundLarge_Slow2", FXI + "Snow/P_SnowSparkle_512X_512Y")
CINDERS = FXB + "Fire/P_Env_Cinders_Area_Sml_01"
rng = random.Random(1717)

# Layout: E1 grove at x≈3700, E2 plaza at x≈13500 (twice the old E1→E2 distance, user revision).
E1_C, E2_C, E2_R = (3700, 0), (13500, 0), 2700


def place(mesh, loc, yaw=0.0, scale=1.0, label=None, sink=0.0, collision=True):
    """Puts the mesh's bounds center on loc (x, y) and its bottom on loc z (IceLands pivots are often off-center)."""
    s = scale if isinstance(scale, tuple) else (scale, scale, scale)
    b = unreal.load_asset(mesh).get_bounds()
    ox, oy = b.origin.x * s[0], b.origin.y * s[1]
    r = math.radians(yaw)
    x = loc[0] - (ox * math.cos(r) - oy * math.sin(r))
    y = loc[1] - (ox * math.sin(r) + oy * math.cos(r))
    z = (loc[2] if len(loc) > 2 else 0.0) - (b.origin.z - b.box_extent.z) * s[2] - sink
    a = mesh_actor(mesh, (x, y, z), rot=(0, 0, yaw), scale=s, label=label or mesh.split("/")[-1])
    if not collision:
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


def fx(system, loc, label=None, scale=1.0):
    e = spawn(unreal.Emitter, loc, scale=(scale, scale, scale), label=label or system.split("/")[-1])
    e.get_editor_property("particle_system_component").set_template(unreal.load_asset(system))
    return e


def light(loc, color, intensity, radius, label="Light"):
    l = spawn(unreal.PointLight, loc, label=label)
    l.light_component.set_light_color(unreal.LinearColor(*color, 1.0))
    l.light_component.set_intensity(intensity)
    l.light_component.set_editor_property("attenuation_radius", radius)
    l.light_component.set_cast_shadows(False)


def brazier(x, y, big=False):
    place(FORT + "SM_Ice_Fort_Brazier_1", (x, y, 0), rng.uniform(0, 360), 1.6 if big else 1.2)
    fx(FIRE, (x, y, 160 if big else 120), "BrazierFire")
    light((x, y, 260), (1.0, 0.55, 0.22), 60.0 if big else 40.0, 1800.0 if big else 1200.0, "BrazierLight")


def ground_material():
    """World-aligned icy snow: a 1:1 IceLands texture would smear over the 200 m ground plane."""
    name, folder = "M_Stage1_Snow", "/Game/Core/Materials"
    mel = unreal.MaterialEditingLibrary
    path = f"{folder}/{name}"
    m = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    T = ICE + "Env_Ice_Terrain/Textures/"
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 0)
    mask = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -750, 0)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mel.connect_material_expressions(wp, "", mask, "")

    def sample(tex, tile, y, normal=False):
        div = mel.create_material_expression(m, unreal.MaterialExpressionDivide, -600, y)
        div.set_editor_property("const_b", tile)
        mel.connect_material_expressions(mask, "", div, "A")
        t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, y)
        t.set_editor_property("texture", unreal.load_asset(T + tex))
        if normal:
            t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mel.connect_material_expressions(div, "", t, "UVs")
        return t

    snow, dirty, macro = sample("T_Env_Icy_Snow_D", 700.0, -300), sample("T_Env_Ice_Snow_Dirty_D", 900.0, 0), sample("T_Env_Snow_D", 9000.0, 300)
    lerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -150, 0)
    mel.connect_material_expressions(snow, "RGB", lerp, "A")
    mel.connect_material_expressions(dirty, "RGB", lerp, "B")
    mel.connect_material_expressions(macro, "R", lerp, "Alpha")  # large-scale patches of trodden snow
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(sample("T_Env_Icy_Snow_N", 700.0, 600, True), "RGB", unreal.MaterialProperty.MP_NORMAL)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -150, 400)
    rough.set_editor_property("r", 0.75)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m, False)
    return path


fresh_level("/Game/Maps/L_Stage1_DarkGrove", "MetaGameMode")
mesh_actor("/Engine/BasicShapes/Plane", (7000, 0, 0), scale=(240, 220, 1), label="Ground", material=ground_material())

# Night: cold moon, blue fog, falling snow over the whole route.
moon = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -35, 60), label="Moonlight")
moon.light_component.set_intensity(0.7)
moon.light_component.set_light_color(unreal.LinearColor(0.5, 0.62, 1.0, 1.0))
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
sky.light_component.set_intensity(0.3)
sky.light_component.set_editor_property("real_time_capture", True)
fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")
fog.component.set_editor_property("fog_density", 0.04)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.03, 0.045, 0.08, 1.0))
for x in range(-1500, 17000, 3000):
    for y in (-2200, 2200):
        fx(SNOW_FALL, (x, y, 1600), "SnowFall")
for x in range(1000, 16000, 2500):
    fx(SNOW_BLOW, (x, rng.uniform(-800, 800), 300), "SnowBlowing")

# Distant mountains, and a giant frozen troll watching over the plaza.
for loc, yaw, sc, mesh in (((6000, 10500, -300), 0, 1.0, MOUNT + "SM_ICE_Outside_Mountain"), ((9000, -10500, -300), 180, 1.0, MOUNT + "SM_ICE_Outside_Mountain"),
                           ((23000, 0, -200), 90, 1.6, MOUNT + "SM_ICE_Outside_Mountain2"), ((-6500, 0, -200), -90, 1.3, MOUNT + "SM_ICE_Outside_Mountain2"),
                           ((15500, 9000, -100), 30, 1.0, MOUNT + "SM_ICE_Outside_JaggedCliff04"), ((2000, -9000, -100), 200, 1.0, MOUNT + "SM_ICE_Outside_JaggedCliff04")):
    place(mesh, loc, yaw, sc, collision=False)
place(ROCKY + "SM_Env_Ice_MelancholicTroll01", (14500, 7600, -100), -100, 1.0, label="FrozenTroll", collision=False)

# ---------------------------------------------------------------- A. Camp (start + breakpoint 1)
spawn(unreal.PlayerStart, (-900, 0, 120), label="PlayerStart")
breakpoint_at("BP_Stage1_Start", (-250, 300, 0))
place(BLD + "SM_Viking_House_A", (-700, 1550, 0), 180)
place(DR + "SM_Stone_House_HI", (300, -1500, 0), 10)
place(DECO2 + "SM_Env_Ice_Deco2_firepit", (-300, -450, 0))
fx(FIRE, (-300, -450, 70), "CampFire")
light((-300, -450, 180), (1.0, 0.5, 0.2), 60.0, 1600.0, "CampFireLight")
for mesh, loc, yaw in ((ENV + "SM_EnvAssets_Boxes1", (-150, 1000), 10), (ENV + "SM_EnvAssets_Barrel1", (-40, 1080), 0),
                       (ENV + "SM_EnvAssets_Barrel1", (30, 960), 40), (EXO1 + "SM_Exo_Crate_Open_Damaged", (420, 1050), -20),
                       (EXO1 + "SM_Exo_Cart_Damaged", (900, -950), 60), (ENV + "SM_EX_EnvAssets_LargeSack", (-900, -1050), 0),
                       (ENV + "SM_EX_EnvAssets_sacks", (-780, -1150), 30), (EXO1 + "SM_ClothTarps_01", (0, -800), 15),
                       (EXO1 + "SM_WoodenBarrel_Intact", (600, -1150), 0), (EXO1 + "SM_Exo_Hay", (-1100, 1000), 0),
                       (DECO2 + "SM_Env_Ice_Deco2_Well", (-1500, -700), 0), (DECO2 + "SM_Env_Ice_Deco2_Log", (-500, -650), 70),
                       (DECO2 + "SM_Env_Ice_Deco2_Pots", (-1000, 1150), 0), (DECO2 + "SM_Env_Ice_Deco2_bucket", (-1400, -500), 0)):
    place(mesh, (loc[0], loc[1], 0), yaw)
for y in (450, -450):
    place(DECO2 + "SM_Env_Ice_Deco2_flag2", (-1150, y, 0), 90)
    brazier(650, y)
scatter(DR + "SM_Snowy_Pine_Tree", 45, 3200, 1900, (0.8, 1.3), 11, loc=(-300, 0), corridor=700, collision=True)
scatter(DR + "SM_EX_EnvAssets_Pine1", 50, 3400, 1500, (0.9, 1.5), 12, loc=(-300, 0), corridor=700)
scatter(DR + "SM_Snowy_Grass", 120, 2600, 0, (0.6, 1.1), 13, loc=(-300, 0), corridor=250)

# ---------------------------------------------------------------- B. Gate: rune-stone stele (E1 clue) between ice pillars
for y in (560, -560):
    place(ROCKY + "SM_Env_Ice_PillarArch001", (1700, y, 0), 90)
stele = spawn(cls("MetaInscription"), (1800, 300, 0), rot=(0, 0, -90), label="E1_Stele")
stele.set_editor_property("text_key", "E1_STELE")
stele.get_editor_property("mesh").set_static_mesh(unreal.load_asset(ICE + "EX_Env_Ice_1/StaticMesh/SM_Ex_Rune_Stone_1"))
place(RUINS + "SM_Statue01", (1450, -800, 0), 30)
place(ROCKY + "SM_Env_Ice_RuinedPillarbit001", (2150, 850, 0), 15)
place(ROCKY + "SM_Env_Ice_RuinedPillarbit001", (2250, -950, 0), 200, 0.8)

# ---------------------------------------------------------------- C. Frozen dead grove (E1: three assassins in the dark)
ex, ey = E1_C
scatter(GROUND + "SM_Tree_Dead_White", 26, 3200, 1600, (0.7, 1.1), 21, loc=E1_C, corridor=900, collision=True)
scatter(GROUND + "SM_Tree_Dead_Snowy", 22, 3200, 1500, (0.7, 1.1), 22, loc=E1_C, corridor=900, collision=True)
scatter(DR + "SM_White_Dead_Tree", 70, 2900, 900, (0.9, 1.6), 23, loc=E1_C, corridor=400)
scatter(DR + "SM_Snowy_Dead_Tree_B", 20, 3300, 1700, (0.8, 1.2), 24, loc=E1_C, corridor=900, collision=True)
scatter(GROUND + "SM_IceCrystals", 60, 2600, 500, (0.8, 2.0), 25, loc=E1_C)
scatter(DR + "SM_Snowy_Grass", 140, 3000, 300, (0.6, 1.1), 26, loc=E1_C, corridor=250)
scatter(GROUND + "SM_Snowy_Dead_Leaves", 50, 2600, 0, (0.8, 1.6), 27, loc=E1_C)
for loc, yaw in (((ex - 300, ey + 700), 20), ((ex + 500, ey - 800), 150), ((ex + 900, ey + 300), 260)):
    place(DECO2 + "SM_Env_Ice_Deco2_Skeleton", (*loc, 0), yaw)
for loc, yaw, sc in (((ex + 1000, ey + 1050), 0, 1.0), ((ex - 800, ey - 1100), 120, 0.8), ((ex + 1300, ey - 1000), 60, 1.2)):
    place(ROCKY + "SM_Env_Ice_NordRock02", (*loc, 0), yaw, sc)
place(DECO + "SM_IceSpikes_01", (ex + 200, ey + 1200, 0), 30)
place(DECO2 + "SM_Env_Ice_Crystals", (ex - 600, ey + 900, 0), 0, 1.4)
for dx, dy in ((-500, 300), (300, -400), (800, 500), (0, 0)):
    fx(MIST, (ex + dx, ey + dy, 20), "GroveMist")
fx(SNOW_SPARKLE, (ex, ey, 50), "GroveSparkle")
fx(SNOW_GROUND, (ex - 400, ey, 30), "GroveDrift")
assassins = []
for i, (loc, yaw) in enumerate((((ex - 100, ey), 180), ((ex + 300, ey + 600), 200), ((ex + 300, ey - 600), 160))):
    a = spawn(assassin_class, (loc[0], loc[1], 120), rot=(0, yaw, 0), label=f"E1_Assassin_{i}")
    a.set_editor_property("boss_data", da1)  # instances of BPs created this run don't pick up the CDO value
    assassins.append(a)
trigger("E1", (ex - 200, ey, 120), (34, 34, 8), assassins)

# ---------------------------------------------------------------- D. Long cliff pass to the fortress (breakpoint 2)
cliffs = [CLIFF + "SM_Env_Ice_Cliffs_CliffWall_03a", CLIFF + "SM_Env_Ice_Cliffs_CliffWall_01a", CLIFF + "SM_Env_Ice_Cliffs_CliffWall_06a"]
for x in range(5600, 10400, 850):
    for side in (1, -1):
        place(rng.choice(cliffs), (x, side * rng.uniform(1950, 2200), -40), rng.choice((0, 90, 180, 270)), rng.uniform(1.0, 1.35))
    if x % 1700 == 0:
        for side in (1, -1):
            place(DECO + "SM_IceWallSpikes", (x + 300, side * 1350, 0), 90 if side > 0 else 270, rng.uniform(0.8, 1.2))
for x in range(5800, 10000, 1400):
    brazier(x, 1050 if (x // 1400) % 2 else -1050)
pond = place(ICE + "Env_Ice_Terrain/SM_WaterPlain", (7600, -650, 3), 0, (1.4, 0.8, 1.0), label="FrozenPond")
pond.static_mesh_component.set_material(0, unreal.load_asset(ICE + "Env_Ice_Terrain/Materials/M_Env_Ice_Frozen_River_1"))
for loc, yaw in (((6300, 1150), 200), ((6300, -1150), -20)):
    place(RUINS + "SM_Statue01", (*loc, 0), yaw, 1.3)
place(ROCKY + "SM_Env_Ice_Weetower001", (8700, 1250, 0), 200)
place(ROCKY + "SM_Env_Ice_RuinedWalls003", (5500, -1150, 0), 0)
place(ROCKY + "SM_Env_Ice_RuinedWalls003", (9100, -1150, 0), 180)
place(DECO2 + "SM_Env_Ice_Deco2_Statue2", (7000, 2900, 0), 200, 0.5, collision=False)
for mesh, loc, yaw in ((EXO1 + "SM_Exo_Cart_Damaged", (8100, 850), 120), (EXO1 + "SM_Exo_Crate_Open_Destroyed", (8350, 950), 30),
                       (EXO1 + "SM_WoodenBarrel_Damaged", (8000, 1000), 0), (DECO2 + "SM_Env_Ice_Deco2_BoxesBroken", (8250, 700), 80),
                       (DECO2 + "SM_Env_Ice_Deco2_Skeleton2", (7900, 600), 200), (DECO2 + "SM_Env_Ice_Deco2_Stones", (6800, 400), 0),
                       (ICE + "EX_Env_Ice_1/StaticMesh/SM_Ex_Rune_Stone_1", (9400, 900), 250)):
    place(mesh, (*loc, 0), yaw)
scatter(IMNT + "SM_SnowBoulder", 22, 2400, 0, (0.6, 1.2), 31, loc=(7900, 0), corridor=900, collision=True)
scatter(DR + "SM_Snowy_Dead_Tree_B", 18, 2600, 0, (0.7, 1.1), 32, loc=(7900, 0), corridor=1000)
scatter(DR + "SM_Snowy_Grass", 120, 2600, 0, (0.6, 1.0), 33, loc=(7900, 0), corridor=200)
scatter(ICE + "Ice_Castle/SM/SM_IceT3_RockPebbles", 30, 2400, 0, (0.8, 1.4), 34, loc=(7900, 0))
for x in (6200, 8000, 9600):
    fx(MIST, (x, rng.uniform(-500, 500), 20), "PassMist")
    fx(SNOW_GROUND, (x, 0, 30), "PassDrift")
breakpoint_at("BP_Stage1_Hall", (10000, 350, 0))
# The knight's crude box-body debris on the road (clue, spec E2): the same gray checker blocks it becomes at quality 0.
for i, (x, y) in enumerate(((10300, 260), (10420, -220), (10560, 120), (10380, -400), (10700, -60), (10520, 420), (10820, 250))):
    s = rng.uniform(0.25, 0.55)
    mesh_actor("/Engine/BasicShapes/Cube", (x, y, s * 50), rot=(0, i * 37, rng.choice((0, 0, 15))), scale=(s, s * 1.4, s),
               label=f"LowPolyShard_{i}", material="/Game/Core/Materials/M_WorldGrid")

# ---------------------------------------------------------------- E. Open ice-fortress plaza (E2: wide arena, no walls)
cx, cy = E2_C
tile = 1040.0  # SM_Ice_Fort_Floor_1 (260 cm) x4
for i in range(-3, 4):
    for j in range(-3, 4):
        if math.hypot(i, j) * tile <= E2_R + 300:
            place(FORT + "SM_Ice_Fort_Floor_1", (cx + i * tile, cy + j * tile, 0), 0, (4.0, 4.0, 1.0), label="PlazaFloor", sink=10)
for k in range(12):
    a = math.radians(k * 30 + 15)
    if abs(math.degrees(a) - 180) < 20:
        continue  # leave the entrance from the pass open
    brazier(cx + math.cos(a) * (E2_R + 200), cy + math.sin(a) * (E2_R + 200), big=True)
    b = math.radians(k * 30)
    place(FORT + "SM_Ice_Fort_Pillar_4", (cx + math.cos(b) * (E2_R + 450), cy + math.sin(b) * (E2_R + 450), 0), k * 30, 1.3)
for side in (1, -1):
    place(DECO2 + "SM_Env_Ice_Deco2_Statue2", (cx + 4300, side * 2600, 0), 180 + side * 25, 0.7, collision=False)
for k in range(18):
    a = math.radians(k * 20 + rng.uniform(-6, 6))
    r = rng.uniform(3500, 4300)
    place(rng.choice([FORT + "SM_Ice_Chunk_2", FORT + "SM_Ice_Chunk_3", FORT + "SM_Ice_Chunk_4", DECO + "SM_IceSpikes_01", DECO2 + "SM_Env_Ice_Crystals"]),
          (cx + math.cos(a) * r, cy + math.sin(a) * r, 0), rng.uniform(0, 360), rng.uniform(1.0, 1.8))
for k in range(14):  # far cliff ring (beyond the arena, not walls of it), open toward the pass
    a = math.radians(40 + k * 20)
    place(rng.choice(cliffs), (cx + math.cos(a) * 5600, cy + math.sin(a) * 5600, -40), rng.choice((0, 90, 180, 270)), rng.uniform(1.2, 1.6))
for dx, dy in ((0, 0), (-1500, 1200), (1200, -1300), (1500, 1400), (-1200, -1500)):
    fx(SNOW_SPARKLE if dx == 0 else SNOW_GROUND, (cx + dx, cy + dy, 40), "PlazaSnow")
for k in range(4):
    a = math.radians(45 + k * 90)
    fx(CINDERS, (cx + math.cos(a) * (E2_R + 200), cy + math.sin(a) * (E2_R + 200), 150), "PlazaCinders")
light((cx, cy, 1500), (0.55, 0.65, 1.0), 400.0, 6000.0, "PlazaMoonFill")
for k in range(4):  # the fight happens in the middle: keep it readable at night
    a = math.radians(k * 90 + 45)
    light((cx + math.cos(a) * 1300, cy + math.sin(a) * 1300, 900), (0.6, 0.7, 1.0), 150.0, 3000.0, "PlazaFill")
knight = spawn(knight_class, (cx + 400, cy, 120), rot=(0, 180, 0), label="E2_ThornKnight")
knight.set_editor_property("boss_data", da2)
for b, d in [(a, da1) for a in assassins] + [(knight, da2)]:
    if b.get_editor_property("boss_data") != d:
        raise RuntimeError(f"{b.get_actor_label()} lost its BossData")
trigger("E2", (cx, cy, 120), (80, 80, 10), [knight])

les.save_current_level()
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("stage3 assets done")
