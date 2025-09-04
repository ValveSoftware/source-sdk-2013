#!/usr/bin/env bash
set -euo pipefail

# Trim the null sink log if it exceeds a size threshold.
# Defaults:
#   TARGET: game/mod_tf/nbdebuglogs/.null.nbdebug.txt (resolved from repo root)
#   MAX:    65536 bytes (64 KiB)
#
# Usage:
#   bash scripts/trim_null_log.sh [--max-bytes N] [--file PATH] [--quiet]

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT="${SCRIPT_DIR%/scripts}"
TARGET="$REPO_ROOT/game/mod_tf/nbdebuglogs/.null.nbdebug.txt"
MAX_BYTES=65536
QUIET=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --max-bytes)
      MAX_BYTES=${2:-65536}; shift 2 ;;
    --file)
      TARGET=${2:?}; shift 2 ;;
    --quiet)
      QUIET=true; shift ;;
    *)
      echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

mkdir -p -- "$(dirname -- "$TARGET")"
touch -- "$TARGET"

size=$(wc -c <"$TARGET" 2>/dev/null || echo 0)
if [[ "$size" =~ ^[0-9]+$ ]] && (( size > MAX_BYTES )); then
  : > "$TARGET"  # truncate to zero
  $QUIET || echo "Trimmed $TARGET (was ${size}B, threshold ${MAX_BYTES}B)"
else
  $QUIET || echo "OK: $TARGET size=${size}B (<= ${MAX_BYTES}B)"
fi

