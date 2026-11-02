# Shared helpers for the lint scripts. Source this file, don't execute it.

# Prints "label|value" rows, aligning values after the longest label.
print_rows() {
  local width=0 row label
  for row in "$@"; do label=${row%%|*}; ((${#label} > width)) && width=${#label}; done
  for row in "$@"; do printf '%-*s %s\n' "$width" "${row%%|*}" "${row#*|}"; done
}

# Prints a failure report to stderr and exits 1. Args: headline, reason, [body].
die() {
  {
    echo "$1. See output for more details."
    echo ""
    echo "$2"
    echo
    [[ -z ${3:-} ]] || echo "$3"
    echo "---"
  } >&2
  exit 1
}
