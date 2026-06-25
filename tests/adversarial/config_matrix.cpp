// SPDX-License-Identifier: MIT
// One executable used by the configuration cross-product script. The script builds
// this file under the meaningful switch combinations; the runtime checks confirm the
// selected configuration actually exercises the corresponding surface instead of only
// compiling it.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/expected.hpp"

#include <cstdio>
#include <optional>
#include <system_error>

using namespace jmpxx;

namespace {

result<int, rich_error> leaf(int x) {
  if (x < 0) return fail(rich_error(19, 6));
  return x;
}
result<int, rich_error> step(int x) {
  JMPXX_TRY(v, leaf(x));
  return v + 1;
}
result<int, rich_error> chain(int x) {
  JMPXX_TRY(v, step(x));
  return v + 1;
}

}  // namespace

int main() {
  static_assert(JMPXX_HARDENING_MODE >= JMPXX_HARDENING_NONE);
  static_assert(JMPXX_HARDENING_MODE <= JMPXX_HARDENING_EXTENSIVE);
  static_assert(JMPXX_HARDENED == (JMPXX_HARDENING_MODE >= JMPXX_HARDENING_FAST));

  result<int, error> r = 7;
  if (!r.has_value() || r.value() != 7) return 1;
  r = fail(error(3, 4));
  if (r.has_value() || r.error() != error(3, 4)) return 2;

  erased_error e(5);
  if (e.value() != 5 || !e.in_domain(generic_domain())) return 3;
  erased_error folded(-1, -2);
  if (!folded.in_domain(generic_domain())) return 13;

  std::error_code ec = to_error_code(error(8, 2));
  if (!is_jmpxx(ec) || from_error_code(ec) != error(8, 2)) return 4;

  std::optional<int> maybe(42);
  if (from_optional<error>(maybe, [] { return error(1); }).value() != 42)
    return 5;

#if JMPXX_INTEROP_HAS_EXPECTED
  if (from_expected(to_expected(result<int, error>(9))).value() != 9) return 6;
#endif

#if JMPXX_DIAGNOSTICS_ENABLED
  landing root;
  result<int, rich_error> bad = chain(-1);
  if (bad.has_value()) return 7;
  diagnostic::context c = diagnostic::inspect(bad.error());
  if (!c.available || c.hop_count != 2) return 8;
#if JMPXX_STACKTRACE
  if (platform::trace_available() && !c.trace_captured) return 9;
#endif
#endif

  std::printf(
      "config-matrix: hardening=%d hardened=%d diagnostics=%d stacktrace=%d "
      "exceptions=%d rtti=%d expected=%d\n",
      JMPXX_HARDENING_MODE, JMPXX_HARDENED, JMPXX_DIAGNOSTICS_ENABLED,
      JMPXX_STACKTRACE, JMPXX_HAS_EXCEPTIONS, JMPXX_HAS_RTTI,
      JMPXX_INTEROP_HAS_EXPECTED);
  return 0;
}
