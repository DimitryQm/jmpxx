// SPDX-License-Identifier: MIT
// The ABI layout descriptor.
//
// A header-only library ships no compiled object, so its ABI is the observable
// memory layout of its public types: the size, the alignment, the field offsets,
// and the trivial-copyability a consumer's register passing and any persisted or
// shared representation depend on. This program prints that layout for the types
// the v0.1.0 ABI promise freezes, one type per line in a fixed order, so the
// jmpxx-verify abi-layout gate can diff the emitted descriptor against a committed
// golden and fail the build on an unversioned change. docs/reference/abi.md states
// which types are frozen and why.
//
// It is compiled in the configuration the niche ships, release with exceptions and
// RTTI disabled, because that is the layout that ships: under NDEBUG the rich
// policy's diagnostic handle is compiled out and rich_error is its frozen form,
// exactly the minimal error. The static assertions below pin the
// architecture-independent facts at translation time on every target the static
// tier builds on, and the printed descriptor pins the full layout, pointer width
// included, against the golden.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"

#include <cstddef>
#include <cstdio>
#include <type_traits>

using namespace jmpxx;

// Architecture-independent layout facts, checked at translation time so a layout
// regression fails the build on every cell the static tier runs on, not only where
// the descriptor gate runs. These mirror the frozen-type promise in
// docs/reference/abi.md. The field offsets hold because the public error types are
// standard-layout; the sizes of the pointer-bearing types are asserted in terms of
// sizeof(void*) so the one assertion holds on both 32-bit and 64-bit targets.
static_assert(sizeof(error) == 8 && alignof(error) == 4);
static_assert(offsetof(error, code) == 0 && offsetof(error, domain) == 4);
static_assert(std::is_standard_layout_v<error> && std::is_trivially_copyable_v<error>);

#if !JMPXX_DIAGNOSTICS_ENABLED
// The frozen rich representation is the release representation: identical to the
// minimal error in size, alignment, and field offsets. The debug representation
// carries an extra handle word and is a build-configuration artifact, not part of
// the ABI promise, so this descriptor is only meaningful under NDEBUG.
static_assert(sizeof(rich_error) == sizeof(error) &&
              alignof(rich_error) == alignof(error));
static_assert(offsetof(rich_error, code) == 0 && offsetof(rich_error, domain) == 4);
static_assert(std::is_standard_layout_v<rich_error> &&
              std::is_trivially_copyable_v<rich_error>);
#endif

static_assert(sizeof(erased_error) == sizeof(int) + sizeof(void*) ||
              sizeof(erased_error) == 2 * sizeof(void*));
static_assert(std::is_trivially_copyable_v<erased_error>);
static_assert(std::is_trivially_copyable_v<result<int, error>>);
static_assert(std::is_trivially_copyable_v<result<void, error>>);
static_assert(std::is_trivially_copyable_v<result<int, erased_error>>);
static_assert(std::is_trivially_copyable_v<result<int, rich_error>>);

namespace {

// One scalar-field type's line: name, size, alignment, the two trait bits, and the
// public field offsets. Used for the error types whose code and domain a consumer
// reads directly.
template <class T>
void fields_line(const char* name, std::size_t code_off, std::size_t domain_off) {
  std::printf("%s sizeof=%zu alignof=%zu trivially_copyable=%d standard_layout=%d "
              "code=%zu domain=%zu\n",
              name, sizeof(T), alignof(T),
              std::is_trivially_copyable_v<T> ? 1 : 0,
              std::is_standard_layout_v<T> ? 1 : 0, code_off, domain_off);
}

// One opaque type's line: name, size, alignment, and the trait bits, for a type
// whose internals are private and whose frozen contract is its size, alignment, and
// trivial copyability rather than a named field offset.
template <class T>
void opaque_line(const char* name) {
  std::printf("%s sizeof=%zu alignof=%zu trivially_copyable=%d standard_layout=%d\n",
              name, sizeof(T), alignof(T),
              std::is_trivially_copyable_v<T> ? 1 : 0,
              std::is_standard_layout_v<T> ? 1 : 0);
}

}  // namespace

int main() {
  std::printf("# jmpxx ABI layout descriptor (frozen public types, ship/release "
              "configuration)\n");
  std::printf("pointer_bits=%zu\n", sizeof(void*) * 8);
  fields_line<error>("error", offsetof(error, code), offsetof(error, domain));
#if !JMPXX_DIAGNOSTICS_ENABLED
  fields_line<rich_error>("rich_error", offsetof(rich_error, code),
                          offsetof(rich_error, domain));
#endif
  opaque_line<erased_error>("erased_error");
  opaque_line<result<int, error>>("result<int,error>");
  opaque_line<result<void, error>>("result<void,error>");
  opaque_line<result<int, erased_error>>("result<int,erased_error>");
  opaque_line<result<int, rich_error>>("result<int,rich_error>");
  return 0;
}
