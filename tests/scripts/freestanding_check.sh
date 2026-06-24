#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Scan the minimal core's include graph for any hosted standard header, then
# build and run it in a freestanding, no-exceptions, no-RTTI configuration.
set -euo pipefail
CXX="${1:?compiler}"; INC="${2:?include dir}"; SRC="${3:?source}"; BIN="${4:?output}"
FLAGS=(-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -I "$INC")

tree="$("$CXX" "${FLAGS[@]}" -H -fsyntax-only "$SRC" 2>&1 1>/dev/null || true)"
if echo "$tree" | grep -qE '/(string|vector|iostream|ostream|istream|sstream|memory|optional|variant|expected|stdexcept|functional|map|unordered_map|set|deque|list|mutex|thread|regex|fstream)$'; then
  echo "FAIL: the minimal core pulled a hosted standard header:"
  echo "$tree" | grep -E '/(string|vector|iostream|ostream|istream|sstream|memory|optional|variant|expected|stdexcept|functional|map|unordered_map|set|deque|list|mutex|thread|regex|fstream)$'
  exit 1
fi

"$CXX" "${FLAGS[@]}" -O2 "$SRC" -o "$BIN"
"$BIN"
echo "freestanding build+run OK; include graph free of hosted headers"
