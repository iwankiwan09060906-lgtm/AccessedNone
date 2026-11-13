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

## Stage 2~6

TODO
