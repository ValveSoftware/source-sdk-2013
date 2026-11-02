#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"

readonly HEAD_BRANCH=$1 BASE_BRANCH=$2 BODY=$3
readonly ISSUE_RE='\b(close|closes|closed|fix|fixes|fixed|resolve|resolves|resolved) #[0-9]+'

[[ $HEAD_BRANCH == development && $BASE_BRANCH == master ]] && exit 0
grep -Eiq "$ISSUE_RE" <<< "$BODY" ||
  die "PR is missing an issue link" "The PR description needs a closing reference to an issue, e.g. 'Closes #123'."
