// SPDX-License-Identifier: MIT
// Static tier: properties checked by translation. Building this file is the test.
#include "jmpxx/core.hpp"

#include <string>
#include <type_traits>

using namespace jmpxx;

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
