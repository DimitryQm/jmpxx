// SPDX-License-Identifier: MIT
// The platform abstraction: the engine's single boundary for target-specific and
// ABI-specific facts and constructs. Detection lives in platform/detect.hpp, the
// fenced fail-fast in platform/trap.hpp, and the optional trace capture in
// platform/trace.hpp; this umbrella exposes the unit's caller-facing surface and
// the small queries a tool or a consumer uses to report which target it is on.
// Every platform-specific construct in the engine resides under this unit (or the
// later, separately fenced unwind arm), which the platform-fence scan enforces.
//
// It pulls in only the freestanding detection and trap headers, so including it
// adds no hosted dependency. The optional trace capture is not pulled in here
// because it is heavier and off by default; a consumer that wants it includes
// platform/trace.hpp directly.
#ifndef JMPXX_PLATFORM_HPP
#define JMPXX_PLATFORM_HPP

#include "jmpxx/platform/detect.hpp"
#include "jmpxx/platform/trap.hpp"

namespace jmpxx {
namespace platform {

// The compiler that built this translation unit, as a short stable identifier.
// One of "gcc", "clang", "msvc", or "unknown".
[[nodiscard]] constexpr const char* compiler_name() noexcept {
#if JMPXX_COMPILER_CLANG
  return "clang";
#elif JMPXX_COMPILER_GCC
  return "gcc";
#elif JMPXX_COMPILER_MSVC
  return "msvc";
#else
  return "unknown";
#endif
}

// The operating system this translation unit targets. "none" is a freestanding
// or bare-metal target with no hosted OS.
[[nodiscard]] constexpr const char* os_name() noexcept {
#if JMPXX_OS_WINDOWS
  return "windows";
#elif JMPXX_OS_MACOS
  return "macos";
#elif JMPXX_OS_LINUX
  return "linux";
#elif JMPXX_OS_BSD
  return "bsd";
#else
  return "none";
#endif
}

// The target architecture. "other" is a target outside the supported matrix,
// reported honestly rather than guessed.
[[nodiscard]] constexpr const char* arch_name() noexcept {
#if JMPXX_ARCH_X86_64
  return "x86_64";
#elif JMPXX_ARCH_ARM64
  return "arm64";
#elif JMPXX_ARCH_ARM32
  return "arm32";
#elif JMPXX_ARCH_RISCV
  return "riscv";
#elif JMPXX_ARCH_WASM
  return "wasm";
#else
  return "other";
#endif
}

// Whether a hosted operating system is present. False on a freestanding or
// bare-metal target, where the minimal core is the supported surface.
[[nodiscard]] constexpr bool is_hosted() noexcept { return JMPXX_OS_HOSTED != 0; }

}  // namespace platform
}  // namespace jmpxx

#endif  // JMPXX_PLATFORM_HPP
