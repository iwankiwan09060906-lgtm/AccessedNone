# ACCESSED NONE — 개발 시작 전 준비 체크리스트

> **상태 (2026-11-15): 완료된 초기 준비 문서입니다.** 개발은 Stage 3까지 진행됐습니다. 마스터 명세는 이 문서가 말하는 `AccessedNone_MasterPrompt.md` 대신 프로젝트 루트의 `CLAUDE.md`에 통합되어 있고, 진행 상황은 `Docs/PROGRESS.md`가 기준입니다. 아래 내용은 기록용으로 남겨 둡니다.

## 필요한 마켓(Fab) 에셋 팩 — Git에 없음 (2026-11-15)

용량(약 34GB)과 라이선스 때문에 아래 팩은 Git에 올리지 않습니다(`.gitignore`). 새 PC에서 클론한 뒤 Epic 런처/Fab에서 받아 **`Content/` 바로 아래에 같은 폴더 이름으로** 넣어야 맵·캐릭터가 열립니다.

| 폴더 | 팩 |
|---|---|
| `ParagonGreystone`, `ParagonSparrow`, `ParagonFengMao`, `ParagonTerra` | 플레이어 캐릭터 (숫자키 1~4) |
| `ParagonSevarog`, `ParagonGrux`, `ParagonNarbash`, `ParagonKhaimera`, `ParagonMuriel`, `ParagonRampage` | 보스·적 (E1 암살자 = Sevarog, E2 기사 = Grux, 엘베른·허수아비 = Narbash, 페이즈 1 잡몹 = Khaimera) |
| `InfinityBladeGrassLands`, `InfinityBladeIceLands`, `InfinityBladeCastle`, `InfinityBladeFireLands` | 지형 (오프닝 = GrassLands, 스테이지 1 = IceLands) |
| `InfinityBladeWarriors`, `InfinityBladeAdversaries`, `InfinityBladeWeapons`, `Infinity_Blade_Assets` | 페이즈 2~4 낙하 방어 개체 등 |
| `InfinityBladeEffects` | 눈·안개·불·폭발 파티클 |
| `EnvironmentPack2`, `EnvironmentPack3` | 오프닝 꽃밭·잔디 머티리얼 |

**로컬에서 수정한 마켓 에셋 (새로 받으면 다시 해야 함):**
- `ParagonTerra` 메시: 깨진 LOD1·LOD2 제거 + 고정밀 스킨 웨이트 (`DECISIONS.md` 2026-11-14 "Paragon Terra 메시 에셋 수정"). 안 하면 Terra 도끼가 깨져 보임.
- IceLands·GrassLands 일부 머티리얼의 Instanced Static Mesh 사용 플래그: `create_stage2_assets.py` / `create_stage3_assets.py`를 다시 실행하면 자동으로 켜짐.

이 문서의 모든 항목에 체크하면 Claude Code에게 **"Stage 1부터 시작해"** 한 마디로 개발을 처음부터 끝까지 맡길 수 있습니다.
기준 문서: `AccessedNone_MasterPrompt.md` (마스터 명세)

> ✅ = 2026-11-13에 Claude가 이 PC에서 확인함 / ⬜ = 사용자가 해야 할 일

---

## 1. 설치 (PC에 한 번만)

| 상태 | 항목 | 확인 방법 |
|---|---|---|
| ✅ | Unreal Engine 5.8 (`C:\Program Files\Epic Games\UE_5.8`) | — |
| ✅ | Visual Studio 2022 / 18 Community | — |
| ⬜ | VS Installer에서 워크로드 **"C++를 사용한 데스크톱 개발"** + **"C++를 사용한 게임 개발"** (Unreal Engine 구성 요소 포함), 최신 **Windows 11 SDK**, **MSVC v14.4x 빌드 도구** 체크 | 6장의 빌드 명령이 성공하면 OK |
| ✅ | Git 2.56 + Git LFS 3.8 | — |
| ✅ | Node.js 24 / npx (Unreal MCP 서버 실행용) | — |
| ⬜ | 엔진 **디버깅 심볼** (선택, 크래시 분석용): Epic 런처 → 라이브러리 → 5.8 ▾ → 옵션 → "디버깅을 위한 에디터 심볼" | — |
| ⬜ | 디스크 여유 공간 **50GB 이상** (D: 드라이브, DDC·빌드 산출물용) | — |

## 2. 마스터 명세를 프로젝트 안으로 옮기기

지금 명세 파일이 엔진 폴더(`C:\Program Files\Epic Games\UE_5.8\`)에 있습니다. 엔진 폴더는 관리자 권한이 필요하고 Claude의 작업 폴더도 아닙니다.

- [ ] `D:\UnrealEngine\AccessedNone\Docs\` 폴더를 만들고 `AccessedNone_MasterPrompt.md`를 그 안으로 **이동**
- [ ] 프로젝트 루트에 `CLAUDE.md` 생성 (Claude Code가 매 세션 자동으로 읽는 파일). 내용:

```markdown
# AccessedNone
- 유일한 기준 문서: Docs/AccessedNone_MasterPrompt.md — 작업 전 반드시 해당 장을 읽는다.
- 진행 상황: Docs/PROGRESS.md 를 먼저 읽고 이어서 작업한다.
- 엔진: C:\Program Files\Epic Games\UE_5.8
- 빌드: "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" AccessedNoneEditor Win64 Development -Project="D:\UnrealEngine\AccessedNone\AccessedNone.uproject" -WaitMutex
- 테스트: "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\UnrealEngine\AccessedNone\AccessedNone.uproject" -ExecCmds="Automation RunTests AccessedNone; Quit" -unattended -nullrhi -nosplash -log
```

## 3. 에디터 설정 (프로젝트를 열고 한 번)

### 3.1 플러그인 (편집 → 플러그인 → 체크 → 에디터 재시작)
Claude가 S1-T01에서 `.uproject`에 직접 추가하지만, **MCP 연결에 필요한 것은 미리** 켜야 합니다.

- [ ] **Python Editor Script Plugin** ← MCP 필수 (현재 `.uproject`에 없음)
- [ ] **Editor Scripting Utilities** ← MCP 필수
- [ ] (나머지는 Claude가 처리: GameplayAbilities, EnhancedInput, CommonUI, Niagara, Metasound)

### 3.2 프로젝트 세팅 (편집 → 프로젝트 세팅)
- [x] 플러그인 → Python → **Enable Remote Execution** = ON (✅ `DefaultEngine.ini`에 이미 `bRemoteExecution=True`)
- [ ] 같은 화면에서 **Multicast Bind Address**가 `127.0.0.1` 또는 `0.0.0.0`인지 확인 (방화벽 팝업이 뜨면 **허용**)

### 3.3 에디터 개인설정 (편집 → 에디터 개인설정)
- [ ] 일반 → 퍼포먼스 → **"백그라운드에서 CPU 사용량 줄이기" OFF** — 끄지 않으면 VS Code에 포커스가 있을 때 MCP 명령이 매우 느려짐
- [ ] 일반 → **Live Coding → "Live Coding 활성화" OFF** — 켜져 있으면 에디터가 열린 상태에서 `Build.bat`이 "Live Coding is active" 에러로 실패함
- [ ] 일반 → 로딩 및 저장 → 자동 저장 **ON** (기본값 유지)

## 4. 에셋 (명세 7.5 플레이스홀더 정책 기준 — 아트 에셋 없이도 전 Stage 진행 가능)

현재 프로젝트는 **빈(Blank) C++ 템플릿**이라 `Content/`가 비어 있습니다. 아래만 추가하면 됩니다.

| 상태 | 에셋 | 방법 | 용도 |
|---|---|---|---|
| ⬜ | **3인칭 템플릿 콘텐츠** (Manny/Quinn 메시·이동 애니메이션) | 콘텐츠 브라우저 **+ 추가 → 기능 또는 콘텐츠 팩 추가 → 블루프린트 → 3인칭** → 프로젝트에 추가 | 플레이어, 모든 보스 플레이스홀더 |
| ⬜ | **스타터 콘텐츠** (선택) | 같은 메뉴 → 콘텐츠 팩 → Starter Content | 석재/금속 머티리얼, 셰이프 |
| ⬜ | **한글 폰트** Noto Sans KR (Google Fonts, OFL 무료) `.ttf` | `Content/UI/Fonts/`에 드래그 → 폰트 페이스 + 컴포짓 폰트 생성 | 모든 UI 한글 텍스트 (기본 Roboto는 패키징 시 한글 깨짐 위험) |
| ⬜ | **고정폭 폰트** D2Coding 또는 JetBrains Mono `.ttf` | 위와 동일 | 콘솔·디버거·커널 로그 UI |
| ⬜ | (선택) Fab 무료 에셋 — 꽃밭/숲 환경, 오케스트라 BGM, SFX | Fab → 무료 필터 → 프로젝트에 추가 | 없으면 기본 셰이프·MetaSound 오실레이터로 대체됨 |

> 폰트 `.ttf` 파일만 `D:\UnrealEngine\AccessedNone\RawAssets\Fonts\`에 넣어 두면 임포트는 Claude가 Python으로 할 수도 있습니다.

## 5. Git 저장소 (명세 0.7 — Task 단위 커밋을 하려면 필요)

- [ ] 프로젝트 폴더에서 `git init` 후 `git lfs track "*.uasset" "*.umap"`
- [ ] `.gitignore` 는 Claude가 S1-T00에서 생성 (Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/)
- [ ] (선택) GitHub 비공개 저장소를 만들고 `git remote add origin ...` — 백업용

> 참고: 루트의 `.ignore` 파일이 `/Content`를 검색 제외합니다. 의도된 설정이면 그대로 두세요 (.uasset은 바이너리라 검색할 필요 없음).

## 6. Claude Code 연결 확인

- [x] Unreal MCP 서버 등록됨 (`@runreal/unreal-mcp`, `~/.claude.json`)
- [ ] **에디터를 켠 상태에서** Claude Code에 `/mcp` 입력 → `unreal` 이 **connected** 인지 확인
- [ ] Claude에게 "MCP로 프로젝트 정보 가져와봐" → 프로젝트 이름이 나오면 성공
- [ ] Claude에게 "빌드 한 번 돌려봐" → `Result: Succeeded` 확인 (1장 VS 구성 검증 겸용)
- [ ] (권장) 권한 팝업 줄이기 — `.claude/settings.json`에 추가:

```json
{
  "permissions": {
    "allow": [
      "Bash(*Build.bat*)",
      "Bash(*UnrealEditor-Cmd.exe*)",
      "Bash(git *)",
      "mcp__unreal__*"
    ]
  }
}
```

## 7. 개발 시작

모두 체크했다면 Claude Code(작업 폴더: `D:\UnrealEngine\AccessedNone`)에 이렇게 입력:

```
Docs/AccessedNone_MasterPrompt.md 를 읽고 16장 시작 지시대로 Stage 1 S1-T00부터 진행해.
각 Stage DoD 통과 시 다음 Stage로 넘어가고, 막히면 MANUAL_STEPS.md에 적고 나에게 알려줘.
```

### 개발 중 사용자가 계속 해야 하는 일
- 에디터는 **켜 둔 채로** 두기 (MCP는 에디터가 열려 있어야 동작). 빌드 때 Claude가 에디터를 닫아 달라고 하면 닫기
- `Docs/MANUAL_STEPS.md`에 항목이 생기면 클릭 단위 지시대로 수행 후 "했어"라고 알려주기
- 각 Stage 끝에 직접 **플레이(PIE)해 보고** 손맛·난이도 피드백 주기 — 자동 테스트로는 "재미"를 검증할 수 없음
- 세션이 끊겨도 `Docs/PROGRESS.md`에서 이어서 진행되므로 "PROGRESS.md 보고 이어서 해"라고 하면 됨
