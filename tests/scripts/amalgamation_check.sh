#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Gate the single-header amalgamations. First regenerate them from the modular headers
# and diff against the committed copies, so a stale single header fails. Then build a
# consumer against each: the core header under -ffreestanding -fno-exceptions -fno-rtti,
# to prove it stays freestanding-pure, and the full header hosted, to prove it is
# self-contained across the hosted surface. Mirrors the other compile-and-run checks.
set -euo pipefail
ROOT="${1:?repo root}"
CXX="${2:?c++ compiler}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Drift: regenerate into a scratch tree and compare.
python3 "$ROOT/packaging/amalgamate.py" --include-dir "$ROOT/include" \
  --output "$TMP/jmpxx-core.hpp" --guard JMPXX_CORE_AMALGAMATED_HPP \
  --title "jmpxx core, single-header amalgamation (freestanding minimal core)" \
  --root jmpxx/core.hpp >/dev/null
python3 "$ROOT/packaging/amalgamate.py" --include-dir "$ROOT/include" \
  --output "$TMP/jmpxx.hpp" --guard JMPXX_AMALGAMATED_HPP \
  --title "jmpxx, single-header amalgamation (full hosted surface)" \
  --root jmpxx/core.hpp --root jmpxx/diagnostics.hpp --root jmpxx/erased.hpp \
  --root jmpxx/reflect.hpp --root jmpxx/platform.hpp --root jmpxx/interop.hpp >/dev/null
for f in jmpxx-core.hpp jmpxx.hpp; do
  if ! diff -q "$ROOT/single_include/$f" "$TMP/$f" >/dev/null; then
    echo "FAIL: single_include/$f is stale; run packaging/amalgamate.sh and commit"
    diff "$ROOT/single_include/$f" "$TMP/$f" | head -40
    exit 1
  fi
done
echo "amalgamation OK: single headers match the modular source"

# Core consumer, freestanding. The core single header must pull in nothing outside the
# freestanding subset, which a freestanding compile proves.
cat > "$TMP/core_consumer.cpp" <<'EOF'
#include <jmpxx-core.hpp>
using namespace jmpxx;
static result<int> leaf(int x){ if (x < 0) return fail(error(42)); return x * 2; }
static result<int> w(int x){ JMPXX_TRY(v, leaf(x)); return v + 1; }
int main(){ auto a = w(5); auto b = w(-1);
  return (a && a.value() == 11 && !b && b.error().code == 42) ? 0 : 1; }
EOF
"$CXX" -std=c++20 -O2 -ffreestanding -fno-exceptions -fno-rtti \
  -I"$ROOT/single_include" "$TMP/core_consumer.cpp" -o "$TMP/core_consumer"
"$TMP/core_consumer"
echo "amalgamation OK: jmpxx-core.hpp builds and runs freestanding"

# Full consumer, hosted.
"$CXX" -std=c++23 -O2 -I"$ROOT/single_include" \
  "$ROOT/packaging/consumers/amalgamation/main.cpp" -o "$TMP/full_consumer"
"$TMP/full_consumer"
echo "amalgamation OK: jmpxx.hpp builds and runs a hosted consumer"
