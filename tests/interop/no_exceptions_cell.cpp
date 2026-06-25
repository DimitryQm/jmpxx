// SPDX-License-Identifier: MIT
// Interop under -fno-exceptions. The std::expected, std::error_code, and
// optional-like bridges all work with exceptions disabled, because none of them
// touches a throwing operation: the expected bridge reads through operator* and
// error() and never std::expected::value(). The exception bridge is absent here by
// construction, the configuration this cell exists to check, and the static
// assertion below checks that the header declared nothing rather than failing to build.
// The std::expected checks are guarded on JMPXX_INTEROP_HAS_EXPECTED so the cell
// also builds where <expected> is not provided.
#include "jmpxx/core.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/exception.hpp"
#include "jmpxx/interop/expected.hpp"

#include <cstdio>
#include <optional>
#include <system_error>

using namespace jmpxx;

static_assert(JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE == 0,
              "the exception bridge must be absent under -fno-exceptions");

int main() {
#if JMPXX_INTEROP_HAS_EXPECTED
  // std::expected bridge, no exceptions: round-trip a value and an error.
  result<int, error> rv = from_expected(to_expected(result<int, error>(7)));
  if (!rv.has_value() || rv.value() != 7) return 1;
  result<int, error> re =
      from_expected(to_expected(result<int, error>(fail(error(9, 3)))));
  if (re.has_value() || re.error() != error(9, 3)) return 2;
#endif

  // std::error_code bridge, no exceptions: expose and recover.
  std::error_code ec = to_error_code(error{5, 2});
  if (from_error_code(ec) != error(5, 2)) return 3;
  std::error_code foreign = std::make_error_code(std::errc::invalid_argument);
  result<int, std::error_code> carried = fail(foreign);
  if (carried.error() != foreign) return 4;

  // adapter, no exceptions.
  result<int, error> a =
      from_optional<error>(std::optional<int>(3), [] { return error(1); });
  if (!a.has_value() || a.value() != 3) return 5;

  std::printf("interop/no_exceptions_cell: bridges work, exception bridge absent "
              "(expected bridge %s)\n",
              JMPXX_INTEROP_HAS_EXPECTED ? "present" : "absent here");
  return 0;
}
