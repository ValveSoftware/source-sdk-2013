#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"

readonly TYPES="feat fix build chore ci docs style refactor perf test"
readonly SCOPES_FILE="$(dirname "$0")/scopes.txt"
readonly SUBJECT=$1
readonly SUBJECT_RE='^([^(!:]*)(\(([^)]*)\))?!?:(.*)$'

SCOPES=$(sed 's/^[^:]*://' "$SCOPES_FILE" | xargs)
SCOPE_ROWS=()
while IFS= read -r line; do SCOPE_ROWS+=("${line%%:*}:|${line#*: }"); done < "$SCOPES_FILE"

commit_rows() {
  print_rows "Expected:|type(scope)[!]: description" "Got:|$SUBJECT" "$@"
  echo
}

scope_rows() {
  commit_rows
  echo "Valid scopes:"
  print_rows "${SCOPE_ROWS[@]}" | sed 's/^/  /'
}

type_rows() {
  commit_rows
  echo "Valid types:"
  echo "  $TYPES"
}

[[ $SUBJECT =~ $SUBJECT_RE ]] ||
  die "Commit is missing a type and colon" "The commit message must start with a type followed by a colon." "$(commit_rows)"
type=${BASH_REMATCH[1]} scope=${BASH_REMATCH[3]} desc=${BASH_REMATCH[4]}

[[ " $TYPES " == *" $type "* ]] ||
  die "Commit has invalid type"            "The commit type '$type' is not one of the valid types."         "$(type_rows)"
[[ -n $scope ]] ||
  die "Commit is missing a scope"          "The commit is missing a scope in parentheses."                  "$(scope_rows)"
[[ " $SCOPES " == *" $scope "* ]] ||
  die "Commit has an invalid scope"        "The commit scope '$scope' is not one of the valid scopes."      "$(scope_rows)"
[[ -n ${desc// /} ]] ||
  die "Commit is missing a description"    "The commit is missing a description after the colon."           "$(commit_rows)"
