# S3: E1 assassin + E2 thorn knight data/BPs, DA_Hint_E1/E2, and L_Stage1_DarkGrove (dark forest, E1 clearing, E2 ruin hall,
# two breakpoints). Re-running rebuilds the level. Run after create_string_table.py.
# Headless: UnrealEditor-Cmd AccessedNone.uproject -run=pythonscript -script=Scripts/Editor/create_stage3_assets.py
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
tuning = unreal.get_default_object(unreal.load_class(None, "/Script/AccessedNone.AccessedNoneSettings"))
GL = "/Game/InfinityBladeGrassLands/Environments/"
RUIN, FLORA, ORG, STAT, FLOOR = (GL + "Plains/Env_Plains_Ruins/StaticMesh/", GL + "Plains/Env_Plains_Flora/StaticMesh/",
                                 GL + "Plains/Env_Plains_Organic/StaticMesh/", GL + "Plains/Env_Plains_Statues/StaticMesh/",
                                 GL + "Plains/Env_Plains_Floors/StaticMesh/")
EXO1, EXO2, EARTH = GL + "Misc/Exo_Deco01/StaticMesh/", GL + "Misc/Exo_Deco02/StaticMesh/", GL + "Earth/StaticMesh/"


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
                 pattern_set_vulnerable=[pattern("Slash", 230.0, 0.9, 0.2, 1.2, 2.5, normal, 1.0, SV + "Swing2_Medium_Montage")],
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


def place(mesh, loc, yaw=0.0, scale=1.0, label=None):
    s = scale if isinstance(scale, tuple) else (scale, scale, scale)
    return mesh_actor(mesh, loc, rot=(0, 0, yaw), scale=s, label=label or mesh.split("/")[-1])


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


def torch_light(loc, intensity=40.0, radius=1100.0):
    l = spawn(unreal.PointLight, loc, label="TorchLight")
    l.light_component.set_light_color(unreal.LinearColor(1.0, 0.5, 0.2, 1.0))
    l.light_component.set_intensity(intensity)
    l.light_component.set_editor_property("attenuation_radius", radius)
    l.light_component.set_cast_shadows(False)


def trigger(eid, loc, scale, bosses):
    t = spawn(cls("EncounterTriggerBox"), loc, scale=scale, label=f"Trigger_{eid}")
    t.set_editor_property("encounter_id", eid)
    t.set_editor_property("bosses", bosses)


def breakpoint_at(bid, loc):
    b = spawn(cls("BreakpointActor"), loc, label=f"Breakpoint_{bid}")
    b.set_editor_property("breakpoint_id", bid)


# ---------------------------------------------------------------- L_Stage1_DarkGrove (+X forward)
fresh_level("/Game/Maps/L_Stage1_DarkGrove", "MetaGameMode")
mesh_actor("/Engine/BasicShapes/Plane", (4500, 0, 0), scale=(150, 80, 1), label="Ground", material="/Game/EnvironmentPack2/Materials/M_Grass_Asian_Dark")
moon = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -35, 60), label="Moonlight")
moon.light_component.set_intensity(0.6)
moon.light_component.set_light_color(unreal.LinearColor(0.45, 0.55, 0.9, 1.0))
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
sky.light_component.set_intensity(0.25)
sky.light_component.set_editor_property("real_time_capture", True)
fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")
fog.component.set_editor_property("fog_density", 0.05)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.02, 0.03, 0.05, 1.0))

# Dense dark forest, a path along X kept clear, clearings around the two encounters.
scatter(FLORA + "SM_Plains_Tree01", 140, 7000, 0, (0.9, 1.5), 1, loc=(4000, 0), corridor=700, collision=True)
scatter(EARTH + "SM_SmallTree02", 160, 7000, 0, (1.0, 1.8), 2, loc=(4000, 0), corridor=500)
scatter(FLORA + "SM_Plains_Fern03", 260, 7000, 0, (0.5, 0.9), 3, loc=(4000, 0), corridor=300)
scatter(EARTH + "SM_GroundCover_Leafy03", 260, 7000, 0, (0.6, 1.1), 4, loc=(4000, 0), corridor=250)
scatter(EARTH + "SM_Rootcluster_A", 50, 6500, 0, (0.8, 1.4), 5, loc=(4000, 0), corridor=500)
scatter(EARTH + "SM_LeavePiles_01", 120, 6000, 0, (0.8, 1.5), 6, loc=(4000, 0))
scatter(ORG + "SM_Plains_LargeRock01", 26, 7500, 0, (0.6, 1.2), 7, loc=(4000, 0), corridor=900, collision=True)

# Start + breakpoint 1
spawn(unreal.PlayerStart, (-600, 0, 120), label="PlayerStart")
breakpoint_at("BP_Stage1_Start", (-250, 300, 0))

# E1 clearing: stele at the entrance, the assassin waits unseen.
for x, y, yaw, mesh in ((1700, 520, 0, RUIN + "SM_Plains_Column_Small_Broken_01"), (1700, -520, 180, RUIN + "SM_Plains_Column_Small_Broken_02"),
                        (3600, 900, 30, RUIN + "SM_Plains_Column_Large_Broken_02"), (2600, -950, 160, ORG + "SM_Plains_SmallRock01")):
    place(mesh, (x, y, 0), yaw)
stele = spawn(cls("MetaInscription"), (1800, 260, 0), rot=(0, 0, -90), label="E1_Stele")
stele.set_editor_property("text_key", "E1_STELE")
stele.get_editor_property("mesh").set_static_mesh(unreal.load_asset(STAT + "SM_Plains_Castle_DemonDoor_01"))
assassin = spawn(assassin_class, (3400, 0, 120), rot=(0, 180, 0), label="E1_Assassin")
assassin.set_editor_property("boss_data", da1)  # instances of BPs created this run don't pick up the CDO value
trigger("E1", (3000, 0, 120), (32, 32, 8), [assassin])

# Breakpoint 2 + the knight's low-poly debris in front of its hall (clue, spec E2).
breakpoint_at("BP_Stage1_Hall", (5100, 320, 0))
debris = [("/Engine/BasicShapes/Sphere", (0.5, 0.5, 0.5)), ("/Engine/BasicShapes/Cone", (0.4, 0.4, 0.6)), ("/Engine/BasicShapes/Cube", (0.4, 0.3, 0.3))]
for i, (x, y) in enumerate(((5600, 260), (5750, -220), (5900, 120), (5650, -380), (6000, -60), (5820, 380))):
    mesh, sc = debris[i % 3]
    mesh_actor(mesh, (x, y, 15), rot=(i * 23, i * 47, i * 11), scale=sc, label=f"LowPolyShard_{i}")

# E2 hall: walled ruin courtyard, torch-lit.
DX0, DX1, DY = 6300, 8940, 1300
place(FLOOR + "SM_Plains_Floors_Flat01", ((DX0 + DX1) / 2, 0, 2), 0, (1.16, 1.14, 1.0))
for k in range(5):
    place(RUIN + "SM_Plains_Wall_Straight_01", (DX1 + 40, DY - 528 * k, 0))
    for side in (1, -1):
        place(RUIN + "SM_Plains_Wall_Straight_01", (DX0 + 528 * k, side * (DY + 40), 0), 90)
for y in (DY, DY - 372, -420, -792):
    place(RUIN + "SM_Plains_Wall_Straight_01", (DX0 - 40, y, 0))
place(RUIN + "SM_Plains_CastleArch_02", (DX0 - 20, 588 * 1.4 / 2 - 8, 0), 90, 1.4)
for x in (7000, 8300):
    for side in (1, -1):
        place(RUIN + "SM_Plains_Column_Large_Broken_01" if x == 7000 else RUIN + "SM_Plains_Column_Large_01", (x + 115, side * 900 + 64, 0))
for side in (1, -1):
    place(RUIN + "SM_Plains_CastleArch_Iron_Torch_01", (DX0 + 250, side * 1050, 0))
    torch_light((DX0 + 250, side * 1050, 260), 50.0, 1500.0)
    place(STAT + "SM_Plains_Man_Statue_01_torch", (DX1 - 250, side * 850, 0), 180)
    torch_light((DX1 - 300, side * 850, 450), 50.0, 1600.0)
for x, y, yaw, mesh in ((6700, 1100, 0, EXO2 + "SM_Exo_CobWeb03"), (8700, -1100, 90, EXO2 + "SM_Exo_CobWeb03"),
                        (7200, -1000, 40, FLOOR + "SM_Plains_Floors_Rubble"), (8100, 1000, 160, FLOOR + "SM_Plains_Floors_Rubble02")):
    place(mesh, (x, y, 3), yaw)
knight = spawn(knight_class, (8200, 0, 120), rot=(0, 180, 0), label="E2_ThornKnight")
knight.set_editor_property("boss_data", da2)
for b, d in ((assassin, da1), (knight, da2)):
    if b.get_editor_property("boss_data") != d:
        raise RuntimeError(f"{b.get_actor_label()} lost its BossData")
trigger("E2", (7400, 0, 120), (18, 18, 8), [knight])

les.save_current_level()
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("stage3 assets done")
