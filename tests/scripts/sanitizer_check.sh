#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build and run the given sources under AddressSanitizer and
# UndefinedBehaviorSanitizer, failing on any finding.
set -euo pipefail
CXX="${1:?compiler}"; INC="${2:?include dir}"; OUT="${3:?output dir}"; shift 3
for src in "$@"; do
  bin="$OUT/$(basename "$src").san"
  "$CXX" -std=c++20 -O1 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -fno-sanitize-recover=all -I "$INC" "$src" -o "$bin"
  "$bin"
done
echo "sanitizer (address, undefined) clean"
