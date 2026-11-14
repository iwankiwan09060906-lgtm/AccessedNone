# DECISIONS

| 날짜 | 결정 | 이유 |
|---|---|---|
| 2026-11-14 | 마스터 명세는 별도 파일 없이 루트 `CLAUDE.md`를 기준 문서로 사용 | 이미 전문이 `CLAUDE.md`에 있고 매 세션 자동 로드됨 |
| 2026-11-14 | Python Remote Execution 멀티캐스트 바인드 주소를 `0.0.0.0`으로 설정 (`DefaultEngine.ini`) | 기본값(127.0.0.1)에서 MCP가 원격 노드를 찾지 못함 |
| 2026-11-14 | C++ 빌드는 `Scripts/Editor/build.sh`로: MCP로 저장·종료 → 빌드 → 에디터 재실행 (사용자 승인) | 에디터가 켜져 있으면 LiveCodingConsole이 남아 UBT가 빌드를 거부함 |
| 2026-11-14 | `Build.cs`에 `PublicIncludePaths.Add(ModuleDirectory)` | `Core/…`, `Meta/…` 하위 폴더 경로로 include |
| 2026-11-14 | `FMetaConsoleResult`에 `TextKey`(CON_* 키), `Suggestion` 필드 추가 | 테스트·UI가 번역문 비교 없이 결과 판정 |
| 2026-11-14 | `UMetaSettingsSubsystem`에 네이티브 델리게이트 `OnSettingChangedNative` 병행 | C++ 소비자(보스·PP·테스트)가 UFUNCTION 없이 람다 바인딩 |
| 2026-11-14 | 디버거 메시지 큐 입구를 `UHintSubsystem::PushDebuggerMessage` / `OnDebuggerMessage`로 단일화 | 설정 덮어쓰기 로그와 힌트가 같은 터미널로 감 (7.2 HintSS→DebuggerUI) |
| 2026-11-14 | 힌트 단계 값 = 사망 임계값 그대로 (0/1/3/5/8) | 6.3 표와 1:1 대응, 별도 enum 불필요 |
| 2026-11-14 | 콘솔 `god`/`kill`은 인자가 있어도 `CON_DENIED` | 의도가 명확한 입력은 거부 응답이 자연스러움 |
| 2026-11-14 | 텍스트 조회는 `MetaText::Get(Key)` → `/Game/Text/ST_Game.ST_Game` | 7.4 String Table 규칙, 에셋은 S2-T01에서 생성 |
| 2026-11-14 | GE는 전부 C++ 파생 클래스(`UGE_*`, 베이스 `UMetaGameplayEffect`)로 구현 | 8.4 허용안. 그래프 자산 최소화, 수치는 `UAccessedNoneSettings`에서 |
| 2026-11-14 | 메타 어트리뷰트 `IncomingRawDamage` 추가 | 반사(3.2-6)는 감쇄 전 Raw 기준인데 PostGameplayEffectExecute에서는 Raw를 알 수 없음. Execution이 Raw→Final 순서로 출력 |
| 2026-11-14 | `Event.Hit.Immune` 게임플레이 이벤트는 Execution에서 송신 | 무적(3단계)이면 출력 모디파이어가 없어 AttributeSet 콜백이 호출되지 않음 |
| 2026-11-14 | `GE_InstantKill`에 FixedDamage 미지정 시 `BossSkillDamage`(99,999) 사용 | GE가 SetByCaller를 스스로 가질 수 없음 |
| 2026-11-14 | AttributeSet 복제 없음 | 싱글 플레이 전용 |
| 2026-11-14 | 입력 바인딩은 `AMetaPlayerController`가 소유하고 폰에 전달 | E3 빙의(`AStatuePawn`) 후에도 조작이 그대로 유지 |
| 2026-11-14 | 입력 에셋은 `/Game/Core/Input/`에서 이름으로 로드 (생성자 FObjectFinder 미사용) | 에셋이 생기기 전 CDO 생성 시 에러 로그 방지 |
| 2026-11-14 | 마우스 감도·Y반전은 입력 모디파이어 대신 C++ `Look()`에서 가상 설정값 적용 | 가상 설정 변경이 즉시 반영, 에셋 수정 불필요 |
| 2026-11-14 | 스태미나: 0보다 많으면 행동 허용, 비용만큼 차감 후 0으로 클램프 | 소울라이크 관례 |
| 2026-11-14 | 근접 판정은 활성 창 시작·끝 2회 박스 오버랩 (중복 타격 방지) | 애니메이션 없는 전투(7.5)의 최소 구현. 빠른 대상이 빠지면 틱 스윕으로 확장 |
| 2026-11-14 | 구르기는 `ApplyRootMotionConstantForce` (속도 1400, 0.4초), 입력 없으면 백스텝 | 애니메이션 없이 일정한 이동 거리 |
| 2026-11-14 | 플레이어 공격 타이밍·히트박스·이동 속도를 `UAccessedNoneSettings`에 [기본값]으로 추가, 전 항목 BlueprintReadOnly | 매직 넘버 금지, Python 스크립트가 같은 값 사용 |
| 2026-11-14 | `GameViewportClientClassName=CommonGameViewportClient` | CommonUI 입력 라우팅 필수 (미설정 시 시작 로그에 Error) |
| 2026-11-14 | `UBossPatternComponent`는 타이머 대신 컴포넌트 틱 상태 머신 | 액터 `CustomTimeDilation`(E6 slomo)이 윈드업·쿨다운·이동에 함께 적용됨. 월드 타이머는 액터 배율을 무시 |
| 2026-11-14 | Breaking 중 피격 무효는 루즈 태그 `State.Breaking` + Execution에서 IMMUNE 이벤트 없이 0 처리 | 3.5 "Breaking 동안 피격되지 않음"; IMMUNE 텍스트는 무적 상태 전용 |
| 2026-11-14 | `SYS_SETTING_OVERRIDE`의 `{Boss}`는 보스 액터 클래스명 (`BP_Assassin_C` 등) | 5.0 예시와 동일하게 BP 생성 클래스명이 그대로 나옴 |
| 2026-11-14 | 인카운터는 동시에 1개, 보스 여러 체 가능, 전원 Defeated 시 종료 | E4 쌍둥이·E5 군단 5체 대응 |
| 2026-11-14 | `UBossDataAsset.HPMultiplierOverride` 추가 | 페이즈 1 가짜 보스 HP `D×15` (3.4) |
| 2026-11-14 | `UBossDataAsset.HintAsset`은 S3-T03에서 `UHintDataAsset`과 함께 추가 | 아직 사용처 없음 |
| 2026-11-14 | DialogueSwap·UIDragDrop 조건은 판정 로직만 구현, 위젯/대화 시스템이 `EvaluateNames`/`SetFraction` 호출 | 해당 UI는 S2-T05, S4-T01에서 생성 |
| 2026-11-14 | 보스 패턴은 보스끼리 피해를 주지 않음 (히트 대상 = 인카운터 대상 플레이어만) | E4/E5 다수 보스 |
| 2026-11-14 | 테스트 보스 조건은 C++ `ADevTestBoss` 생성자에서 설정, 데이터는 `BP_TestBoss`/`DA_Boss_Test` | Instanced 조건 객체를 Python으로 BP CDO에 넣는 것은 불안정 |
| 2026-11-14 | 내비메시가 없으면 보스는 대상에게 직선 이동 (`AddMovementInput`) | 샌드박스에 NavMeshBounds를 Python으로 만들 수 없음(브러시 미생성). T-포즈 슬라이딩 콘셉트와 일치 |
| 2026-11-14 | Stage DoD의 PIE 흐름은 `-game` + 지연 자동화 테스트(`Sandbox.BossFlow`, ClientContext)로 검증 | MCP는 빌드마다 재연결 필요. 게임 프로세스 안에서 실시간 대기하며 반복 실행 가능 |
| 2026-11-14 | 개발 명령 `AN_Set/AN_Menu/AN_Status/AN_Encounter` (PlayerController Exec) | S1 DoD "임시 디버그 명령 허용" |
| 2026-11-14 | `PythonScriptPlugin`, `EditorScriptingUtilities`를 S1-T01 전에 `.uproject`에 선반영 | MCP 연결의 전제 조건 |
| 2026-11-14 | 플레이어 캐릭터를 Paragon 4종(1 Greystone / 2 Sparrow / 3 FengMao / 4 Terra)으로 교체, 숫자키 1~4로 `SetHero` (메시·ABP 교체). 정의는 `UAccessedNoneSettings::Heroes`. 스탯은 기사 그대로 | 사용자 요청. 캐릭터 선택 화면은 추후 개발 |
| 2026-11-14 | 궁수(Sparrow)의 원거리 공격은 긴 히트박스(오프셋 650)로 임시 구현 | 투사체(`AMetaProjectile`)는 S5-T04. 그때 교체 |
| 2026-11-14 | 테스트 보스(`ADevTestBoss`)를 Paragon Narbash 메시+ABP로 변경. 마네킹 참조 제거(에셋은 삭제하지 않음) | 사용자 요청 ("다른 에셋은 건드리지 마") |
| 2026-11-14 | 캐릭터 교체 시 `EmptyOverrideMaterials` + 천 시뮬레이션 재생성 + Paragon 인트로 몽타주(LevelStart/SelectScreen) 1초간 정지 (`AMetaCharacterBase::SuppressIntroMontages`, 보스도 적용) | Paragon ABP가 인트로 연출 중 슬롯별 머티리얼 오버라이드를 남겨, 다음 캐릭터가 이전 캐릭터의 머티리얼로 그려졌음(3·4번 머리 사라짐, 무기 늘어짐) |
| 2026-11-14 | 시각 확인용 `AccessedNone.Sandbox.HeroShots` (렌더링 필요, `Saved/Screenshots`) | MCP 없이 스크린샷으로 캐릭터 모습 검증. Paragon 에셋의 BP 컴파일 Error 로그 때문에 종료 코드는 -1, 판정이 아니라 시각 확인용 |
| 2026-11-14 | Terra(4번)의 도끼 머티리얼 슬롯 `M07_Justice_Weapon`을 모든 LOD에서 숨김 (`FHeroDef::HiddenMaterialSlots`) | UE 5.8이 Terra 메시를 로드 시 재빌드하면서 도끼 섹션(본 1개) 스킨 데이터가 깨져 애니메이션 중 늘어남. Paragon 원본 `TerraPlayerCharacter`도 동일. 기본 자세·도끼 숨김 시 정상 확인(HeroMotion 스크린샷). 에셋 수정은 사용자 승인 전까지 보류 |
| 2026-11-14 | (사용자 승인) Paragon Terra 메시 에셋 수정: 깨진 LOD1·LOD2 제거(5→3 LOD), 전 LOD `UseHighPrecisionSkinWeights=true`, 저장. 도끼 숨김 우회 코드 제거 | LOD1·2의 도끼 섹션 버텍스가 본 16개에 흩어져 바인딩되어 애니메이션 중 늘어남(LOD0·3·4는 `weapon_slide` 1개로 정상). 원본 백업: `Saved/AssetBackups/Terra_original.uasset` |
| 2026-11-14 | `ST_Game`의 원본은 `Scripts/Editor/create_string_table.py`의 표. CSV로 써서 C++ `UMetaTextLibrary::ImportStringTableCSV`로 임포트 | Python은 String Table을 편집할 수 없음. 텍스트를 한 곳에서 diff 가능하게 관리 |
| 2026-11-14 | 11장에 없는 UI 키 추가: `SET_*`(설정창 라벨·선택지), `TITLE_*`(타이틀) | 7.4: 모든 사용자 노출 텍스트는 ST_Game. 비율 `16:9`/`4:3`, 숫자, 스테레오 `L/C/R`은 리터럴 |
| 2026-11-14 | `MetaText::Get`이 첫 호출 시 `ST_Game`을 로드·루트 고정 | 미로드 상태에서 `<MISSING STRING TABLE ENTRY>` 표시(타이틀에서 확인됨) |
| 2026-11-14 | 폰트는 Slate 기본(Roboto + DroidSansFallback, 모노는 DroidSansMono). `Content/UI/Fonts`의 NotoSansKR는 임포트하지 않음 | 기본 폰트로 한글 정상 표시 확인(스크린샷). 에셋 추가 불필요 |
| 2026-11-14 | 모든 위젯 트리는 C++ `RebuildWidget`에서 구성, `WBP_*`는 부모 클래스만 지정. 코드는 `MetaUI::WidgetClass`로 WBP를 찾고 없으면 C++ 클래스 사용. `MetaUI::NeedsBuild`가 빈 루트 패널을 C++ 트리로 교체 | MCP/Python으로 위젯 트리 편집 불가 (MCP_CAPABILITIES) |
| 2026-11-14 | 플로팅 데미지는 전역 네이티브 델리게이트 `FMetaCombatEvents::OnDamage`(AttributeSet·Execution에서 송신) | HUD가 모든 전투원 피격을 하나의 구독으로 받음 |
| 2026-11-14 | HUD가 배너·시스템 로그·튜토리얼·화면 틴트·토스트도 담당. 페이즈 3 로그·대사는 HUD 로그 패널(좌상단)에 출력 | 디버거 터미널은 4.5에서 "첫 등장"이므로 페이즈 3에서 쓰지 않음 |
| 2026-11-14 | HUD의 FP 바는 장식(항상 가득) | 원 기획에 FP 소모 규칙이 없음. 4.1 "정상적인 소울라이크 HUD" 외형만 재현 |
| 2026-11-14 | `MS_TapeStop` 대신 BGM `UAudioComponent::SetPitchMultiplier` 1.0→0.3(2초) 후 정지. `DefaultEngine.ini` `GlobalMinPitchScale=0.1` | 리샘플링 피치는 피치와 재생 속도가 함께 변해 테이프 스톱과 같음. MetaSound 그래프 자동 생성은 불안정(0.3③) |
| 2026-11-14 | BGM·키 클릭음은 `create_audio.py`가 합성한 WAV(`SW_BGM_Prologue` 16초 루프 현악 패드, `SW_KeyClick`) | 콘텐츠에 음악 에셋 없음 (12.4 대체 규칙) |
| 2026-11-14 | `NS_SkyCrack`/`NS_SlashX999` 대신 Infinity Blade Cascade 파티클(`P_DOT_Lightning_01`, `P_HeroSwipe_Fire`)을 `APrologueDirector`의 소프트 참조로 사용 | Niagara 이미터 그래프는 Python으로 구성 불가(0.3③). 사용자 제공 에셋 활용 |
| 2026-11-14 | 하늘 균열 = 하늘 평면 + `M_SkyCrack`(커스텀 HLSL: 들쭉날쭉한 원형 개방, 보라 테두리, 내부 `T_VoidNodes` 붉은 펄스, 포그 미적용). `T_VoidNodes`는 스크립트가 PNG로 생성 | 4.2 "직접 제작한 노드 텍스처". 파라미터 `CrackProgress`만 C++에서 제어 |
| 2026-11-14 | `M_WorldGrid` = 커스텀 HLSL 월드 체크무늬(#7F7F7F/#5A5A5A) + WPO 정점 폭발(`ExplodeAmount`) | 4.3 비주얼 규칙을 노드 2개로 |
| 2026-11-14 | `UBossDataAsset`에 `Mesh`/`AnimClass`/`MeshScale` 추가, `AMetaBossBase::BeginPlay`에서 적용. `BP_FakeBoss_Elbern`은 데이터 전용 BP(Paragon Sevarog) | 보스마다 C++ 서브클래스 불필요 |
| 2026-11-14 | 방어 개체 = `ADefenseEntity`(Manny 레퍼런스 포즈, AI 없음, 무적, 컨트롤러 없이 중력 낙하) | 4.2~4.4. T-포즈 확인(스크린샷) |
| 2026-11-14 | 오프닝 연출은 `Story/APrologueDirector`(새 폴더 `Story/`) 하나의 페이즈 머신. 장면 액터는 태그(`SkyCrack`, `HolyLight`)로 찾음 | 레벨 시퀀스 대신 C++ (0.2-2, 그래프 자산 최소화) |
| 2026-11-14 | 페이즈 1에서 사망하면 3초 후 프롤로그 재시작. 튜토리얼 단계는 해당 행동 시 또는 12초 후 진행 | 브레이크포인트는 S3-T01. 튜토리얼이 진행을 막지 않게 |
| 2026-11-14 | 페이즈 4 즉사는 시작 3.4초에 스크립트로 `GE_InstantKill` 적용(방어 개체가 플레이어 옆으로 스냅). 그 순간 일반 데미지 숫자를 끄고 `OP_P4_DAMAGE` 표시 | 입력과 무관하게 5초 내 사망 보장. 실제 수치(99,999)와 연출 수치 충돌 방지 |
| 2026-11-14 | 오류창 규칙은 순수 구조체 `FMetaErrorWindowState`. `UGimmickCondition_UIDragDrop`은 오류창에 쓰지 않음(보스 없음, 판정식 동일) | Slate 없이 테스트 가능 |
| 2026-11-14 | 새 게임은 가상 설정·힌트 카운터만 초기화하고 저장 파일은 지우지 않음 | 데이터 손실 방지. 저장은 다음 브레이크포인트에서 덮어씀 |
| 2026-11-14 | `AMetaGameMode::StartPlay`가 레벨에 `AMetaPostProcessController`가 없으면 스폰. 마스터 볼륨은 `FAudioDevice::SetTransientPrimaryVolume`(게임 믹스만) | 8.9 "레벨마다 1개"를 레벨 작업 없이 보장. 1.3 OS 오디오 불변 |
| 2026-11-14 | 감마 매핑: 0.5 중립, ±0.5 → ±3 EV + ColorGamma ±0.6, 0.95 이상 추가 +3 EV(화이트아웃). 전환 속도 상한 = 전체 구간 0.8초, 실시간 델타 | 12.3 화이트아웃 ≥0.8초, 메뉴 ×0.15 중에도 즉시 반영 |
| 2026-11-14 | (에셋 수정) Infinity Blade 꽃·풀·나무 머티리얼에 Instanced Static Mesh 사용 플래그 추가 (`create_stage2_assets.py`가 설정) | 플래그가 없으면 인스턴싱 시 기본 체크 머티리얼로 렌더링됨. 비파괴적 플래그 변경 |
| 2026-11-14 | `AMetaScatterField`(HISM)로 꽃밭·숲 배치 | Python으로 폴리지 페인팅 불가 |
| 2026-11-14 | `GameDefaultMap=L_Title`, `GlobalDefaultGameMode=MetaGameMode` | 6.4 타이틀에서 시작 |
| 2026-11-14 | `L_Stage1_DarkGrove`는 바닥·조명·나무·PlayerStart만 있는 플레이스홀더 (스크립트는 없을 때만 생성) | S2-T10 "없으면 빈 레벨 생성". 본 구성은 S3-T02 |
| 2026-11-14 | `Flow.Opening` 게임 테스트는 Paragon BP의 `ResetOrientationAndPosition` 컴파일 에러를 예상 에러로 등록 | 에셋 팩의 기존 결함(5.8 비호환)이며 이 흐름과 무관 |
| 2026-11-14 | (사용자 지시, 4.1 변경) 페이즈 1 = 튜토리얼 + 가짜 잡몹 Paragon Khaimera 3체(`P1_Minions`, HP D×3, 이름 없음) → 약 15초 길이의 폐허 대로 → GrassLands 폐허 던전에서 가짜 보스 Paragon Narbash가 소환(`P1`, `P_Summon_Portal`). 보스 표시명은 기존 `OP_P1_BOSS_NAME`(엘베른) 유지 | 사용자 요청. 보스는 인카운터 시작 전까지 숨김 |
| 2026-11-14 | (사용자 지시, 4.2 변경) 하늘 균열 = 유리처럼 깨지는 보로노이 파편(흰·보라 번개 테두리) + 파편 안쪽은 검정 배경의 초록 숫자 디지털 레인(`T_DigitAtlas`, 5×7 비트맵 숫자). 균열은 페이즈 2 시작 시 카메라 정면 하늘에 배치 | 사용자 참고 이미지. `T_VoidNodes` 삭제 |
| 2026-11-14 | (사용자 지시, 4.2~4.4 변경) 낙하 개체 = Infinity Blade Warriors 17종 무작위, 40체(0.15초 간격), 레퍼런스 포즈(T-포즈) + 원래 머티리얼. 강제 패배 = 거대 폭발(IB 폭발 파티클 ×8 스케일, 섬광 1회 1.6초 페이드, 합성 폭발음 `SW_Explosion`, 개체들 날아감) 후 즉사. 기존 T-포즈 접근·끊기는 휘두르기·참격은 제거, 오버플로우 체력바는 유지 | 사용자 요청. 섬광은 단일 페이드라 12.3 준수 |
| 2026-11-14 | (사용자 지시, 4.5) 사망 화면 = `WBP_DeathScreen`(암전, 붉은 띠 펼침, 줌인되는 `Player = None` + 색수차 글리치, 엔진 에러 한 줄 타이핑 `UI_DEATH_SUB`) | "게임 느낌" 업그레이드 |
| 2026-11-14 | (사용자 지시) 오버플로우 경고창 14→28개, 같은 4초 안(0.1초 간격) | 2배 빠른 에러 표시 |
| 2026-11-14 | 공격 모션: 플레이어는 `FHeroDef.LightAttackAnims`(Paragon 기본공격 몽타주 콤보, 1.2초 내 연속 시 다음 단계) / `HeavyAttackAnim`(느린·능력 시퀀스를 기본공격 몽타주의 슬롯에 동적 몽타주로). 보스는 `FBossPattern.OptionalMontage` 재생. 모두 재생 속도 = 길이 ÷ (윈드업+활성+회복) | 애니메이션 없는 전투 규칙(7.5)의 타이밍과 모션을 맞춤 |
| 2026-11-14 | Paragon 인트로 디졸브가 남기는 머티리얼 오버라이드를 인트로 억제 시·억제 창 종료 시 모든 캐릭터에서 제거 (`AMetaCharacterBase`) | 보스에 Paragon 메시를 데이터로 입히면 완전히 디졸브된 상태(투명)로 남았음(스크린샷으로 확인) |
| 2026-11-14 | 이름(`DisplayNameKey`) 없는 보스는 HUD 보스 바를 띄우지 않음 | 소울라이크 관례: 잡몹에는 보스 바 없음 |
| 2026-11-14 | 횃불 포인트 라이트는 칸델라 40~60 | 9000으로 두었더니 자동 노출이 하얗게 날아감 |
| 2026-11-14 | 머티리얼 `ComponentMask`는 모든 채널을 명시적으로 지정 | R 기본값이 켜져 있어 Opacity가 빨강 채널을 읽어 균열 내부가 투명했음 |
| 2026-11-14 | `AMetaScatterField.CorridorHalfWidth`(직선 통로 비우기), `ADefenseEntity::BlastAway`, `AN_Status`에 보스 위치·메시 출력 | 대로 위 나무/꽃 배제, 폭발 연출, 디버깅 |
| 2026-11-14 | (사용자 지시, 4.2 재개정) 하늘 균열 = 하나의 큰 구멍(16꼭짓점 불규칙 다각형 + 날카로운 파편 가장자리, 가장자리 조각이 떨어져 나감) 안에 빽빽한 초록 숫자 디지털 레인, 바깥으로 깜빡이는 번개 균열 10줄과 잔금. IB 번개 파티클은 제거 | 사용자: "하늘이 깨지면서 큰 구멍, 그 안에 초록 숫자". 파티클 리본이 조잡해 보였음 |
| 2026-11-14 | (사용자 지시) 낙하 개체는 `UPoseableMeshComponent`에 무작위 관절 포즈(UE4 마네킹 본 15개) + 낙하 중 회전, 착지 시 바로 섬 | IB Warriors 팩에는 애니메이션·피직스 에셋이 없음 |
| 2026-11-14 | (사용자 지시, 4.4 ② 삭제) 오버플로우 체력바(`WBP_BossBar_Overflow`, `UMetaBossBarOverflowWidget`) 제거. 폭발은 페이즈 4 시작 1.0초, 사망 1.3초 | "게임 내용에 불필요" |
| 2026-11-14 | (사용자 지시, 2.1 변경) 대시 제거, `Space` 누름 = 구르기(`IA_Dodge` Pressed, `IA_Dash` 매핑 없음). 구르기는 메시를 몸 중심 기준 360° 공중제비(0.55초, 1100cm/s), 입력 없으면 정면 유지한 채 뒤로 구르기 | "대시 스킬을 구르는 스킬로". 구르기 애니메이션 에셋이 없음 |
| 2026-11-14 | 카메라 거리 고정: 모든 캐릭터의 캡슐·메시가 `ECC_Camera`를 무시 (지형·벽은 여전히 카메라를 당김) | 적과 붙으면 스프링암이 적에 막혀 카메라가 근접했음. 벽까지 무시하면 벽 너머가 보임 — 완전 고정이 필요하면 `bDoCollisionTest=false`로 전환 |
| 2026-11-14 | (사용자 지시) HUD = 좌하단: HP 바(오른쪽에 물약 아이콘 3개 + 개수, 사용한 물약은 빈 병) + 스태미나 바, 흐려지는 검은 배경. 바는 금빛 테두리 + 피해 잔상(Trail). FP 바 제거. 튜토리얼은 왼쪽 중앙, 보스 바는 하단 중앙 위쪽. 회복약은 기존대로 최대 3개(`HealFlaskMax`), 브레이크포인트에서만 충전 | "체력·스태미나·회복 아이템을 왼쪽 아래" |
| 2026-11-14 | (사용자 지시) 타이틀 = `L_Title`에 달밤 폐허(아치·천사상·꽃·벚나무) 3D 배경 + `TitleCamera`, 비네트, 자간 넓힌 금색 제목과 장식선, 하단 어두운 띠 위 텍스트 메뉴(호버 시 금빛 띠), 타이틀 BGM | "게임 시작 UI를 분위기에 맞게" |
| 2026-11-14 | UI 텍스처(`T_UI_GradientH/GradientCenter/Vignette/Flask`)는 스크립트가 PNG로 생성 (`/Game/UI/Textures`) | 외부 에셋 없이 재생성 가능 |
| 2026-11-14 | (사용자 지시) 적 사망 = `UBossDataAsset.DeathAnim` 단일 노드 재생(마지막 프레임 유지) → 모션 길이 + `VanishDelay`(2초) 뒤 숨김·충돌 해제. 인카운터 리셋(Dormant) 시 다시 나타남. 잡몹 Khaimera `Death_A`, 엘베른/샌드박스 Narbash `Death_Fwd` | "쓰러지는 모션 후 2초 뒤 사라짐". Destroy 대신 숨김: 트리거·리셋이 액터 참조를 유지 |
| 2026-11-14 | (사용자 지시) `L_Dev_Sandbox` 보스 = 허수아비(`bTrainingDummy`): 패턴·이동 없음, 기믹(감마)과 감쇄는 그대로, 사라진 뒤 1.5초에 부활해 인카운터 재시작(`UEncounterSubsystem::RestartEncounter`) | 타격 연습용. 부활이 없으면 한 번 처치 후 연습할 대상이 없음 |
| 2026-11-14 | 버그 수정: 보스 리셋의 `RemoveLooseGameplayTag(Tag, -1)` → `SetLooseGameplayTagCount(Tag, 0)` | 음수 개수는 태그를 제거하지 않고 추가해, 리셋된 보스가 계속 '사망' 상태였음 (허수아비 부활 테스트에서 발견) |
| 2026-11-14 | (사용자 지시) 공격 사거리: 1·4번 = 박스 100 / 오프셋 120, 3번 = 박스 130 / 오프셋 165(약간 김), 2번 = 원거리. 기존 궁수 긴 히트박스 임시안 폐기 | 캐릭터별 차이 |
| 2026-11-14 | (사용자 지시) 2번(Sparrow) 화살 = `AMetaProjectile`(직선, 6000cm/s, 1.5초, GE_Damage = AttackPower × MotionValue). 조준점 = 락온 대상 또는 화면 중앙 트레이스(최대 40m). 화면 중앙 십자선(적 위면 붉은색) + 조준 지점 지면 마커(Paragon `P_RainArrowsTarget`), 궁수일 때 어깨 너머 카메라. 약/강공격 궤적 FX는 Paragon Sparrow 기본/궁극기 화살 | "타격점을 보여주어서 공격". `AMetaProjectile`은 7.3의 S5 클래스를 앞당겨 최소 구현 (S5에서 `Bounded.InMap` 확장) |
| 2026-11-14 | (사용자 지시, 2.1/10 변경) 강공격 = 좌클릭을 `HeavyChargeSeconds`(0.8초) 이상 누른 뒤 뗄 때(Completed) 발동, 짧게 누르면 뗄 때 약공격. `IA_LightAttack` 하나(트리거 없음)로 처리, Shift 코드·`IA_HeavyAttack` 매핑 제거(에셋은 유지). 튜토리얼 문구 변경 | "버튼을 떼는 시점 기준 풀차지". 약공격이 누를 때 → 뗄 때로 바뀜(짧은 클릭이면 체감 차이 미미) |
| 2026-11-14 | 강공격 모션을 기본공격 콤보와 다른 능력 애니메이션으로 교체: Greystone `Ability_R`, Sparrow `RMB_Fire`, FengMao `SlowAbility`, Terra `Q_swing_B` (스크린샷으로 차이 확인. FengMao `Ultimate`은 화면 밖으로 도약해 제외) | "강한 공격은 기본 공격과 다른 모션" |
| 2026-11-14 | 버그 수정: 내비메시가 없는 레벨에서 `MoveToActor`가 실패를 반환하지 않고 이동도 하지 않아 적이 제자리에서 걷는 모션만 했음 → 내비 데이터가 있을 때만 경로 이동, 없으면 직접 조향. 추가로 `AggroRange`(18m) 안에서만 추격, 밖이면 대기 | 사용자 보고. `Flow.EnemyChase` 테스트로 재현(4cm 이동) 후 수정 확인(12m → 공격 사거리) |
| 2026-11-14 | (사용자 지시) 궁수는 공격 중에만 조준점 쪽으로 몸을 돌림(`SetAttackFacing`, 이동 방향 회전 해제), 공격이 끝나면(`EndAbility`) 진행 방향 회전으로 복귀 | 활은 진행 방향인데 화살이 다른 곳으로 나가 어색했음 |
| 2026-11-14 | (사용자 지시) 강공격을 다시 `Shift`+좌클릭으로(차지 방식 폐기, 입력 파일은 이전 커밋으로 복원). 강공격 윈드업 0.8초, `HeavyMotionValue` 1.5→2.0(기본공격 2배; 3.1의 확정값 변경, `Combat.HitsToKill` 강공격 7→5회로 갱신). 강공격 전용 모션은 유지 | 사용자 요청 |
| 2026-11-14 | (사용자 지시) 구르기: 입력 없으면 바라보는 방향으로, 스태미나 0이면 불가(기존 규칙 유지), 쿨타임 `DodgeCooldown` 1초(구르기 시작 기준), 무적은 기존 `GE_IFrame` 0.3초 | `Sandbox.Roll` 게임 테스트로 검증 |
| 2026-11-14 | (사용자 지시) 피격 반응: 플레이어는 적 공격이 적중하면 영웅별 `HitReactAnim`(Paragon HitReact_Front/Fwd)을 몽타주 슬롯에 재생(구르기·사망 중 제외). 적은 모션 없이 접촉 지점(대상 캡슐 표면, 공격자 쪽)에 IB 타격 이펙트 — 약공격 `P_Impact_Enemy_IB`, 강공격 `P_Impact_Enemy_Strong`(1.6배). 궁수 화살은 기존 적중 FX | shortcut: 피격 방향 무관하게 정면 모션(데미지 이벤트에 공격자 정보 없음), 방향별 모션이 필요하면 이벤트에 Instigator 추가 |
