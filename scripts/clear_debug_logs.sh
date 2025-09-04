#!/usr/bin/env bash
set -euo pipefail

# Clear NextBot debug logs (and optionally build logs) from the repo.
#
# Usage:
#   bash scripts/clear_debug_logs.sh [--build] [--all] [--dry-run]
#
# Flags:
#   --build    Also clear build logs under buildlog/ (*.out)
#   --all      Clear both NextBot logs and build logs
#   --dry-run  Show what would be removed without deleting
#
# Examples:
#   bash scripts/clear_debug_logs.sh --dry-run
#   bash scripts/clear_debug_logs.sh --all

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT="${SCRIPT_DIR%/scripts}"

NB_DIR="$REPO_ROOT/game/mod_tf/nbdebuglogs"
NB_FILE_LEGACY="$REPO_ROOT/game/mod_tf/nextbot.debug.txt"
BUILDLOG_DIR="$REPO_ROOT/buildlog"

DRY_RUN=false
CLEAR_BUILD=false

for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=true ;;
    --build) CLEAR_BUILD=true ;;
    --all) CLEAR_BUILD=true ;;
    *) echo "Unknown option: $arg" >&2; exit 2 ;;
  esac
done

remove_files() {
  local pattern_desc="$1"; shift
  local -a files=("$@")
  local count=0
  if ((${#files[@]})); then
    if $DRY_RUN; then
      printf "[DRY-RUN] Would remove %d %s:\n" "${#files[@]}" "$pattern_desc"
      printf '  %s\n' "${files[@]}"
    else
      for f in "${files[@]}"; do
        rm -f -- "$f"
        ((count++)) || true
      done
      printf "Removed %d %s\n" "$count" "$pattern_desc"
    fi
  fi
}

main() {
  # NextBot logs (keep docs/placeholders)
  if [[ -d "$NB_DIR" ]]; then
    mapfile -t nb_logs < <(find "$NB_DIR" -type f -name '*.debug.txt' -print | sort)
  else
    nb_logs=()
  fi

  # Legacy root logfile
  if [[ -f "$NB_FILE_LEGACY" ]]; then
    nb_logs+=("$NB_FILE_LEGACY")
  fi

  if ((${#nb_logs[@]}==0)); then
    echo "No NextBot debug logs found under nbdebuglogs/"
  else
    remove_files "NextBot debug log(s)" "${nb_logs[@]}"
  fi

  # Build logs (optional)
  if $CLEAR_BUILD; then
    if [[ -d "$BUILDLOG_DIR" ]]; then
      mapfile -t build_logs < <(find "$BUILDLOG_DIR" -maxdepth 1 -type f -name '*.out' -print | sort)
      if ((${#build_logs[@]})); then
        remove_files "build log(s)" "${build_logs[@]}"
      else
        echo "No build logs found under buildlog/"
      fi
    else
      echo "No buildlog/ directory present"
    fi
  fi
}

main "$@"

