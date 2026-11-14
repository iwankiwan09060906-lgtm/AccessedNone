# S2: placeholder audio (spec 12.4 "no audio assets → synthesize"). Writes WAVs to Saved/GeneratedAudio and imports:
#   /Game/Audio/SW_BGM_Prologue  — 16 s looping string-like pad (Dm - Bb - F - A), normal-world "orchestral" stand-in
#   /Game/Audio/SW_KeyClick      — 25 ms terminal key click (debugger)
import math
import os
import random
import struct
import wave
import unreal

RATE = 22050
out_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "GeneratedAudio")
os.makedirs(out_dir, exist_ok=True)


def write_wav(name, samples):
    path = os.path.join(out_dir, name + ".wav")
    peak = max(1e-6, max(abs(s) for s in samples))
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(s / peak * 0.8 * 32767)) for s in samples))
    return path


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12)


# --- BGM: four 4 s chords, each voice = band-limited saw (6 harmonics) with vibrato and slow swell ---
chords = [[50, 57, 62, 65, 69], [46, 53, 58, 62, 65], [41, 53, 57, 60, 65], [45, 52, 57, 61, 64]]
chord_len = 4.0
n = int(RATE * chord_len * len(chords))
bgm = [0.0] * n
for ci, chord in enumerate(chords):
    start = int(ci * chord_len * RATE)
    length = int(chord_len * RATE)
    for vi, note in enumerate(chord):
        f = midi(note)
        phase = random.random()
        for i in range(length):
            t = i / RATE
            env = min(1.0, t / 1.2) * min(1.0, (chord_len - t) / 1.0)  # swell in, fade out → seamless loop
            vib = 1.0 + 0.004 * math.sin(2 * math.pi * 5.2 * t + vi)
            x = 2 * math.pi * f * vib * t + phase
            s = sum(math.sin(k * x) / k for k in range(1, 7))
            bgm[start + i] += s * env * (0.6 if vi == 0 else 0.35)
bgm_path = write_wav("SW_BGM_Prologue", bgm)

# --- Key click ---
click = [(random.uniform(-1, 1) * 0.6 + math.sin(2 * math.pi * 2400 * i / RATE)) * math.exp(-i / (RATE * 0.004)) for i in range(int(RATE * 0.025))]
click_path = write_wav("SW_KeyClick", click)

# --- Explosion (phase 4): noise burst + falling low rumble, 3.5 s ---
boom = []
lp = 0.0
for i in range(int(RATE * 3.5)):
    t = i / RATE
    lp += (random.uniform(-1, 1) - lp) * 0.06  # low-passed noise = rumble
    sub = math.sin(2 * math.pi * (55 - 30 * min(t, 1.0)) * t)
    boom.append((lp * 3.0 + sub * 0.8 + random.uniform(-1, 1) * math.exp(-t * 18)) * math.exp(-t * 1.3))
boom_path = write_wav("SW_Explosion", boom)

tasks = []
for p in (bgm_path, click_path, boom_path):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", p)
    t.set_editor_property("destination_path", "/Game/Audio")
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

bgm_asset = unreal.load_asset("/Game/Audio/SW_BGM_Prologue")
bgm_asset.set_editor_property("looping", True)
unreal.EditorAssetLibrary.save_loaded_asset(bgm_asset, False)
print("audio:", [t.get_editor_property("imported_object_paths") for t in tasks])
