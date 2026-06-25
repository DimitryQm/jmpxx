#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Platform-fence scan. Every platform-specific or ABI-specific construct must live
# inside the platform abstraction unit (include/jmpxx/platform/) or the separately
# fenced unwind arm (include/jmpxx/unwind/). This scan reads every public header
# outside those two directories and fails if it finds a raw compiler-identity macro,
# an operating-system or architecture detection macro, a platform or ABI system
# header, or inline assembly. The rest of the library asks for target facts through
# the JMPXX_COMPILER_*, JMPXX_OS_*, and JMPXX_ARCH_* tokens that platform/detect.hpp
# defines, so a raw target macro outside the fence is the leak this scan catches.
#
# Portable compiler intrinsics that behave identically on every target (for example
# __builtin_addressof and __builtin_expect) are a compiler-portability concern, not
# a platform or ABI concern, so they are not fenced and not flagged.
#
# Usage: platform_fence_check.sh <root-include-dir>
set -euo pipefail
ROOT="${1:?root include dir}"

# Compiler identity, operating system, architecture, platform/ABI system headers,
# and inline assembly. Word boundaries keep, for example, __arm__ from matching a
# longer identifier and _WIN32 from matching inside a comment word.
patterns=(
  '\b_MSC_VER\b' '\b_MSVC_LANG\b' '\b__GNUC__\b' '\b__GNUG__\b' '\b__clang__\b'
  '\b_WIN32\b' '\b_WIN64\b' '\b__linux__\b' '\b__APPLE__\b' '\b__MACH__\b'
  '\b__FreeBSD__\b' '\b__NetBSD__\b' '\b__OpenBSD__\b' '\b__DragonFly__\b' '\b__unix__\b'
  '\b__x86_64__\b' '\b_M_X64\b' '\b__i386__\b' '\b_M_IX86\b'
  '\b__aarch64__\b' '\b_M_ARM64\b' '\b__arm__\b' '\b_M_ARM\b'
  '\b__riscv\b' '\b__wasm__\b' '\b__wasm32__\b' '\b__wasm64__\b'
  '<windows\.h>' '<unwind\.h>' '<execinfo\.h>' '<cxxabi\.h>' '<intrin\.h>'
  '<setjmp\.h>' '<csetjmp>' '<immintrin\.h>' '<emmintrin\.h>'
  '__asm__' '\basm[[:space:]]+volatile\b'
)
joined="$(IFS='|'; echo "${patterns[*]}")"

violations=0
while IFS= read -r -d '' f; do
  case "$f" in
    */platform/*|*/unwind/*) continue ;;
  esac
  if hits="$(grep -nE "$joined" "$f")"; then
    echo "FAIL: platform-specific construct outside the fence in $f"
    echo "$hits" | sed 's/^/    /'
    violations=$((violations + 1))
  fi
done < <(find "$ROOT" -name '*.hpp' -print0)

if [ "$violations" -ne 0 ]; then
  echo "platform-fence scan: $violations file(s) leak platform code outside platform/ or unwind/"
  exit 1
fi
echo "platform-fence OK: no platform-specific construct outside platform/ or unwind/"
