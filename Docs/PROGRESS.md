# PROGRESS

상태: `TODO` / `IN PROGRESS` / `DONE` / `BLOCKED`

## Stage 1: 프로젝트 기반 아키텍처와 코어 전투

| ID | 작업 | 상태 | 검증 / 메모 |
|---|---|---|---|
| S1-T00 | 환경 점검 | DONE | UE 5.8.4, 빌드 성공(증분), Docs 4종, .gitignore 기존 충족, MCP 연결·능력 실검증 (MCP_CAPABILITIES.md) |
| S1-T01 | 모듈·플러그인 설정 | DONE | Build.cs 7.1 모듈 전부, .uproject 플러그인 7종 (로그로 마운트 확인), 엔진 콘솔 키 Tilde→F10. 빌드 성공(실컴파일), 프로젝트 경고 0 |
| S1-T02 | 기반 코드 (MetaLog, MetaGameplayTags, UAccessedNoneSettings) | DONE | 로그 카테고리 5개, 9장 태그 34개, 3장 수치 전부. 빌드 성공·경고 0. 에디터 런타임 확인은 MCP 재연결 대기로 미검증 |
| S1-T03 | 메타 서브시스템 | DONE | UMetaSettingsSubsystem, UMetaConsoleSubsystem(파서), UHintSubsystem, UMetaSaveGame. 빌드 경고 0. 테스트 3/3 통과 (ClampAndBroadcast, Console.Parser, Hint.Tiers). `ST_Game` 미생성으로 LogStringTable 경고 발생 → S2-T01에서 해소 |
| S1-T04 | GAS 코어 | DONE | UMetaCombatAttributeSet, UMetaDamageExecution(3.2 1~4단계), 반사·사망 이벤트(5~6단계), GE 12종 C++. 빌드 경고 0. 테스트 8/8 통과 (Combat 5종 신규) |
| S1-T05 | 플레이어 | DONE | AMetaCharacterBase/AMetaPlayerCharacter/AMetaPlayerController/AMetaGameMode, GA 4종, UMeleeHitboxComponent, 락온, 스태미나·대시, 입력 에셋 19종(`create_input_assets.py`). 빌드 경고 0, 테스트 9/9 (Input.Assets 신규). 헤드리스 게임 스모크: 플레이어 스폰·어빌리티 부여·입력 에셋 로드 에러 없음. **실제 키 입력 조작감은 미검증** → S1-T08 샌드박스 PIE에서 확인 |
| S1-T06 | 보스 프레임워크 | DONE | UGimmickComponent + 조건 4종, AMetaBossBase, UBossPatternComponent(틱 기반), UBossDataAsset/FBossPattern, UEncounterSubsystem, AEncounterTriggerBox. 빌드 경고 0, 테스트 12/12 (Gimmick.StateMachine 신규 + 선행 구현된 Gimmick.ConsoleAND, Dialogue.SwapAliases). **보스 액터·패턴·인카운터 서브시스템은 자동 테스트 없음** → S1-T08 샌드박스에서 검증 |
| S1-T07 | 시간 배율 | DONE | ESC 토글·IMC 전환·임시 메뉴(`UMetaTempMenuWidget`)·메뉴 잠금 차단, 개발 명령 `AN_Set`/`AN_Menu`/`AN_Status`/`AN_Encounter`. 비전투 ×1.0, 전투 중 ×0.15 (Sandbox.BossFlow에서 실게임 검증) |
| S1-T08 | 테스트 맵 `L_Dev_Sandbox` | DONE | `create_dev_sandbox.py`로 생성: 바닥·조명·PlayerStart·`BP_TestBoss`(ADevTestBoss, Gamma≥0.95)·`DA_Boss_Test`·트리거. `AccessedNone.Sandbox.BossFlow`(게임 내 실행) 통과: 진입 리셋 → ×0.15 → Armored 1% → Breaking → Vulnerable → 실제 GA 약공격 10회 처치 → 인카운터 종료 |

**Stage 1 DoD:** 빌드 경고 0 ✅ · 에디터 테스트 12/12 + 게임 테스트 1/1 ✅ · 스모크(게임 로그 Error 없음) ✅ · GameUserSettings 미사용 ✅ · **손 조작감 확인 대기 (MANUAL_STEPS #2)**

## Stage 2: 메타 UI 프레임워크와 오프닝 시퀀스

| ID | 작업 | 상태 | 검증 / 메모 |
|---|---|---|---|
| S2-T01 | String Table `ST_Game` | DONE | 11장 전체 + 설정/타이틀 UI 키 99개. 원본은 `create_string_table.py`, C++ `UMetaTextLibrary`로 임포트. `Text.AllKeysExist` 통과 |
| S2-T02 | HUD | DONE | `WBP_HUD`(HP/FP/스태미나·퀵슬롯·토스트·배너·로그·튜토리얼), `WBP_BossBar`, 플로팅 데미지(`FMetaCombatEvents::OnDamage`). 스크린샷 확인 |
| S2-T03 | 가상 설정창 | DONE | `WBP_Settings` 3탭 8항목 즉시 적용·외부 변경 동기화·사망 8회 점멸, `AMetaPostProcessController`(감마·모션블러·마스터볼륨). 전투 중 ESC ×0.15 게임 테스트로 확인. **8회 점멸은 화면 미확인** (S3 힌트 테스트에서) |
| S2-T04 | 디버거 터미널 | DONE | `WBP_Debugger` 큐 + 실시간 타자기(0.03초) + 키 클릭음. 스크린샷 확인 |
| S2-T05 | 오류창 | DONE | `WBP_ErrorWindow`, `UMetaDraggableNumberWidget`, `UMetaNumberDragOperation`, 클릭-클릭 모드, 30/60초 힌트, 오버플로우 연출. `ErrorWindow.SwapRatio`/`InvalidDrop` 통과. **실제 마우스 드래그는 미검증** (MANUAL_STEPS #4) |
| S2-T06 | 오버플로우 체력바 | DONE | `WBP_BossBar_Overflow` 0.1초 간격 12줄. 스크린샷 확인 |
| S2-T07 | 타이틀 | DONE | `L_Title` + `WBP_Title`(새 게임/이어하기(저장 있을 때)/종료), 기본 맵으로 설정 |
| S2-T08 | 페이즈 1 | DONE | `L_Prologue_FlowerField`(달밤 꽃밭), `BP_FakeBoss_Elbern`(Paragon Sevarog, HP D×15, 데미지 100), 튜토리얼 5단계, 성광 + `DEMON VANQUISHED` |
| S2-T09 | 페이즈 2~4 | DONE | 테이프 스톱(피치 1→0.3), 글리치, `M_SkyCrack`(보라 균열+붉은 노드), IB 번개, 방어 개체 낙하(`ADefenseEntity`, `M_WorldGrid`), 로그 5줄, 오버플로우 바, 끊기는 휘두르기, 참격, 강제 즉사(3.4초) |
| S2-T10 | 페이즈 5 | DONE | `Player = None`, 디버거 첫 등장, 오류창 파훼 → `L_Stage1_DarkGrove`(플레이스홀더) 로드 |

**Stage 2 개정 (사용자 지시, 2026-11-14):** 페이즈 1 맵 확장(GrassLands 대로·던전, Khaimera 잡몹 3 + Narbash 소환 보스), 깨지는 하늘 + 디지털 레인, IB Warriors 40체 낙하 + 거대 폭발 강제 패배, 게임풍 사망 화면, 오버플로우 2배, 플레이어·보스 공격 모션. `Flow.Opening` 갱신 후 통과, 스크린샷 확인.

**Stage 2 개정 2 (사용자 지시):** 큰 구멍 하늘 + 디지털 레인, 낙하 개체 무작위 포즈, 오버플로우 체력바 삭제, 카메라 거리 고정(캐릭터 무시), 대시 → 구르기(공중제비 모션), 좌하단 HUD(물약 3개 표시), 타이틀 3D 배경 + UI 리디자인. 테스트 전부 통과, 스크린샷 확인.

**Stage 2 개정 3 (사용자 지시):** 적 사망 모션 + 2초 후 소멸, 샌드박스 허수아비(무공격·부활), 캐릭터별 사거리(1=4<3), 2번 화살 + 조준 표시. 게임 테스트 `Sandbox.Practice` 추가. 에디터 15/15, 게임 3/3 통과.

**Stage 2 개정 4 (사용자 지시):** 차지 강공격(0.8초 홀드 후 떼기) + 별도 모션, 적 추격 버그 수정 + 어그로 범위, 궁수 공격 시에만 조준 방향 회전. 게임 테스트 `Flow.EnemyChase` 추가. 에디터 15/15, 게임 4/4 통과.

**Stage 2 개정 5 (사용자 지시):** 구르기(정면·쿨타임 1초·스태미나·무적), 플레이어 피격 모션, 적 타격 지점 이펙트, 강공격 Shift+클릭·0.8초 후 2배 데미지. 게임 테스트 `Sandbox.Roll` 추가. 에디터 15/15, 게임 5/5 통과.

**Stage 2 개정 6 (사용자 지시):** Paragon 타격 이펙트, 보스 3배(판정·사거리 비례), 플레이어 사망 모션. 에디터 15/15, 게임 5/5 통과.

**Stage 2 개정 7 (사용자 지시):** 락온 대상 크기에 비례한 카메라 거리(400 → 최대 750, 부드럽게). 게임 테스트 `Sandbox.Camera` 추가. 에디터 15/15, 게임 6/6 통과.

**Stage 2 DoD:** 빌드 경고 0 ✅ · 에디터 테스트 15/15 + 게임 테스트 2/2 (`Flow.Opening`, `Sandbox.BossFlow`) ✅ · 타이틀 → 엔딩 직전 스테이지 1 진입까지 한 번에 진행(`Flow.Opening`, 렌더링 + 스크린샷 14장) ✅ · GameUserSettings 미변경 ✅ · **손 조작 확인 대기 (MANUAL_STEPS #4)**

## Stage 3: 스테이지 1 인카운터 (E1 암살자 & E2 가시 기사)

| ID | 작업 | 상태 | 검증 / 메모 |
|---|---|---|---|
| S3-T01 | 브레이크포인트 | DONE | `ABreakpointActor`(붉은 구, `[E]` 상호작용 → HP·회복약 회복, 저장, 리스폰 지점), `IMetaInteractable`, `UMetaSaveSubsystem`(저장·이어하기·처치 기록), `AMetaGameMode` 사망 → 사망 화면 → 2.5초 후 제자리 부활(레벨 재로드 없음), 3/5회 디버거 힌트(`DA_Hint_E1/E2`), 8회 메뉴 점멸. `Flow.Continue` 통과 |
| S3-T02 | 레벨 | DONE | `L_Stage1_DarkGrove`: 어두운 숲(나무·고사리·뿌리·바위, 짙은 안개), 출발 브레이크포인트, E1 숲속 공터 + 비석 `E1_STELE`(`AMetaInscription`), 두 번째 브레이크포인트, 로우폴리 파편, E2 폐허 홀 |
| S3-T03 | E1 | DONE | `AAssassinBoss`/`BP_Assassin`/`DA_Boss_Assassin`(Paragon Sevarog): 은신(메시 숨김 + 눈 반짝임, 1회 사망 후 더 자주) + 무적 + 즉사 기습, 감마 20% 강제, ≥95% → 0.8초 화이트아웃에 드러남 → 6초 스턴 → 약공격 3회, 처치 후 감마 2초에 50%로 + `E1_CLEAR` |
| S3-T04 | E2 | DONE | `AThornKnightBoss`/`BP_ThornKnight`/`DA_Boss_ThornKnight`(Paragon Grux, 3배): 가시 26개(`Spike_Hitbox_*`), `Triangles: 48,000,000` 표시, 등장 시 프레임 드랍 연출, 99% 감쇄 + 300% 반사, 품질 3 강제, 0 → 0.6초 가시 디졸브 + 최저 LOD + 가시 콜리전 off → 약공격 10회 |

**Stage 3 DoD:** 빌드 경고 0 ✅ · 에디터 15/15 + 게임 9/9 ✅ · `Stage1.Encounters`(게임 내): E1·E2 각각 ① 정공법 불합리(무적 / 1% + 300% 반사) ② 사망 1·3·5·8회 힌트 단계(디버거 3·5회, 메뉴 점멸 8회), 매번 3초 내 브레이크포인트 부활 ③ 설정으로 파훼 후 처치 ✅ · 미리 맞춘 설정 진입 시 리셋(`Encounter.PreSetReset` 내용을 같은 테스트에 포함) ✅ · 시각 확인 `Stage1.Tour` 스크린샷 ✅ · **손 조작 확인 대기 (MANUAL_STEPS #6)**

## Stage 4~6

TODO
