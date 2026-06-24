#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Regenerate both single-header amalgamations into single_include/. Run from the repo
# root. Continuous integration runs this and then `git diff --exit-code`, so a stale
# committed single header fails the build.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
inc="$root/include"
out="$root/single_include"
py="$root/packaging/amalgamate.py"

# The freestanding minimal core: the same surface as <jmpxx/core.hpp>, pulling in
# nothing outside the freestanding subset.
python3 "$py" --include-dir "$inc" --output "$out/jmpxx-core.hpp" \
  --guard JMPXX_CORE_AMALGAMATED_HPP \
  --title "jmpxx core, single-header amalgamation (freestanding minimal core)" \
  --root jmpxx/core.hpp

# The full hosted surface. The core is the first root, so the shared base headers are
# inlined unconditionally before any conditional include of them. The unwind arm is
# excluded by design; it stays the opt-in modular include.
python3 "$py" --include-dir "$inc" --output "$out/jmpxx.hpp" \
  --guard JMPXX_AMALGAMATED_HPP \
  --title "jmpxx, single-header amalgamation (full hosted surface)" \
  --root jmpxx/core.hpp \
  --root jmpxx/diagnostics.hpp \
  --root jmpxx/erased.hpp \
  --root jmpxx/reflect.hpp \
  --root jmpxx/platform.hpp \
  --root jmpxx/interop.hpp
