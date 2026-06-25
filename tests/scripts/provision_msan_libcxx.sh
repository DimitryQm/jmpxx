#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build an MSan-instrumented libc++/libc++abi/libunwind install for the memory
# sanitizer gate. The install lives outside the repository build products and is a
# dev-only verifier dependency.
set -euo pipefail
ROOT="${1:?output root}"
LLVM_TAG="${2:-llvmorg-18.1.8}"
CXX_BIN="${CXX:-clang++}"
CC_BIN="${CC:-clang}"
LLVM="$ROOT/llvm-project"
INSTALL="$ROOT/install"
if [[ -d "$INSTALL/include/c++/v1" ]] &&
   find "$INSTALL" \( -name 'libc++.a' -o -name 'libc++.so' \) -print -quit | grep -q . &&
   find "$INSTALL" \( -name 'libc++abi.a' -o -name 'libc++abi.so' \) -print -quit | grep -q . &&
   find "$INSTALL" \( -name 'libunwind.a' -o -name 'libunwind.so' \) -print -quit | grep -q .; then
  printf '%s\n' "$INSTALL"
  exit 0
fi
mkdir -p "$ROOT"
if [[ ! -d "$LLVM/.git" ]]; then
  git clone --depth=1 --branch "$LLVM_TAG" https://github.com/llvm/llvm-project.git "$LLVM"
fi
cmake -S "$LLVM/runtimes" -B "$ROOT/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$CC_BIN" \
  -DCMAKE_CXX_COMPILER="$CXX_BIN" \
  -DLLVM_ENABLE_RUNTIMES="libunwind;libcxxabi;libcxx" \
  -DLLVM_USE_SANITIZER=MemoryWithOrigins \
  -DLIBUNWIND_ENABLE_SHARED=OFF \
  -DLIBCXX_ENABLE_SHARED=OFF \
  -DLIBCXXABI_ENABLE_SHARED=OFF \
  -DLIBCXX_ENABLE_STATIC=ON \
  -DLIBCXXABI_ENABLE_STATIC=ON \
  -DCMAKE_INSTALL_PREFIX="$INSTALL"
cmake --build "$ROOT/build" --target cxx cxxabi unwind cxx_experimental -j "${JMPXX_MSAN_BUILD_JOBS:-2}"
cmake --install "$ROOT/build" --prefix "$INSTALL"
printf '%s\n' "$INSTALL"
