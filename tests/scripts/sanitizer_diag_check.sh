#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build a diagnostic-layer source with a given sanitizer and the layer on, then run
# it. Used for the thread sanitizer over concurrent rich failures and the address
# sanitizer over landing-scope exit, checking the out-of-band store for races and
# leaks rather than asserting those properties in prose.
set -euo pipefail
CXX="${1:?compiler}"; INC="${2:?include dir}"; OUT="${3:?output dir}"
SAN="${4:?sanitizer}"; SRC="${5:?source}"
bin="$OUT/$(basename "$SRC").${SAN//,/_}.san"
"$CXX" -std=c++20 -O1 -g -fsanitize="$SAN" -fno-omit-frame-pointer \
  -fno-sanitize-recover=all -DJMPXX_DIAGNOSTICS_ENABLED=1 -pthread \
  -I "$INC" "$SRC" -o "$bin"
"$bin"
echo "sanitizer ($SAN) clean for $(basename "$SRC")"
