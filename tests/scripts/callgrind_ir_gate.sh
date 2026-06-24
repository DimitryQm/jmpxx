#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# The deterministic perf gate. callgrind counts the instructions a region executes from a
# simulated CPU, so the count is identical on every machine and every run, unlike a wall-clock
# time. The gate runs the jmpxx and the hand-written kernels under callgrind over an identical
# call loop and bounds the jmpxx instruction count to a small multiple of the hand-written one,
# so a jmpxx chain that executes more instructions than the branch it replaces fails the build.
# It has teeth in the same run: the deliberately slowed kernel must exceed the bound, so a gate
# that would pass a real regression fails here first.
#
# An instruction count is not a cycle count. callgrind models no pipeline, cache, or branch
# prediction, so this gate proves instruction parity on the measured region, not the wall-clock
# sad-path cost, which the distribution benchmark measures.
set -euo pipefail
BENCH="${1:?jmpxx-bench path}"
VG="${2:-valgrind}"
ITERS="${3:-200000}"
BOUND_PCT="${4:-110}"  # jmpxx instructions must be <= this percent of hand-written

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

ir_for() {
  local mech="$1" fail="${2:-0}"
  local cg="$work/cg.$mech.$fail"
  "$VG" --tool=callgrind --collect-atstart=no --callgrind-out-file="$cg" \
    "$BENCH" callgrind --mech "$mech" --fail "$fail" --iters "$ITERS" >/dev/null 2>&1
  # With collection deferred and toggled around the region, the summary line holds the region's
  # instruction total (a single Ir event by default).
  awk '/^summary:/{print $2; exit}' "$cg"
}

jx="$(ir_for jmpxx 0)"
hw="$(ir_for handwritten 0)"
slow="$(ir_for jmpxx_slow 0)"

if [ -z "$jx" ] || [ -z "$hw" ] || [ -z "$slow" ]; then
  echo "FAIL: could not read an instruction total from callgrind output"
  exit 1
fi

echo "instructions over $ITERS happy-path calls:"
echo "  jmpxx        = $jx  (per call ~ $((jx / ITERS)))"
echo "  handwritten  = $hw  (per call ~ $((hw / ITERS)))"
echo "  jmpxx_slow   = $slow (per call ~ $((slow / ITERS)))"

# Gate: jmpxx within BOUND_PCT% of hand-written.
if [ "$((100 * jx))" -gt "$((BOUND_PCT * hw))" ]; then
  echo "FAIL: jmpxx instructions $jx exceed ${BOUND_PCT}% of hand-written $hw"
  exit 1
fi
# Teeth: the slowed kernel must trip the same bound, or the gate is vacuous.
if [ "$((100 * slow))" -le "$((BOUND_PCT * hw))" ]; then
  echo "FAIL(teeth): the slowed kernel did not exceed the bound; the gate proves nothing"
  exit 1
fi

echo "callgrind-ir OK: jmpxx within ${BOUND_PCT}% of hand-written; the slowed kernel trips the gate"
