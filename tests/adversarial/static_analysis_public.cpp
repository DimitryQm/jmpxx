// SPDX-License-Identifier: MIT
// Public-header static-analysis translation unit for the portable hardening pass.
// The analyzer sees the headers a consumer ships without reaching the experimental
// unwind arm, whose ABI-bound matrix is covered separately.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/expected.hpp"
#include "jmpxx/platform.hpp"
#include "jmpxx/reflect.hpp"

#include <optional>
#include <system_error>

int main() {
  jmpxx::result<int, jmpxx::error> r = 5;
  jmpxx::result<int, jmpxx::error> e = jmpxx::fail(jmpxx::error(1, 2));
  if (!r || e) return 1;
  std::optional<int> maybe(3);
  auto adapted = jmpxx::from_optional<jmpxx::error>(
      maybe, [] { return jmpxx::error(9); });
  std::error_code ec = jmpxx::to_error_code(e.error());
  jmpxx::erased_error erased(7);
  return adapted.value() + jmpxx::from_error_code(ec).code + erased.value();
}
