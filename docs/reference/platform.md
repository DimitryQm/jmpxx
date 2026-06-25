<!-- SPDX-License-Identifier: MIT -->
# Platform abstraction

The platform abstraction is the library's single boundary for target-specific and
ABI-specific facts and constructs. Every platform-specific construct in the library
resides under it, in `jmpxx/platform/`, or in the separately fenced experimental
unwind area; a static scan enforces that, so the rest of the library is
platform-agnostic.

## Detection (`jmpxx/platform/detect.hpp`)
This header is the one place the library reads the compiler's predefined target
macros. It exposes the result as tokens the rest of the code asks for instead of
reading a raw `_MSC_VER`, `__x86_64__`, or `_WIN32` itself.

`JMPXX_COMPILER_GCC`, `JMPXX_COMPILER_CLANG`, and `JMPXX_COMPILER_MSVC` are each 1 on
exactly one compiler and 0 otherwise. Clang is reported as Clang even when it also
defines `__GNUC__` or `_MSC_VER`, because clang-cl behaves like Clang for intrinsics.
`JMPXX_OS_WINDOWS`, `JMPXX_OS_MACOS`, `JMPXX_OS_LINUX`, and `JMPXX_OS_BSD` report the
operating system, with `JMPXX_OS_HOSTED` their union and `JMPXX_OS_NONE` set on a
freestanding or bare-metal target. `JMPXX_ARCH_X86_64`, `JMPXX_ARCH_ARM64`,
`JMPXX_ARCH_ARM32`, `JMPXX_ARCH_RISCV`, and `JMPXX_ARCH_WASM` report the architecture.
`JMPXX_CPLUSPLUS` is the active standard version, normalized for MSVC.
`JMPXX_HAS_EXCEPTIONS` reports whether exceptions are enabled, and
`JMPXX_HAS_RTTI` reports whether RTTI is enabled. The header uses only the
preprocessor and is freestanding, so it is safe on the minimal core's include path.

## Queries (`jmpxx/platform.hpp`)
The unit's umbrella includes the detection and the fenced fail-fast and exposes small
queries for a tool or a program that reports which target it is on.
`jmpxx::platform::compiler_name()`, `os_name()`, and `arch_name()` return a short
stable identifier (`"gcc"`, `"linux"`, `"x86_64"`, and so on, or `"none"` and
`"other"` where there is no hosted OS or the target is outside the matrix).
`jmpxx::platform::is_hosted()` reports whether a hosted operating system is present.
Each is `constexpr` and `noexcept`.

## Fail-fast (`jmpxx/platform/trap.hpp`)
`jmpxx::platform::fail_fast(const char*)` terminates the program immediately and never
returns. It is the one platform-specific act the core performs, used for a checked
precondition violation on value or error extraction, so that access is a defined,
checked event rather than undefined behavior. It pulls no standard header and uses
only a compiler intrinsic, so it is freestanding.

## Trace capture (`jmpxx/platform/trace.hpp`)
An optional, off-by-default return-address capture for the diagnostic layer, behind
`JMPXX_STACKTRACE`. It captures addresses only without symbolizing, takes no lock, and
allocates nothing, and a platform without a fenced capturer returns an empty trace
rather than an unverified one. See [diagnostics.md](diagnostics.md).

## The fence

A construct is platform-specific or ABI-specific when it differs by operating system,
architecture, or ABI: an OS or architecture detection macro, a raw compiler-identity
macro, a platform or ABI system header, or inline assembly. Every such construct lives
under `jmpxx/platform/` or the fenced unwind area, and the `platform_fence` test scans
every other public header and fails on one found outside, with a known-bad fixture
proving the scan rejects a leak. A portable compiler intrinsic that behaves
identically on every target, such as a branch hint, is a compiler-portability concern
rather than a platform one and is not fenced.
