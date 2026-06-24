// SPDX-License-Identifier: MIT
// The minimal error representation. A bare integer code plus a domain tag that
// keeps codes from different sources from colliding. It is trivially copyable
// and register-sized, carries no out-of-band context, and is usable where there
// is no heap, no exceptions, and no RTTI. Richer representations that attach
// context are layered on later without changing this one or the call sites.
#ifndef JMPXX_CORE_ERROR_HPP
#define JMPXX_CORE_ERROR_HPP

#include "jmpxx/core/config.hpp"

namespace jmpxx {

struct error {
  int code = 0;
  int domain = 0;

  constexpr error() noexcept = default;
  constexpr explicit error(int c, int d = 0) noexcept : code(c), domain(d) {}

  [[nodiscard]] friend constexpr bool operator==(error, error) noexcept =
      default;
};

}  // namespace jmpxx

#endif  // JMPXX_CORE_ERROR_HPP
