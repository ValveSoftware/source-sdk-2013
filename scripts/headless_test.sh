#!/usr/bin/env bash
set -euo pipefail

# Headless NextBot test runner
# - Launches TF mod in headless/text mode
# - Clears nbheadlessdebuglogs before run for clean captures
# - Executes cfg/headless.cfg which routes logs into nbheadlessdebuglogs
#
# Usage:
#   scripts/headless_test.sh [map] [bots] [mode] [timeout]
#     map     : map name (default: pl_upward)
#     bots    : bot quota (default: 8)
#     mode    : nb_debug mode: behavior|path|loco|lookat|vision|off (default: behavior)
#     timeout : seconds before auto-exit (default: 150)
#
# Env overrides:
#   HEADLESS_REPAIR_FRAC   Health fraction threshold for repair-first (default: 0)
#   HEADLESS_EXTRA_ARGS    Extra +cvars to append to the command line

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
GAME_DIR="$REPO_ROOT/game"
MOD_DIR="$GAME_DIR/mod_tf"
LAUNCH_EXE="$GAME_DIR/mod_tf_linux64"

MAP_NAME=${1:-pl_upward}
BOTS=${2:-8}
MODE=${3:-behavior}
RUN_TIMEOUT=${4:-150}
REPAIR_FRAC=${HEADLESS_REPAIR_FRAC:-0}
EXTRA_ARGS=${HEADLESS_EXTRA_ARGS:-}

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

echo "[HEADLESS] Launching headless: map=$MAP_NAME bots=$BOTS mode=$MODE timeout=${RUN_TIMEOUT}s repair_frac=$REPAIR_FRAC" >&2

# Run headless with minimal args; autoexec will run first, then we exec headless.cfg to override logging
set +e
timeout "${RUN_TIMEOUT}s" "$LAUNCH_EXE" \
  -textmode -noshaderapi -nosound -novid -console -insecure \
  +sv_cheats 1 +developer 1 +con_timestamp 1 \
  +exec headless.cfg \
  +$NB_SET_ALIAS \
  +map "$MAP_NAME" \
  +tf_bot_quota_mode fill +tf_bot_keep_class 1 +tf_bot_join_after_player 0 \
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

exit 0
