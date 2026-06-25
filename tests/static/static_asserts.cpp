// SPDX-License-Identifier: MIT
// Static tier: properties checked by translation. Building this file is the test.
#include "jmpxx/core.hpp"

#include <cstddef>
#include <string>
#include <type_traits>

using namespace jmpxx;

// The version surface. The combined integer follows major*10000+minor*100+patch and
// the string derives from the same components, so a consumer may gate on either and
// the two cannot drift.
static_assert(JMPXX_VERSION == JMPXX_VERSION_MAJOR * 10000 +
                                   JMPXX_VERSION_MINOR * 100 + JMPXX_VERSION_PATCH);
static_assert(sizeof(JMPXX_VERSION_STRING) >= sizeof("0.0.0"));
static_assert(JMPXX_HARDENING_NONE == 0);
static_assert(JMPXX_HARDENING_FAST == 1);
static_assert(JMPXX_HARDENING_EXTENSIVE == 2);
static_assert(JMPXX_HARDENING_MODE >= JMPXX_HARDENING_NONE);
static_assert(JMPXX_HARDENING_MODE <= JMPXX_HARDENING_EXTENSIVE);
static_assert(JMPXX_HARDENED == (JMPXX_HARDENING_MODE >= JMPXX_HARDENING_FAST));

// ABI freeze, checked on every cell the static tier builds on. The minimal error's
// size, alignment, and public field offsets are part of the layout the v0.1.0 ABI
// promise holds fixed within the major version (docs/reference/abi.md); a reorder or
// an inserted field breaks these and fails the build everywhere, not only where the
// layout-descriptor gate runs. offsetof is well defined here because error is
// standard-layout.
static_assert(sizeof(error) == 8 && alignof(error) == 4);
static_assert(offsetof(error, code) == 0 && offsetof(error, domain) == 4);
static_assert(std::is_standard_layout_v<error>);

// Trivial copyability is preserved when, and only when, the contents are trivial.
static_assert(std::is_trivially_copyable_v<result<int, int>>);
static_assert(std::is_trivially_copyable_v<result<int, error>>);
static_assert(std::is_trivially_copyable_v<result<double, error>>);
static_assert(std::is_trivially_copyable_v<result<void, error>>);
static_assert(std::is_trivially_destructible_v<result<int, error>>);
static_assert(!std::is_trivially_copyable_v<result<std::string, error>>);
static_assert(!std::is_trivially_copyable_v<result<int, std::string>>);

// The transport adds nothing beyond the discriminant and natural padding.
static_assert(sizeof(result<int, int>) == 8);
static_assert(sizeof(result<char, char>) == 2);

// A nothrow-movable payload yields a nothrow-movable transport.
static_assert(std::is_nothrow_move_constructible_v<result<int, error>>);

// Member type aliases.
static_assert(std::is_same_v<result<int, error>::value_type, int>);
static_assert(std::is_same_v<result<void, error>::value_type, void>);
static_assert(std::is_same_v<result<int, error>::error_type, error>);

// A throwing-move payload removes assignment rather than admitting a valueless
// state, while construction and move construction remain available.
struct throwing_move {
  int v;
  explicit throwing_move(int x) : v(x) {}
  throwing_move(const throwing_move&) = default;
  throwing_move(throwing_move&&) noexcept(false) {}
  throwing_move& operator=(const throwing_move&) = default;
  throwing_move& operator=(throwing_move&&) = default;
};
// Move construction stays available (it propagates the throw without leaving a
// valueless object); move assignment is removed because it could not be made
// never-valueless. Copy operations stay available: throwing_move's copy is
// trivial, so the transport copies by memcpy, which cannot become valueless.
static_assert(std::is_move_constructible_v<result<throwing_move, error>>);
static_assert(!std::is_move_assignable_v<result<throwing_move, error>>);
static_assert(std::is_copy_constructible_v<result<throwing_move, error>>);

int main() { return 0; }
