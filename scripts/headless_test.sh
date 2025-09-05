#!/usr/bin/env bash
set -euo pipefail

# Headless NextBot test runner
# - Launches TF mod in headless/text mode
# - Clears nbheadlessdebuglogs before run for clean captures
# - Executes cfg/headless.cfg which routes logs into nbheadlessdebuglogs
#
# Usage:
#   scripts/headless_test.sh [map] [bots] [mode] [timeout] [killmode]
#     map     : map name (default: pl_upward)
#     bots    : bot quota (default: 8)
#     mode    : nb_debug mode: behavior|path|loco|lookat|vision|off (default: behavior)
#     timeout : seconds before auto-exit (default: 150)
#     killmode: how to end the session on timeout (default: int)
#               - int  : send SIGINT on timeout, then SIGKILL after grace
#               - term : send SIGTERM on timeout, then SIGKILL after grace
#               - kill : send SIGKILL immediately on timeout
#
# Env overrides:
#   HEADLESS_REPAIR_FRAC     Health fraction threshold for repair-first (default: 0)
#   HEADLESS_EXTRA_ARGS      Extra +cvars to append to the command line
#   HEADLESS_KILL_GRACE_SEC  Seconds to wait before SIGKILL (default: 5)
#   HEADLESS_ASSERT_REPAIR   If set to 1, exit non-zero when no repair markers found

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
GAME_DIR="$REPO_ROOT/game"
MOD_DIR="$GAME_DIR/mod_tf"
LAUNCH_EXE="$GAME_DIR/mod_tf_linux64"

MAP_NAME=${1:-pl_upward}
BOTS=${2:-8}
MODE=${3:-behavior}
RUN_TIMEOUT=${4:-150}
KILL_MODE=${5:-int}
REPAIR_FRAC=${HEADLESS_REPAIR_FRAC:-0}
EXTRA_ARGS=${HEADLESS_EXTRA_ARGS:-}
KILL_GRACE=${HEADLESS_KILL_GRACE_SEC:-5}

NB_DIR="$MOD_DIR/nbheadlessdebuglogs"

mkdir -p "$NB_DIR" "$NB_DIR/bot-visuals-debug"

echo "[HEADLESS] Clearing old logs under: $NB_DIR" >&2
# Remove only .txt logs to keep any docs like legend.md
if compgen -G "$NB_DIR/**/*.txt" > /dev/null || compgen -G "$NB_DIR/*.txt" > /dev/null; then
  find "$NB_DIR" -type f -name "*.txt" -print -delete || true
fi

if [[ ! -x "$LAUNCH_EXE" ]]; then
  echo "[HEADLESS] ERROR: launcher not found: $LAUNCH_EXE" >&2
  exit 1
fi

# Map nb_debug mode to headless cfg deferred set aliases (executed before +map)
case "$MODE" in
  behavior) NB_SET_ALIAS=hd_set_pending_behavior ;;
  path)     NB_SET_ALIAS=hd_set_pending_path ;;
  loco|locomotion) NB_SET_ALIAS=hd_set_pending_loco ;;
  lookat)   NB_SET_ALIAS=hd_set_pending_lookat ;;
  vision)   NB_SET_ALIAS=hd_set_pending_vision ;;
  off)      NB_SET_ALIAS=hd_set_pending_off ;;
  *) echo "[HEADLESS] Invalid mode '$MODE' — using 'behavior'" >&2; NB_SET_ALIAS=hd_set_pending_behavior ;;
esac

echo "[HEADLESS] Launching headless: map=$MAP_NAME bots=$BOTS mode=$MODE timeout=${RUN_TIMEOUT}s killmode=$KILL_MODE repair_frac=$REPAIR_FRAC" >&2

# Compose timeout options
TIMEOUT_OPTS=( )
case "$KILL_MODE" in
  int)  TIMEOUT_OPTS=( -s INT -k "${KILL_GRACE}s" ) ;;
  term) TIMEOUT_OPTS=( -s TERM -k "${KILL_GRACE}s" ) ;;
  kill) TIMEOUT_OPTS=( -s KILL ) ;;
  *)    echo "[HEADLESS] Unknown killmode '$KILL_MODE' — using 'int'" >&2; TIMEOUT_OPTS=( -s INT -k "${KILL_GRACE}s" ) ;;
esac

# Run headless with minimal args; autoexec will run first, then we exec headless.cfg to override logging
set +e
timeout "${TIMEOUT_OPTS[@]}" "${RUN_TIMEOUT}s" "$LAUNCH_EXE" \
  -textmode -noshaderapi -nosound -novid -console -insecure \
  +sv_cheats 1 +developer 1 +con_timestamp 1 \
  +exec headless.cfg \
  +$NB_SET_ALIAS \
  +map "$MAP_NAME" \
  +tf_bot_quota_mode fill +tf_bot_keep_class_after_death 1 +tf_bot_join_after_player 0 \
  +tf_bot_force_class engineer +tf_bot_quota "$BOTS" +mp_waitingforplayers_cancel 1 \
  +tf_bot_engineer_seams_repair_health_frac "$REPAIR_FRAC" \
  $EXTRA_ARGS
STATUS=$?
set -e

echo "[HEADLESS] Game exited with status $STATUS" >&2

echo "[HEADLESS] Captured logs in: $NB_DIR" >&2
ls -la "$NB_DIR" || true

for f in "$NB_DIR/nextbot.debug.txt" "$NB_DIR/nb.behavior.debug.txt" "$NB_DIR/nb.path.debug.txt" "$NB_DIR/nb.locomotion.debug.txt" "$NB_DIR/bot-visuals-debug/nb.lookat.debug.txt" "$NB_DIR/bot-visuals-debug/nb.vision.debug.txt"; do
  if [[ -f "$f" ]]; then
    echo "--- $f (tail) ---"
    tail -n 60 "$f" || true
  fi
done

# Simple verification that guarded code paths produced output
REPAIR_COUNT=0
if command -v rg >/dev/null 2>&1; then
  REPAIR_COUNT=$(rg -n "\\[TF-ENG seam\\] Repair priority" -S "$NB_DIR"/*.txt "$NB_DIR"/bot-visuals-debug/*.txt 2>/dev/null | wc -l | tr -d ' ')
else
  REPAIR_COUNT=$(grep -R "\[TF-ENG seam\] Repair priority" "$NB_DIR" 2>/dev/null | wc -l | tr -d ' ')
fi

if [[ "${REPAIR_COUNT}" =~ ^[0-9]+$ ]] && (( REPAIR_COUNT > 0 )); then
  echo "[HEADLESS] Check: Repair priority markers found: ${REPAIR_COUNT} (PASS)"
else
  echo "[HEADLESS] Check: Repair priority markers found: 0 (WARN)"
  if [[ "${HEADLESS_ASSERT_REPAIR:-0}" == "1" ]]; then
    echo "[HEADLESS] ASSERT: Expected repair markers but none found — failing" >&2
    exit 2
  fi
fi

exit 0
