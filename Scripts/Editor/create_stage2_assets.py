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


# ---------------------------------------------------------------- T_DigitAtlas (digits 0-9, 5x7 bitmap font, for the digital rain)
DIGITS = ["01110100011001110101110011000101110", "00100011000010000100001000010001110", "01110100010000100010001000100011111",
          "11111000100010000010000011000101110", "00010001100101010010111110001000010", "11111100001111000001000011000101110",
          "00110010001000011110100011000101110", "11111000010001000100010000100001000", "01110100011000101110100011000101110",
          "01110100011000101111000010001001100"]


def make_digit_png(path, cell=64, px=7):
    w, h = cell * 10, cell
    img = [[0] * w for _ in range(h)]
    ox, oy = (cell - 5 * px) // 2, (cell - 7 * px) // 2
    for d, bits in enumerate(DIGITS):
        for r in range(7):
            for c in range(5):
                if bits[r * 5 + c] == "1":
                    for y in range(oy + r * px, oy + (r + 1) * px - 1):
                        for x in range(d * cell + ox + c * px, d * cell + ox + (c + 1) * px - 1):
                            img[y][x] = 255
    raw = b"".join(b"\x00" + bytes(v for v in row for _ in range(3)) for row in img)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


png = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "T_DigitAtlas.png")
make_digit_png(png)
task = unreal.AssetImportTask()
for k, v in dict(filename=png, destination_path="/Game/Core/Textures", automated=True, replace_existing=True, save=True).items():
    task.set_editor_property(k, v)
tools.import_asset_tasks([task])
digit_tex = unreal.load_asset("/Game/Core/Textures/T_DigitAtlas")
digit_tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
eal.save_loaded_asset(digit_tex, False)
if eal.does_asset_exist("/Game/Core/Textures/T_VoidNodes"):
    eal.delete_asset("/Game/Core/Textures/T_VoidNodes")  # replaced by the digital rain

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

# ---------------------------------------------------------------- M_SkyCrack (4.2, user revision: the sky shatters like glass,
# white-purple lightning along the shard edges, green digital rain on black behind the broken shards)
MAT = "/Game/Core/Materials"
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
texobj = mel.create_material_expression(sky, unreal.MaterialExpressionTextureObject, -900, 300)
texobj.set_editor_property("texture", digit_tex)
SKY_HLSL = """
const float PI2 = 6.2831853;
const float K = 16.0;
float2 o = float2(0.5, 0.5);
float2 dd = UV - o; float r = length(dd);
float a = atan2(dd.y, dd.x); if (a < 0.0) a += PI2;
float base = P * 0.22;
float on = step(0.001, P);
// One big hole: a jagged 16-point polygon (random vertex radii) plus fine splinters.
float s = a / PI2 * K; float k0 = floor(s); float t = s - k0; float k1 = fmod(k0 + 1.0, K);
float h0 = frac(sin(k0 * 78.233 + 1.7) * 43758.5453); float h1 = frac(sin(k1 * 78.233 + 1.7) * 43758.5453);
float spl = pow(abs(frac(a * 7.3) - 0.5) * 2.0, 6.0) * 0.18 + pow(abs(frac(a * 19.7 + 0.3) - 0.5) * 2.0, 8.0) * 0.08; // sharp splinters
float R = base * (lerp(0.55 + 0.8 * h0, 0.55 + 0.8 * h1, t) + spl);
float hole = step(r, R) * on;
// Voronoi shards: chunks next to the rim break off too; the rest of the cells form the crack network.
float2 grid = UV * 15.0; float2 ip = floor(grid); float2 fp = frac(grid);
float d1 = 8.0; float d2 = 8.0; float2 cell = 0; float2 cpt = 0;
for (int j = -1; j <= 1; j++) { for (int i = -1; i <= 1; i++) {
  float2 c = ip + float2(i, j);
  float2 h = frac(sin(float2(dot(c, float2(127.1, 311.7)), dot(c, float2(269.5, 183.3)))) * 43758.5453);
  float2 v = float2(i, j) + h - fp; float d = dot(v, v);
  if (d < d1) { d2 = d1; d1 = d; cell = c; cpt = (c + h) / 15.0; } else if (d < d2) { d2 = d; }
} }
float edge = sqrt(d2) - sqrt(d1);
float2 cd = cpt - o; float cr = length(cd); float ca = atan2(cd.y, cd.x); if (ca < 0.0) ca += PI2;
float cs = ca / PI2 * K; float c0 = floor(cs); float ct = cs - c0; float c1 = fmod(c0 + 1.0, K);
float Rc = base * lerp(0.55 + 0.8 * frac(sin(c0 * 78.233 + 1.7) * 43758.5453), 0.55 + 0.8 * frac(sin(c1 * 78.233 + 1.7) * 43758.5453), ct);
float crnd = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
float shard = step(cr, Rc + base * 0.18) * step(0.5, crnd) * on;
hole = max(hole, shard);
float outside = 1.0 - hole;
float nearRim = saturate(1.0 - (r - R) / (base * 0.7 + 0.0001)) * step(R, r);
float crackNet = (1.0 - smoothstep(0.0, 0.008, edge)) * nearRim * nearRim * outside * on;
float shardEdge = (1.0 - smoothstep(0.0, 0.02, edge)) * shard * step(R * 0.92, r); // only chunks breaking off the rim
float rim = saturate(1.0 - abs(r - R) / 0.012) * on;
float rimCore = saturate(1.0 - abs(r - R) / 0.003) * on;
// Radial lightning cracks running out from the hole: thin, jagged (triangle-wave zigzag), fading outward.
float rays = 0.0; float rayGlow = 0.0;
for (int n = 0; n < 10; n++) {
  float fn = n;
  float ang = (fn + frac(sin(fn * 91.3) * 43758.5)) * PI2 / 10.0;
  float len = base * (0.5 + 1.1 * frac(sin(fn * 17.7) * 9343.1));
  float da = a - ang; da -= PI2 * round(da / PI2);
  float zig = (abs(frac(r * 22.0 + fn * 0.37) - 0.5) - 0.25) * 0.02;
  float dist = abs(da * r + zig);
  float onRay = step(R, r) * step(r, R + len);
  float fade = (1.0 - saturate((r - R) / max(len, 0.0001))) * (0.55 + 0.45 * sin(T * (9.0 + fn) + fn * 2.1)); // flicker
  rays = max(rays, (1.0 - smoothstep(0.0, 0.0022, dist)) * onRay * fade);
  rayGlow = max(rayGlow, (1.0 - smoothstep(0.0, 0.012, dist)) * onRay * fade);
}
rays *= on; rayGlow *= on;
float lineCore = max(max(rays, rimCore), max(crackNet * 0.85, shardEdge));
float glow = max(rim, max(rayGlow * 0.6, shardEdge * 0.6));
// Inside: dense green digital rain on black.
const float COLS = 110.0; const float ROWS = 70.0;
float2 g = UV * float2(COLS, ROWS);
float col = floor(g.x); float row = floor(g.y);
float ch = frac(sin(col * 91.7) * 4375.85);
float speed = 8.0 + ch * 14.0;
float head = fmod(T * speed + ch * 300.0, ROWS + 30.0);
float dlt = head - row;
float trail = dlt >= 0.0 ? exp(-dlt / 9.0) : 0.0;
float gidx = floor(frac(sin(col * 13.1 + row * 71.7 + floor(T * 4.0 + ch * 10.0) * 0.37) * 9137.1) * 10.0);
float2 lu = frac(g);
float glyph = Texture2DSampleLevel(Tex, TexSampler, float2((gidx + lu.x) / 10.0, lu.y), 0).r;
float headGlow = (dlt >= 0.0 && dlt < 1.0) ? 1.0 : 0.0;
float3 rain = glyph * (float3(0.05, 1.0, 0.25) * (0.12 + trail * 2.6) + float3(0.7, 1.0, 0.8) * headGlow * 3.0);
float3 c3 = rain * hole + float3(0.54, 0.25, 0.99) * glow * 1.8 + float3(1.0, 0.95, 1.0) * lineCore * 3.5;
float mask = 1.0 - smoothstep(0.44, 0.5, max(abs(dd.x), abs(dd.y)));
return float4(c3 * mask, saturate(hole + glow * 0.8 + lineCore) * mask);
"""
crack = custom(sky, SKY_HLSL, ["UV", "P", "T", "Tex"], unreal.CustomMaterialOutputType.CMOT_FLOAT4, -500, 100)
for src, pin in ((uv, "UV"), (prog, "P"), (tm2, "T"), (texobj, "Tex")):
    mel.connect_material_expressions(src, "", crack, pin)
rgb = mel.create_material_expression(sky, unreal.MaterialExpressionComponentMask, -250, 50)
for c in ("r", "g", "b"):
    rgb.set_editor_property(c, True)
alpha = mel.create_material_expression(sky, unreal.MaterialExpressionComponentMask, -250, 200)
for c, on in (("r", False), ("g", False), ("b", False), ("a", True)):  # R defaults to on → Opacity would read red
    alpha.set_editor_property(c, on)
mel.connect_material_expressions(crack, "", rgb, "")
mel.connect_material_expressions(crack, "", alpha, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(sky)
eal.save_loaded_asset(sky, False)

# ---------------------------------------------------------------- UI textures (HUD / title restyle, user revision)
def write_rgba_png(path, w, h, pixel):
    rows = []
    for y in range(h):
        row = bytearray(b"\x00")
        for x in range(w):
            row += bytes(max(0, min(255, int(c))) for c in pixel(x, y))
        rows.append(bytes(row))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))


def smooth(e0, e1, x):
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0)))
    return t * t * (3 - 2 * t)


def flask_px(x, y):
    # 64x96 potion: cork, neck, round glass body with red liquid and a highlight.
    cx, cy, r = 32.0, 62.0, 25.0
    d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
    neck = abs(x + 0.5 - cx) < 9 and 18 <= y < 42
    cork = abs(x + 0.5 - cx) < 11 and 8 <= y < 18
    body = d < r
    if cork:
        return (120, 82, 50, 255)
    if not (neck or body):
        return (0, 0, 0, 0)
    rim = (body and d > r - 3) or (neck and abs(x + 0.5 - cx) > 6)
    if rim:
        return (230, 220, 200, 255)
    if body and y > 50:
        k = (y - 50) / 37.0
        hl = 70 if math.hypot(x - 22, y - 60) < 5 else 0
        return (200 - 70 * k + hl, 20 + hl, 30 + hl, 255)
    return (70, 60, 80, 150)  # empty glass


UITEX = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "UITextures")
os.makedirs(UITEX, exist_ok=True)
specs = {
    "T_UI_GradientH": (256, 8, lambda x, y: (255, 255, 255, 255 * (1 - x / 255.0) ** 1.4)),
    "T_UI_GradientCenter": (256, 8, lambda x, y: (255, 255, 255, 255 * (1 - abs(2 * x / 255.0 - 1) ** 1.6))),
    "T_UI_Vignette": (256, 256, lambda x, y: (255, 255, 255, 255 * smooth(0.25, 0.85, math.hypot(x / 255.0 - 0.5, (y / 255.0 - 0.5) * 1.1) * 1.6))),
    "T_UI_Flask": (64, 96, flask_px),
}
ui_tasks = []
for name, (w, h, fn) in specs.items():
    path = os.path.join(UITEX, name + ".png")
    write_rgba_png(path, w, h, fn)
    t = unreal.AssetImportTask()
    for k, v in dict(filename=path, destination_path="/Game/UI/Textures", automated=True, replace_existing=True, save=True).items():
        t.set_editor_property(k, v)
    ui_tasks.append(t)
tools.import_asset_tasks(ui_tasks)
for name in specs:
    tex = unreal.load_asset(f"/Game/UI/Textures/{name}")
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    eal.save_loaded_asset(tex, False)

# ---------------------------------------------------------------- WBP_* (trees are built in C++, spec MCP rule)
UI = "/Game/UI"
for wbp, parent in [("WBP_HUD", "MetaHUDWidget"), ("WBP_BossBar", "MetaBossBarWidget"), ("WBP_Settings", "MetaSettingsWidget"),
                    ("WBP_Debugger", "MetaDebuggerWidget"), ("WBP_ErrorWindow", "MetaErrorWindowWidget"),
                    ("WBP_DraggableNumber", "MetaDraggableNumberWidget"),
                    ("WBP_Title", "MetaTitleWidget"), ("WBP_DeathScreen", "MetaDeathScreenWidget")]:
    f = unreal.WidgetBlueprintFactory()
    f.set_editor_property("parent_class", cls(parent))
    bp = recreate(wbp, UI, unreal.WidgetBlueprint, f)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, False)

# ---------------------------------------------------------------- Opening enemies (4.1, user revision): 3 fake minions
# (Paragon Khaimera) + the summoned fake boss "Elbern" (Paragon Narbash). Attack motions come from the Paragon montages.


def pattern(pid, max_range, windup, active, recovery, cooldown, damage, weight=1.0, montage=None):
    p = unreal.BossPattern()
    for k, v in dict(pattern_id=pid, min_range=0.0, max_range=max_range, windup=windup, active=active,
                     recovery=recovery, cooldown=cooldown, weight=weight, fixed_damage=damage).items():
        p.set_editor_property(k, v)
    if montage:
        p.set_editor_property("optional_montage", unreal.load_asset(montage))
    return p


def hero_path(hero, rel):
    return f"/Game/Paragon{hero}/Characters/Heroes/{hero}/{rel}"


def boss_assets(folder, da_name, bp_name, hero, display_key, minion, hp_override, armored, vulnerable, scale=1.0, death=None):
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", cls("BossDataAsset"))
    path = f"{folder}/{da_name}"
    da = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(da_name, folder, None, f)
    da.set_editor_property("display_name_key", display_key)
    da.set_editor_property("debug_name", f"{bp_name}_C")
    da.set_editor_property("is_minion", minion)
    da.set_editor_property("hp_multiplier_override", hp_override)
    da.set_editor_property("armored_effects", [])
    da.set_editor_property("pattern_set_armored", armored)
    da.set_editor_property("pattern_set_vulnerable", vulnerable)
    da.set_editor_property("mesh", unreal.load_asset(hero_path(hero, f"Meshes/{hero}")))
    da.set_editor_property("anim_class", unreal.load_class(None, hero_path(hero, f"{hero}_AnimBlueprint.{hero}_AnimBlueprint_C")))
    da.set_editor_property("mesh_scale", scale)
    if death:
        da.set_editor_property("death_anim", unreal.load_asset(hero_path(hero, f"Animations/{death}")))
    eal.save_loaded_asset(da, False)
    bp_path = f"{folder}/{bp_name}"
    if not eal.does_asset_exist(bp_path):
        f = unreal.BlueprintFactory()
        f.set_editor_property("parent_class", cls("MetaBossBase"))
        tools.create_asset(bp_name, folder, unreal.Blueprint, f)
    unreal.BlueprintEditorLibrary.compile_blueprint(eal.load_asset(bp_path))
    bp_class = eal.load_blueprint_class(bp_path)
    unreal.get_default_object(bp_class).set_editor_property("boss_data", da)
    eal.save_asset(bp_path)
    return bp_class


dmg = tuning.get_editor_property("fake_boss_damage")
KH = hero_path("Khaimera", "Animations/")
NB = hero_path("Narbash", "Animations/")
minion_class = boss_assets("/Game/Characters/Bosses/Khaimera", "DA_Minion_Khaimera", "BP_FakeMinion_Khaimera", "Khaimera", "", True, 0.0,
                           [pattern("ClawA", 220.0, 0.7, 0.2, 0.9, 2.2, dmg, 1.0, KH + "Melee_A_Montage"),
                            pattern("ClawB", 220.0, 0.7, 0.2, 0.9, 2.6, dmg, 1.0, KH + "Melee_B_Montage"),
                            pattern("ClawC", 240.0, 0.9, 0.2, 1.1, 3.5, dmg, 0.6, KH + "Melee_C_Montage")], [], death="Death_A")
boss_class = boss_assets("/Game/Characters/Bosses/Elbern", "DA_Boss_Elbern", "BP_FakeBoss_Elbern", "Narbash", "OP_P1_BOSS_NAME", False,
                         tuning.get_editor_property("fake_boss_hp_multiplier"),
                         [pattern("Swing1", 280.0, 0.9, 0.2, 1.1, 2.4, dmg, 2.0, NB + "Primary_Swing1_Medium_Montage"),
                          pattern("Swing2", 280.0, 0.9, 0.2, 1.1, 2.8, dmg, 1.5, NB + "Primary_Swing2_Medium_Montage"),
                          pattern("Smash", 320.0, 1.4, 0.25, 1.5, 5.0, dmg, 1.0, NB + "Primary_Swing3_Medium_Montage")], [], 1.15, death="Death_Fwd")

# Sandbox test boss (Narbash) gets the same swing motions.
if eal.does_asset_exist("/Game/Dev/DA_Boss_Test"):
    test_da = eal.load_asset("/Game/Dev/DA_Boss_Test")
    for prop, montages in (("pattern_set_armored", ["Primary_Swing1_Medium_Montage", "Primary_Swing3_Medium_Montage"]),
                           ("pattern_set_vulnerable", ["Primary_Swing2_Medium_Montage"])):
        pats = list(test_da.get_editor_property(prop))
        for p, m in zip(pats, montages):
            p.set_editor_property("optional_montage", unreal.load_asset(NB + m))
        test_da.set_editor_property(prop, pats)
    # L_Dev_Sandbox: a practice dummy that never attacks and revives after vanishing (user request).
    test_da.set_editor_property("training_dummy", True)
    test_da.set_editor_property("death_anim", unreal.load_asset(NB + "Death_Fwd"))
    eal.save_loaded_asset(test_da, False)

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


def scatter(mesh, count, radius, inner=0.0, scale=(0.8, 1.2), seed=1, z=0.0, loc=(0, 0), corridor=0.0, collision=False):
    # Instanced rendering needs the usage flag, or the asset-pack material falls back to the default checker.
    for slot in unreal.load_asset(mesh).static_materials:
        base = slot.material_interface.get_base_material() if slot.material_interface else None
        if base:
            unreal.MaterialEditingLibrary.set_material_usage(base, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
            eal.save_loaded_asset(base, False)
    a = spawn(cls("MetaScatterField"), (loc[0], loc[1], z), label=f"Scatter_{mesh.split('/')[-1]}_{seed}")
    for k, v in dict(mesh=unreal.load_asset(mesh), count=count, radius=radius, inner_radius=inner, corridor_half_width=corridor,
                     scale_range=unreal.Vector2D(*scale), seed=seed, collision=collision).items():
        a.set_editor_property(k, v)
    return a


def save_level():
    les.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


# L_Title (6.4, user revision): a moonlit ruin seen through the title UI. TitleCamera = view target.
GL_T = "/Game/InfinityBladeGrassLands/Environments/"
fresh_level("/Game/Maps/L_Title", "MetaTitleGameMode")
mesh_actor("/Engine/BasicShapes/Plane", (2000, 0, 0), scale=(100, 100, 1), label="Ground",
           material=GL_T + "Plains/Env_Plains_Terrain/Materials/M_Grass01")
tm = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -18, 150), label="Moonlight")
tm.light_component.set_intensity(1.2)
tm.light_component.set_light_color(unreal.LinearColor(0.5, 0.6, 1.0, 1.0))
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
tsl = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
tsl.light_component.set_intensity(0.5)
tsl.light_component.set_editor_property("real_time_capture", True)
tfog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")
tfog.component.set_editor_property("fog_density", 0.06)
scatter("/Game/InfinityBladeIceLands/Environments/Ice/DR_Ice_Mountain/StaticMesh/SM_Snowy_Flowers", 900, 3500, 0, (0.35, 0.6), 71, loc=(1600, 0))
for i, n in enumerate(("01", "02", "03")):
    scatter(GL_T + f"Plains/Env_Plains_Flora/StaticMesh/SM_Plains_Flowers{n}", 120, 3500, 0, (0.4, 0.7), 72 + i, loc=(1600, 0))
scatter(GL_T + "Earth/StaticMesh/SM_Cherry_Tree", 30, 5000, 1200, (1.0, 1.5), 75, loc=(2500, 0), corridor=900)
place_t = lambda mesh, loc, yaw=0.0, s=1.0: mesh_actor(GL_T + mesh, loc, rot=(0, 0, yaw), scale=(s, s, s))
place_t("Plains/Env_Plains_Ruins/StaticMesh/SM_Plains_CastleArch_02", (2200, 588 * 1.6 / 2, 0), 90, 1.6)
place_t("Plains/Env_Plains_Statues/StaticMesh/SM_Plains_Angel_Statue_01", (2600, 0, 0), 180, 1.2)
place_t("Plains/Env_Plains_Ruins/StaticMesh/SM_Plains_Column_Large_Broken_01", (1700, -900, 0), 30)
place_t("Plains/Env_Plains_Ruins/StaticMesh/SM_Plains_Column_Small_Broken_01", (1500, 800, 0), 0)
place_t("Plains/Env_Plains_Organic/StaticMesh/SM_Plains_LargeRock_SpireLOD", (9000, 2500, 0), 20, 3.5)
place_t("Plains/Env_Plains_Organic/StaticMesh/SM_Plains_LargeRock_SpireLOD", (8000, -4000, 0), -40, 3.0)
glow = spawn(unreal.PointLight, (2500, 0, 450), label="StatueGlow")
glow.light_component.set_light_color(unreal.LinearColor(0.95, 0.8, 0.45, 1.0))
glow.light_component.set_intensity(80.0)
glow.light_component.set_editor_property("attenuation_radius", 1500.0)
cam = spawn(unreal.CameraActor, (0, 0, 190), rot=(0, 6, 0), label="TitleCamera")
cam.tags = ["TitleCamera"]
save_level()

# L_Prologue_FlowerField (4.1, user revision): moonlit flower field → 3 fake minions → ~15 s ruined avenue → small ruin
# dungeon where the fake boss is summoned. Everything is Infinity Blade GrassLands. +X is forward.
GL = "/Game/InfinityBladeGrassLands/Environments/"
RUIN, FLORA, ORG, STAT, FLOOR = (GL + "Plains/Env_Plains_Ruins/StaticMesh/", GL + "Plains/Env_Plains_Flora/StaticMesh/",
                                 GL + "Plains/Env_Plains_Organic/StaticMesh/", GL + "Plains/Env_Plains_Statues/StaticMesh/",
                                 GL + "Plains/Env_Plains_Floors/StaticMesh/")
EXO1, EXO2, EARTH = GL + "Misc/Exo_Deco01/StaticMesh/", GL + "Misc/Exo_Deco02/StaticMesh/", GL + "Earth/StaticMesh/"


def place(mesh, loc, yaw=0.0, scale=1.0, label=None):
    s = scale if isinstance(scale, tuple) else (scale, scale, scale)
    return mesh_actor(mesh, loc, rot=(0, 0, yaw), scale=s, label=label or mesh.split("/")[-1])


def torch_light(loc, intensity=60.0, radius=1100.0):  # candela
    l = spawn(unreal.PointLight, loc, label="TorchLight")
    l.light_component.set_light_color(unreal.LinearColor(1.0, 0.55, 0.2, 1.0))
    l.light_component.set_intensity(intensity)
    l.light_component.set_editor_property("attenuation_radius", radius)
    l.light_component.set_cast_shadows(False)


fresh_level("/Game/Maps/L_Prologue_FlowerField", "MetaGameMode")
mesh_actor("/Engine/BasicShapes/Plane", (3000, 0, 0), scale=(280, 160, 1), label="Ground",
           material=GL + "Plains/Env_Plains_Terrain/Materials/M_Grass01")
moon = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(0, -28, 40), label="Moonlight")
moon.light_component.set_intensity(1.5)
moon.light_component.set_light_color(unreal.LinearColor(0.55, 0.65, 1.0, 1.0))
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
sl = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
sl.light_component.set_intensity(0.6)
sl.light_component.set_editor_property("real_time_capture", True)
spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Fog")

# --- Flower fields (kept clear on the road) ---
for center, seed in (((-1800, 0), 3), ((2400, 0), 4)):
    scatter("/Game/InfinityBladeIceLands/Environments/Ice/DR_Ice_Mountain/StaticMesh/SM_Snowy_Flowers", 1000, 3800, 0, (0.35, 0.6), seed, loc=center, corridor=420)
    for i, n in enumerate(("01", "02", "03", "04")):
        scatter(FLORA + f"SM_Plains_Flowers{n}", 140, 3800, 0, (0.4, 0.7), seed * 10 + i, loc=center, corridor=420)
    scatter(FLORA + "SM_Plains_Grass01", 300, 4000, 0, (0.2, 0.35), seed * 10 + 5, loc=center, corridor=420)
    scatter(FLORA + "SM_Plains_Fern02", 120, 4000, 0, (0.4, 0.7), seed * 10 + 6, loc=center, corridor=500)
# --- Trees, rocks, distant cliffs ---
scatter(EARTH + "SM_Cherry_Tree", 70, 8500, 1500, (0.9, 1.5), 41, loc=(2500, 0), corridor=1500)
scatter(FLORA + "SM_Plains_Tree01", 45, 9000, 2000, (0.8, 1.2), 42, loc=(2500, 0), corridor=1800)
scatter(ORG + "SM_Plains_LargeRock01", 22, 9500, 2500, (0.6, 1.2), 43, loc=(2500, 0), corridor=1800, collision=True)
scatter(ORG + "SM_Plains_SmallRock01", 40, 9000, 1200, (0.4, 0.9), 44, loc=(2500, 0), corridor=900, collision=True)
for x, y, yaw, s in ((17000, 0, 0, 4.0), (12000, 8000, 60, 3.5), (12000, -8000, -60, 3.5), (2000, 9500, 90, 4.0),
                     (2000, -9500, -90, 4.0), (-9500, 6000, 150, 3.0), (-9500, -6000, -150, 3.0)):
    place(ORG + "SM_Plains_LargeRock_SpireLOD", (x, y, 0), yaw, s)

# --- Start: fountain and angel statues behind the hero ---
place(RUIN + "SM_Plains_Castle_Fountain_01", (-3700, 0, 0))
place(STAT + "SM_Plains_Angel_Statue_01", (-3200, 700, 0), -90)
place(STAT + "SM_Plains_Angel_Statue_01", (-3200, -700, 0), 90)
spawn(unreal.PlayerStart, (-2600, 0, 120), label="PlayerStart")

# --- Fake minions (3x Khaimera) right ahead ---
minions = [spawn(minion_class, (-900, y, 120), rot=(0, 0, 180), label=f"FakeMinion_{i}") for i, y in enumerate((-380, 0, 380))]
t_min = spawn(cls("EncounterTriggerBox"), (-1100, 0, 120), scale=(25, 25, 6), label="Trigger_P1_Minions")
t_min.set_editor_property("encounter_id", "P1_Minions")
t_min.set_editor_property("bosses", minions)
place(RUIN + "SM_Plains_Column_Small_Broken_01", (-700, 900, 0), 20)
place(RUIN + "SM_Plains_Column_Large_Broken_02", (-1300, -1000, 0), 120)
place(EXO1 + "SM_Exo_Cart_Damaged", (-1500, 1100, 0), 70)

# --- Ruined avenue (x 0..6000, ~15 s at walk speed): stone road, columns, torches, arches, statues, debris ---
for k in range(9):
    place(FLOOR + "SM_Plains_Floors_Flat02", (386 + 772 * k, 0, 1), 0, (1.0, 1.05, 1.0))
columns = ["SM_Plains_Column_Small_01", "SM_Plains_Column_Small_Broken_01", "SM_Plains_Column_Small_01", "SM_Plains_Column_Small_Broken_02"]
for k, x in enumerate(range(400, 6000, 900)):
    for side in (1, -1):
        place(RUIN + columns[(k + (side > 0)) % 4], (x, side * 640, 0), 0 if side > 0 else 180)
    if k % 2 == 0:
        for side in (1, -1):
            place(RUIN + "SM_Plains_CastleArch_Iron_Torch_01", (x + 450, side * 560, 0))
            torch_light((x + 450, side * 560, 260))
for x, s in ((1500, 2.2), (3600, 2.2), (5400, 2.0)):
    place(RUIN + "SM_Plains_CastleArch_02", (x, 588 * s / 2 - 6 * s, 0), 90, s)  # spans the road (local X → world Y)
for x, side, yaw in ((2500, 1, -90), (2500, -1, 90)):
    place(STAT + "SM_Plains_Man_Statue_01_torch", (x, side * 900, 0), yaw)
    torch_light((x, side * 900, 450), 40.0)
for x, side, yaw in ((4400, 1, -90), (4400, -1, 90)):
    place(STAT + "SM_Plains_Angel_Statue_01", (x, side * 950, 0), yaw)
for x, y, yaw, mesh in ((1000, 1500, 10, "SM_Plains_CastleWall_Ruin_01"), (2000, -1600, 170, "SM_Plains_CastleWall_Ruin_02"),
                        (3100, 1700, -15, "SM_Plains_CastleWall_Ruin_03"), (4800, -1500, 190, "SM_Plains_CastleWall_Ruin_01"),
                        (3000, 2700, 30, "SM_Plains_CastleTower_Ruin_02"), (5600, -2500, -20, "SM_Plains_CastleTower_Ruin_01"),
                        (1800, 2400, 0, "SM_Plains_Column_Large_Broken_01"), (4000, -2300, 60, "SM_Plains_Column_Large_Broken_03")):
    place(RUIN + mesh, (x, y, 0), yaw)
for x, y, yaw, mesh in ((900, -900, 30, EXO1 + "SM_WoodenBarrel_Damaged"), (950, -1000, 0, EXO1 + "SM_WoodenBarrel_Intact"),
                        (3300, 950, 80, EXO1 + "SM_Exo_Crate_Open_Damaged"), (4900, 900, 10, EXO1 + "SM_Exo_Hay"),
                        (2100, -950, 0, ORG + "SM_Plains_LargeRock_Pile01"), (5800, -1000, 45, EXO2 + "SM_Exo_Stone01_Group")):
    place(mesh, (x, y, 0), yaw)

# --- Dungeon: walled ruin courtyard (x 6300..8940, y ±1300), entrance arch on the road ---
DX0, DX1, DY = 6300, 8940, 1300
place(FLOOR + "SM_Plains_Floors_Flat01", ((DX0 + DX1) / 2, 0, 2), 0, (1.16, 1.14, 1.0))
for k in range(5):
    place(RUIN + "SM_Plains_Wall_Straight_01", (DX1 + 40, DY - 528 * k, 0))                     # back wall, runs -Y
    for side in (1, -1):
        place(RUIN + "SM_Plains_Wall_Straight_01", (DX0 + 528 * k, side * (DY + 40), 0), 90)     # side walls, run +X
for y in (DY, DY - 372, -420, -792):
    place(RUIN + "SM_Plains_Wall_Straight_01", (DX0 - 40, y, 0))                                # front wall, gap |y| < 400
place(RUIN + "SM_Plains_CastleArch_02", (DX0 - 20, 588 * 1.4 / 2 - 8, 0), 90, 1.4)              # entrance arch
place(RUIN + "SM_Plains_CastleArch_Iron_Gate_01_Broken", (DX0 - 60, 520, 0), 90)
for x in (6900, 8300):
    for side in (1, -1):
        place(RUIN + "SM_Plains_Column_Large_01", (x + 115, side * 800 + 64, 0))
place(STAT + "SM_Plains_Angel_Statue_01", (DX1 - 250, 0, 0), 180, 1.3)
for side in (1, -1):
    place(STAT + "SM_Plains_Man_Statue_01_torch", (DX1 - 250, side * 850, 0), 180)
    torch_light((DX1 - 300, side * 850, 450), 60.0, 1600.0)
    place(RUIN + "SM_Plains_CastleArch_Iron_Torch_01", (DX0 + 250, side * 1050, 0))
    torch_light((DX0 + 250, side * 1050, 260), 60.0, 1400.0)
for x, y, yaw, mesh in ((6600, 1150, 0, EXO2 + "SM_Exo_CobWeb03"), (8700, -1150, 90, EXO2 + "SM_Exo_CobWeb03"),
                        (7000, -1000, 40, FLOOR + "SM_Plains_Floors_Rubble"), (8000, 1000, 160, FLOOR + "SM_Plains_Floors_Rubble02"),
                        (7500, 1100, 0, EXO2 + "SM_Exo_Stone01_Group"), (8500, 600, 30, EXO1 + "SM_WoodenBarrel_Destroyed")):
    place(mesh, (x, y, 3), yaw)
boss = spawn(boss_class, (8300, 0, 130), rot=(0, 0, 180), label="FakeBoss_Elbern")
t_boss = spawn(cls("EncounterTriggerBox"), (7600, 0, 120), scale=(16, 16, 6), label="Trigger_P1")
t_boss.set_editor_property("encounter_id", "P1")
t_boss.set_editor_property("bosses", [boss])

# --- Story actors ---
holy = spawn(unreal.PointLight, (8300, 0, 700), label="HolyLight")
holy.light_component.set_light_color(unreal.LinearColor(0.95, 0.82, 0.48, 1.0))  # #F2D27A
holy.light_component.set_editor_property("attenuation_radius", 4000.0)
holy.light_component.set_intensity(0.0)
holy.tags = ["HolyLight"]
crack_plane = mesh_actor("/Engine/BasicShapes/Plane", (14000, 0, 4000), rot=(90, 0, -90), scale=(130, 130, 1), label="SkyCrack",
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
