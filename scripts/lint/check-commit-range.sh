#!/usr/bin/env bash
set -euo pipefail

status=0
shas=$(git rev-list --no-merges "$1")
for sha in $shas; do
  "$(dirname "$0")/check-commit-msg.sh" "$(git log -1 --format=%s "$sha")" || status=1
done
exit $status
