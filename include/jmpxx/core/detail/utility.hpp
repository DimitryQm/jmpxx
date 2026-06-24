// SPDX-License-Identifier: MIT
// Small utilities the freestanding core needs without reaching for <utility> or
// <memory>, whose full freestanding status varies by standard version. move,
// forward, and addressof are hand-rolled here; the trait predicates that need
// compiler magic come from <type_traits>, which is freestanding and pulls in no
// hosted machinery.
#ifndef JMPXX_CORE_DETAIL_UTILITY_HPP
#define JMPXX_CORE_DETAIL_UTILITY_HPP

#include "jmpxx/core/config.hpp"

#include <type_traits>

namespace jmpxx {
namespace detail {

template <class T>
[[nodiscard]] JMPXX_ALWAYS_INLINE constexpr std::remove_reference_t<T>&& move(
    T&& t) noexcept {
  return static_cast<std::remove_reference_t<T>&&>(t);
}

template <class T>
[[nodiscard]] JMPXX_ALWAYS_INLINE constexpr T&& forward(
    std::remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}

template <class T>
[[nodiscard]] JMPXX_ALWAYS_INLINE constexpr T&& forward(
    std::remove_reference_t<T>&& t) noexcept {
  static_assert(!std::is_lvalue_reference_v<T>,
                "forward must not turn an rvalue into an lvalue");
  return static_cast<T&&>(t);
}

// __builtin_addressof yields the true address even for a type with an
// overloaded operator&, and is available on every supported compiler.
template <class T>
[[nodiscard]] JMPXX_ALWAYS_INLINE constexpr T* addressof(T& t) noexcept {
  return __builtin_addressof(t);
}
template <class T>
const T* addressof(const T&&) = delete;

}  // namespace detail
}  // namespace jmpxx

#endif  // JMPXX_CORE_DETAIL_UTILITY_HPP
