#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Opt-in scan for the experimental unwind arm. The arm is reached only by including
# jmpxx/unwind.hpp; no portable engine header may pull it in, so a consumer who never
# opts in never compiles it. This reads every engine header outside the arm's own
# umbrella and its fenced unwind/ directory and fails if one includes the arm umbrella or
# any unwind/ header. Pure text scan, so it runs anywhere bash does.
#
# Usage: unwind_opt_in_check.sh <root-include-dir>
set -euo pipefail
ROOT="${1:?root include dir}"

violations=0
while IFS= read -r -d '' f; do
  case "$f" in
    */unwind/*|*/unwind.hpp) continue ;;
  esac
  if hits="$(grep -nE 'include[[:space:]]*[<\"]jmpxx/unwind(\.hpp|/)' "$f")"; then
    echo "FAIL: a default-path header reaches the opt-in unwind arm in $f"
    echo "$hits" | sed 's/^/    /'
    violations=$((violations + 1))
  fi
done < <(find "$ROOT" -name '*.hpp' -print0)

if [ "$violations" -ne 0 ]; then
  echo "unwind opt-in scan: $violations header(s) reach the arm off the opt-in path"
  exit 1
fi
echo "unwind opt-in OK: no default-path engine header includes the unwind arm"
