#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"

readonly BRANCH=$1
readonly BRANCH_RE='^issue[0-9]+/[a-z0-9]+(-[a-z0-9]+)*$'

case $BRANCH in master | development) exit 0 ;; esac
[[ $BRANCH =~ $BRANCH_RE ]] ||
  die "Branch name is invalid" "Branch '$BRANCH' must be issue<nr>/title-in-kebab-case, e.g. issue123/add-branch-lint."
