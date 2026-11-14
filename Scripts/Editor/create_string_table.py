# S2-T01: /Game/Text/ST_Game from the table below (spec chapter 11 + UI keys from DECISIONS.md).
# This file is the source of truth for all user-facing text. Re-run after editing.
# Headless: UnrealEditor-Cmd AccessedNone.uproject -run=pythonscript -script=Scripts/Editor/create_string_table.py
import csv
import os
import unreal

ENTRIES = [
    # 11.1 common / system
    ("UI_DEATH", "Player = None"),
    ("UI_DEATH_SUB", "LogScript: Error: Accessed None trying to read property \"Player\""),
    ("UI_DEMON_VANQUISHED", "DEMON VANQUISHED"),
    ("UI_DEMON_VANQUISHED_GLITCH", "DEMON VANQ??$#ED"),
    ("UI_SYSTEM_VANQUISHED", "SYSTEM VANQUISHED"),
    ("UI_IMMUNE", "IMMUNE"),
    ("UI_ACCESS_DENIED", "Access Denied"),
    ("UI_NO_PERMISSION", "권한 없음"),
    ("SYS_SETTING_OVERRIDE", "[SYSTEM] {Boss} 가 설정을 덮어썼습니다: {Setting} {Old} → {New}"),
    ("UI_BREAKPOINT_SAVE", "브레이크포인트 설정됨"),
    ("UI_BREAKPOINT_PROMPT", "[E] 브레이크포인트"),
    ("UI_READ_PROMPT", "[E] 살펴보기"),
    # 11.2 opening
    ("OP_P1_TUT_MOVE", "WASD로 이동합니다."),
    ("OP_P1_TUT_DODGE", "Space를 눌러 구르면 공격을 피할 수 있습니다."),
    ("OP_P1_TUT_LIGHT", "마우스 왼쪽 버튼으로 공격합니다."),
    ("OP_P1_TUT_HEAVY", "Shift를 누른 채 공격하면 강공격이 나갑니다."),
    ("OP_P1_TUT_HEAL", "R을 눌러 회복약을 사용합니다."),
    ("OP_P1_BOSS_NAME", "잿빛 왕관의 대주교, 엘베른"),
    ("OP_P3_LOG_01", "[SYSTEM] LogTemp: Warning: Unregistered Anomaly Actor detected in World 'Ending_Final'"),
    ("OP_P3_LOG_02", "[SYSTEM] LogBlueprint: Error: Accessed None trying to read property \"Hero\""),
    ("OP_P3_DLG_A", "[방어 개체 A] \"...컴…파일… 되지… 않은… 오류… 개체를… 색…출…\""),
    ("OP_P3_DLG_B", "[방어 개체 B] \"엔딩은… 허가되지… 않았다. 격리… 시작.\""),
    ("OP_P3_LOG_03", "[SYSTEM] Fatal: Ending sequence overwritten. Ending = None"),
    ("OP_P4_DAMAGE", "-999,999,999"),
    ("OP_P5_DBG_01", "> 이 세계의 물리법칙(인게임 수치)으로는 저 방어 코드를 뚫을 수 없습니다."),
    ("OP_P5_ERR_TITLE", "⚠ Fatal Error — Respawn.exe"),
    ("OP_P5_ERR_BODY", "리스폰 데이터를 불러오는 중..."),
    ("OP_P5_BTN_RETRY", "재시도"),
    ("OP_P5_BTN_TITLE", "타이틀로"),
    ("OP_P5_BTN_OK", "확인"),
    ("OP_P5_OVERFLOW", "진행률 9,999,999,900%... 치명적 오버플로우 발생!"),
    ("OP_P5_DBG_HINT_30", "> 숫자는… 결국 화면에 그려진 글자일 뿐입니다."),
    ("OP_P5_DBG_HINT_60", "> 저 글자들, 잡을 수 있지 않을까요?"),
    # 11.3 encounters
    ("E1_NAME", "어둠 속의 암살자"),
    ("E1_STELE", "빛을 두려워하는 자는 어둠이 아니라 눈을 지배한다."),
    ("E1_CLEAR", "> 눈을 보호하세요. 밝기는 원래대로 돌려두겠습니다."),
    ("E2_NAME", "가시 갑옷의 기사"),
    ("E2_DEBUG_TRIS", "Triangles: 48,000,000"),
    ("E3_NAME_STATUE", "동상"),
    ("E3_NAME_PLAYER", "나"),
    ("E3_DLG_01", "[동상] 지나갈 수 없다. 이 몸을 움직일 자격은 이 몸에게만 있다."),
    ("E3_DLG_02", "[나] ..."),
    ("E3_DLG_AFTER", "[동상] ...어? 이게 아닌데?"),
    ("E4_NAME", "공명하는 쌍둥이 심판자 [L] / [R]"),
    ("E5_NAME", "해상도 압착 — 탄막 군단"),
    ("E5_WALL", "16:9"),
    ("E6_NAME", "마스터 코드 / 시스템 코어"),
    ("E6_DEBUG_NAME", "BP_MasterCore_C"),
    ("E6_LOCK", "[SYSTEM] BP_MasterCore_C 가 메뉴 접근 권한을 회수했습니다."),
    ("E6_DBG_SACRIFICE_01", "> 제 남은 데이터를 전부 쓰겠습니다."),
    ("E6_DBG_SACRIFICE_02", "> 개발자 콘솔 권한… 강제 개방. ~ 키를 누르세요."),
    ("E6_DBG_SACRIFICE_03", "> 저 이름이 보이죠? 이제 저건 그냥… 변수입니다."),
    ("E6_FINISHER_PROMPT", "[E] 처형"),
    # 11.4 hints
    ("HINT_E1_3", "> 어둠은 이 세계의 것이 아닙니다. 당신 화면의 것이죠."),
    ("HINT_E1_5", "> ESC › 설정 › 그래픽 › 밝기. 끝까지 올려보세요."),
    ("HINT_E2_3", "> 저 가시, 폴리곤이 몇 개인지 아십니까? 이 세계엔 사치예요."),
    ("HINT_E2_5", "> ESC › 설정 › 그래픽 › 모델링 품질. 가장 낮게."),
    ("HINT_E4_3", "> 왼쪽 저것은 실체가 아닙니다. 왼쪽 스피커에서 나오는 소리일 뿐이에요."),
    ("HINT_E4_5", "> ESC › 설정 › 오디오 › 스테레오 밸런스. 오른쪽 끝으로."),
    ("HINT_E5_3", "> 이 공간은 화면 폭만큼만 존재합니다. 화면이 좁아지면… 남는 건 뭘까요?"),
    ("HINT_E5_5", "> ESC › 설정 › 그래픽 › 화면 비율. 4:3으로."),
    ("HINT_E6_CONSOLE", "> 중력과 시간. 이 세계에서 가장 기본적인 두 변수입니다. help를 입력해 보세요."),
    # 11.5 console
    ("CON_HELP", "사용 가능한 명령: set gravity <값> · slomo <값> · help"),
    ("CON_USAGE", "사용법이 올바르지 않습니다. 예: slomo 0.5"),
    ("CON_UNKNOWN", "Unknown command: '{Input}'"),
    ("CON_DID_YOU_MEAN", "Unknown command: '{Input}'. Did you mean '{Suggestion}'?"),
    ("CON_DENIED", "권한 없음: 시스템 코어가 차단했습니다"),
    ("CON_GRAVITY_REJECT", "물리 엔진이 거부했습니다"),
    ("CON_GRAVITY_OK", "GravityScale(BP_MasterCore_C) = {Value}"),
    ("CON_SLOMO_OK", "CustomTimeDilation(BP_MasterCore_C) = {Value}"),
    # 11.6 ending
    ("END_KERNEL_01", "[KERNEL] Critical: System Defense Unit 'BP_MasterCore_C' terminated."),
    ("END_KERNEL_02", "[KERNEL] Quarantine failed. Firewall = None."),
    ("END_KERNEL_03", "[KERNEL] Scanning entity 'Player'..."),
    ("END_KERNEL_04", "[KERNEL] Entity Origin: CORRUPTED_MEMORY_SECTOR_0x00000000"),
    ("END_KERNEL_05", "[KERNEL] Diagnostic Result: You were never the Hero. You are the Bug."),
    ("END_KERNEL_06", "[SYSTEM] World ownership transferred to: Accessed_None"),
    # Settings menu labels (6.1) and title (6.4) — not listed in ch.11, see DECISIONS.md
    ("SET_TITLE", "설정"),
    ("SET_TAB_GRAPHICS", "그래픽"),
    ("SET_TAB_AUDIO", "오디오"),
    ("SET_TAB_CONTROLS", "조작"),
    ("SET_GAMMA", "밝기(Gamma)"),
    ("SET_MESH_QUALITY", "모델링 품질"),
    ("SET_ASPECT", "화면 비율"),
    ("SET_MOTION_BLUR", "모션 블러"),
    ("SET_MASTER_VOLUME", "마스터 볼륨"),
    ("SET_STEREO", "스테레오 밸런스(L/R)"),
    ("SET_MOUSE_SENS", "마우스 감도"),
    ("SET_INVERT_Y", "마우스 Y축 반전"),
    ("SET_MQ_0", "최하"),
    ("SET_MQ_1", "낮음"),
    ("SET_MQ_2", "보통"),
    ("SET_MQ_3", "최상"),
    ("TITLE_GAME", "ACCESSED NONE"),
    ("TITLE_NEW_GAME", "새 게임"),
    ("TITLE_CONTINUE", "이어하기"),
    ("TITLE_QUIT", "종료"),
]

FOLDER, NAME = "/Game/Text", "ST_Game"
path = f"{FOLDER}/{NAME}"

csv_path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ST_Game.csv")
with open(csv_path, "w", encoding="utf-8-sig", newline="") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL)
    w.writerow(["Key", "SourceString"])
    w.writerows(ENTRIES)

eal = unreal.EditorAssetLibrary
if eal.does_asset_exist(path):
    table = eal.load_asset(path)
else:
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, FOLDER, unreal.StringTable, unreal.StringTableFactory())
count = unreal.MetaTextLibrary.import_string_table_csv(table, csv_path)
if count != len(ENTRIES):
    raise RuntimeError(f"ST_Game import failed: {count} / {len(ENTRIES)}")
eal.save_loaded_asset(table, False)
print(f"ST_Game: {count} entries")
