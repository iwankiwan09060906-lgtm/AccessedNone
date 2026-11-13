# MCP 능력 점검

- 점검일: 2026-11-14
- MCP 서버: `@runreal/unreal-mcp` (stdio, npx) — Python Remote Execution(UDP 멀티캐스트 239.0.0.1:6766) 경유
- 엔진: UE 5.8.4 (CL 58902052), Python 3.11.8

## 연결

| 항목 | 결과 |
|---|---|
| 엔진/프로젝트 경로 설정 | ✅ |
| 에디터 원격 노드 연결 | ✅ (`DefaultEngine.ini` 바인드 주소 `0.0.0.0` + 에디터 재시작 후). 에디터 시작 직후 첫 호출은 `Remote node is not available`로 실패할 수 있음 → 재시도하면 연결됨 |

## 능력 (2026-11-14 실제 호출로 검증)

| 점검 항목 | 방법 | 결과 |
|---|---|---|
| 액터 스폰 / 삭제 / 트랜스폼 | `editor_create_object` / `editor_delete_object` | ✅ 검증 |
| 레벨 생성 / 저장 | Python `LevelEditorSubsystem.new_level` / `save_current_level` | API 존재 확인, 실사용은 S1-T08에서 검증 |
| Blueprint 생성 / 부모 지정 / CDO 기본값 | Python `BlueprintFactory` + `get_default_object().set_editor_property` | ✅ 검증 |
| 컴포넌트 추가 | – | C++ 생성자에서 부착 (정책) |
| DataAsset 생성 / 프로퍼티 | Python `DataAssetFactory` | ✅ 검증 |
| UMG 위젯 생성 | Python `WidgetBlueprintFactory` | ✅ 검증 |
| UMG 위젯 트리 편집 | Python에 `widget_tree` 노출 안 됨 | ❌ → C++ `BindWidget` / `NativeConstruct`로 구성 |
| Input Action / Mapping Context (키 매핑 포함) | Python `InputAction_Factory`, `InputMappingContext_Factory`, `map_key` | ✅ 검증 |
| 머티리얼 생성 / 노드 그래프 (파라미터 연결) | Python `MaterialEditingLibrary` | ✅ 검증 |
| Unreal Python 실행 | `editor_run_python` | ✅ 검증 |
| 콘솔 명령 | `editor_console_command` (출력 반환 없음) | ✅ 검증 |
| PIE 시작·종료 | Python `editor_play_simulate` / `editor_request_end_play` | API 존재 확인, 미검증 |
| 로그 읽기 | `Saved/Logs/AccessedNone.log` 파일 직접 읽기 | ✅ |
| 에셋 조회 | `editor_project_info` | ✅ 검증 |

## 운용 방침

- 에셋 생성은 `Scripts/Editor/*.py`로 작성해 `editor_run_python`으로 실행 (재실행 가능하게).
- 에디터가 꺼져 있으면 `UnrealEditor-Cmd.exe -run=pythonscript -script=...`로 대체.
- 위젯 트리는 C++에서 구성, WBP는 부모 클래스 지정만.
