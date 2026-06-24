#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# The minimal core include must not transitively pull the hosted policy or
# diagnostic headers. Compile a translation unit that includes only jmpxx/core.hpp
# and scan the full include graph for any rich, type-erased, diagnostic, or
# source-location header. Their presence would mean an embedded consumer pays for
# facilities the minimal policy never uses.
set -euo pipefail
CXX="${1:?compiler}"; INC="${2:?include dir}"
SRC="$(mktemp --suffix=.cpp)"
trap 'rm -f "$SRC"' EXIT
printf '#include <jmpxx/core.hpp>\nint main(){return 0;}\n' > "$SRC"

graph="$("$CXX" -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -I "$INC" \
  -H -fsyntax-only "$SRC" 2>&1 1>/dev/null || true)"

forbidden='diagnostics\.hpp|erased\.hpp|diagnostic/store\.hpp|platform/trace\.hpp|/source_location$|/source_location"|<source_location>|/execinfo'
if echo "$graph" | grep -nE "$forbidden"; then
  echo "FAIL: the minimal core pulled a hosted policy or diagnostic header"
  exit 1
fi
echo "include-boundary OK: jmpxx/core.hpp pulls no rich, erased, diagnostic, or source_location header"
