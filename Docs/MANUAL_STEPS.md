# MANUAL_STEPS

사용자가 에디터에서 직접 해야 하는 작업입니다. 완료하면 "했어"라고 알려주세요.

## #1 에디터 재시작 (MCP 연결 + Live Coding 완전 해제) — ✅ 완료 (2026-11-14)

1. 언리얼 에디터에서 파일 → 종료 (저장할 것이 있으면 저장)
2. `D:\UnrealEngine\AccessedNone\AccessedNone.uproject` 더블클릭으로 다시 열기
3. Windows 방화벽 팝업이 뜨면 **개인 네트워크 허용** 클릭
4. 편집 → 프로젝트 세팅 → 플러그인 → Python → **Enable Remote Execution** 체크 확인, **Multicast Bind Address** = `0.0.0.0` 확인
5. 편집 → 에디터 개인설정 → 일반 → Live Coding → "Live Coding 활성화"가 **꺼져 있는지** 확인
