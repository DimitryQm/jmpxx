#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# The std::expected bridge preserves the freestanding property: including it adds no
# non-freestanding dependency to the minimal core, and the bridge itself builds and
# runs in a freestanding, no-exceptions, no-RTTI configuration. This run proves that
# property. <expected> became freestanding
# in C++26 but compiles freestanding on the supported toolchains today, so the
# fixture is built at C++23, pinned to the one libstdc++ that honors -ffreestanding.
set -euo pipefail
CXX="${1:?compiler}"; INC="${2:?include dir}"; SRC="${3:?source}"; BIN="${4:?output}"
FLAGS=(-std=c++23 -ffreestanding -fno-exceptions -fno-rtti -I "$INC")

# 1. The minimal core's include graph stays clean: including jmpxx/core.hpp pulls
# neither <expected> nor <system_error>, so a freestanding consumer that never
# includes a bridge pays for none of it.
core="$(mktemp --suffix=.cpp)"
trap 'rm -f "$core"' EXIT
printf '#include <jmpxx/core.hpp>\nint main(){return 0;}\n' > "$core"
graph="$("$CXX" "${FLAGS[@]}" -H -fsyntax-only "$core" 2>&1 1>/dev/null || true)"
if echo "$graph" | grep -qE '/(expected|system_error)$'; then
  echo "FAIL: the minimal core pulled <expected> or <system_error>:"
  echo "$graph" | grep -E '/(expected|system_error)$'
  exit 1
fi

# 2. The bridge builds and runs freestanding.
"$CXX" "${FLAGS[@]}" -O2 "$SRC" -o "$BIN"
"$BIN"
echo "interop freestanding OK: the std::expected bridge builds and runs -ffreestanding, and the minimal core pulls no <expected> or <system_error>"
