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
