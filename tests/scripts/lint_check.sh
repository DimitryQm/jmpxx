#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Drive the companion check set over a fixture and assert its exact finding set. In expect-findings
# mode every check must flag its violation and the documented total must match. In expect-clean
# mode the file, including its tricky non-violations, must produce no finding. A false verdict on
# clean code fails the check, because flagging correct code is the failure it must avoid.
set -euo pipefail
LINT="${1:?jmpxx-lint path}"
ROOT="${2:?repo root}"
MODE="${3:?expect-findings|expect-clean}"
SRC="${4:?fixture}"

out="$("$LINT" "$SRC" -- -std=c++20 -fno-exceptions -fno-rtti -I"$ROOT/include" 2>/dev/null)"
printf '%s\n' "$out"
count="$(printf '%s\n' "$out" | sed -n 's/^jmpxx-lint: \([0-9]*\) finding.*/\1/p')"
: "${count:=missing}"

if [ "$MODE" = expect-clean ]; then
  if [ "$count" = 0 ]; then
    echo "lint OK: no false verdict on clean code"
    exit 0
  fi
  echo "FAIL: $count false verdict(s) on clean code"
  exit 1
fi

# expect-findings: each check must have flagged its violation, and the total must be the four
# the violations fixture contains (two discards, one unchecked access, one manual propagation).
for kind in jmpxx-discarded-result jmpxx-unchecked-access jmpxx-manual-propagation; do
  if ! printf '%s\n' "$out" | grep -q "$kind:"; then
    echo "FAIL: check $kind did not flag its violation"
    exit 1
  fi
done
if [ "$count" != 4 ]; then
  echo "FAIL: expected 4 findings in the violations fixture, got $count"
  exit 1
fi
echo "lint OK: every check flagged its violation, four findings, no more"
