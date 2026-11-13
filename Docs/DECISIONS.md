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
| 2026-11-14 | `PythonScriptPlugin`, `EditorScriptingUtilities`를 S1-T01 전에 `.uproject`에 선반영 | MCP 연결의 전제 조건 |
