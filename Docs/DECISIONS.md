# DECISIONS

| 날짜 | 결정 | 이유 |
|---|---|---|
| 2026-11-14 | 마스터 명세는 별도 파일 없이 루트 `CLAUDE.md`를 기준 문서로 사용 | 이미 전문이 `CLAUDE.md`에 있고 매 세션 자동 로드됨 |
| 2026-11-14 | Python Remote Execution 멀티캐스트 바인드 주소를 `0.0.0.0`으로 설정 (`DefaultEngine.ini`) | 기본값(127.0.0.1)에서 MCP가 원격 노드를 찾지 못함 |
| 2026-11-14 | C++ 빌드는 `Scripts/Editor/build.sh`로: MCP로 저장·종료 → 빌드 → 에디터 재실행 (사용자 승인) | 에디터가 켜져 있으면 LiveCodingConsole이 남아 UBT가 빌드를 거부함 |
| 2026-11-14 | `Build.cs`에 `PublicIncludePaths.Add(ModuleDirectory)` | `Core/…`, `Meta/…` 하위 폴더 경로로 include |
| 2026-11-14 | `PythonScriptPlugin`, `EditorScriptingUtilities`를 S1-T01 전에 `.uproject`에 선반영 | MCP 연결의 전제 조건 |
