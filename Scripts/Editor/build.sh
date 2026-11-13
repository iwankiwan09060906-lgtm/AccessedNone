#!/bin/sh
# Closes the editor gracefully (save via MCP first if possible), builds, then relaunches the editor.
# Usage: Scripts/Editor/build.sh [--no-launch]
UE="/c/Program Files/Epic Games/UE_5.8/Engine"
PROJ="D:/UnrealEngine/AccessedNone/AccessedNone.uproject"

# Graceful close (no /F): if anything is unsaved the editor shows its save prompt and we abort after 60s.
taskkill //IM UnrealEditor.exe >/dev/null 2>&1
for i in $(seq 1 60); do
  tasklist //FI "IMAGENAME eq UnrealEditor.exe" | grep -q UnrealEditor.exe || break
  sleep 1
done
if tasklist //FI "IMAGENAME eq UnrealEditor.exe" | grep -q UnrealEditor.exe; then
  echo "Editor still running after 60s; aborting build." >&2
  exit 2
fi
# LiveCodingConsole can outlive the editor and still block UBT
taskkill //F //IM LiveCodingConsole.exe >/dev/null 2>&1

"$UE/Build/BatchFiles/Build.bat" AccessedNoneEditor Win64 Development -Project="$PROJ" -WaitMutex > "$TEMP/an_build.log" 2>&1
rc=$?
grep -E "error|warning C|warning:|Result:" "$TEMP/an_build.log" | grep -v "^Log file"
echo "warnings(project): $(grep -E 'AccessedNone.*warning' "$TEMP/an_build.log" | wc -l)"

[ "$1" = "--no-launch" ] || ("$UE/Binaries/Win64/UnrealEditor.exe" "$PROJ" >/dev/null 2>&1 &)
exit $rc
