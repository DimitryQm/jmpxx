// SPDX-License-Identifier: MIT
// Behavioral round-trip tier for the interop bridges. A value or failure crossing
// each documented bridge and returning is equal to the original in every field the
// bridge promises to preserve. Built with exceptions enabled and at C++23 so the
// exception bridge is present and std::expected is available where the standard
// library provides it. The std::expected checks are guarded on
// JMPXX_INTEROP_HAS_EXPECTED so the fixture also builds where <expected> is absent,
// for example Clang 18 against a standard library that does not yet expose it; the
// other bridges are exercised there regardless. Each check returns a distinct
// nonzero code so a failure names itself.
#include "jmpxx/core.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/exception.hpp"
#include "jmpxx/interop/expected.hpp"

#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace jmpxx;

namespace {

enum { missing = 1, bad = 2 };

// An external component that signals failure by throwing, for the exception bridge.
int risky(int x) {
  if (x < 0) throw std::runtime_error("boom");
  return x * 2;
}

}  // namespace

int main() {
#if JMPXX_INTEROP_HAS_EXPECTED
  // 1. std::expected round-trip, value and error, lossless including a non-trivial
  // payload and the move path.
  {
    result<int, error> rv = 7;
    if (from_expected(to_expected(rv)).value() != 7) return 1;
    result<int, error> re = fail(error(9, 3));
    if (from_expected(to_expected(re)).error() != error(9, 3)) return 2;
    result<std::string, error> rs =
        from_expected(std::expected<std::string, error>("hello"));
    if (!rs.has_value() || rs.value() != "hello") return 3;
    result<std::string, error> rse = from_expected(
        std::expected<std::string, error>(std::unexpected(error(11, 4))));
    if (rse.has_value() || rse.error() != error(11, 4)) return 4;
    // void
    result<void, error> vok = from_expected(to_expected(result<void, error>{}));
    if (!vok.has_value()) return 5;
    result<void, error> verr =
        from_expected(to_expected(result<void, error>(fail(error(99)))));
    if (verr.has_value() || verr.error().code != 99) return 6;
  }

  // 2. std::expected constexpr round-trip: the bridge is usable in constant
  // evaluation (building these is the check).
  {
    constexpr result<int, error> cv = 42;
    static_assert(from_expected(to_expected(cv)).value() == 42);
    constexpr result<int, error> ce = fail(error(5, 1));
    static_assert(from_expected(to_expected(ce)).error() == error(5, 1));
  }
#endif

  // 3. std::error_code: expose a jmpxx error and recover it losslessly (code and
  // domain), with stable category identity per domain.
  {
    error e{5, 2};
    std::error_code ec = to_error_code(e);
    if (ec.value() != 5 || !is_jmpxx(ec)) return 7;
    if (from_error_code(ec) != e) return 8;
    std::error_code adl = make_error_code(error{7, 9});
    if (from_error_code(adl) != error(7, 9)) return 9;
    if (&error_category(2) != &error_category(2)) return 10;
    if (&error_category(2) == &error_category(3)) return 11;
  }

  // 4. std::error_code: a foreign code is carried verbatim through result<T,
  // std::error_code> with no loss of value or category, and recognized as foreign.
  {
    std::error_code foreign = std::make_error_code(std::errc::invalid_argument);
    result<int, std::error_code> r = fail(foreign);
    if (r.has_value() || r.error() != foreign || is_jmpxx(foreign)) return 12;
#if JMPXX_INTEROP_HAS_EXPECTED
    // a std::expected<T, std::error_code>-returning library bridges in verbatim
    std::expected<int, std::error_code> lib = std::unexpected(foreign);
    result<int, std::error_code> br = from_expected(lib);
    if (br.has_value() || br.error() != foreign) return 13;
#endif
  }

  // 5. exception bridge: a jmpxx failure crosses out as a throw and back, losslessly.
  {
    result<int, error> bad_in = fail(error(42, 7));
    result<int, error> back = catch_into_result<error>(
        [&] { return value_or_throw(bad_in); }, [] { return error(-1); });
    if (back.has_value() || back.error() != error(42, 7)) return 14;
    // success path
    if (value_or_throw(result<int, error>(21)) != 21) return 15;
    // a foreign exception is mapped by the caller's policy
    result<int, error> mapped = catch_into_result<error>(
        [&] { return risky(-1); }, [] { return error(99, 1); });
    if (mapped.has_value() || mapped.error().code != 99) return 16;
    result<int, error> fok =
        catch_into_result<error>([&] { return risky(5); }, [] { return error(0); });
    if (!fok.has_value() || fok.value() != 10) return 17;
    // void
    result<void, error> vr =
        catch_into_result<error>([&] { (void)risky(1); }, [] { return error(0); });
    if (!vr.has_value()) return 18;
  }

  // 6. exception bridge carries the rich and type-erased policies too, not only the
  // minimal error, so the carrier is policy-agnostic.
  {
    result<int, erased_error> re = fail(erased_error(3, generic_domain()));
    result<int, erased_error> back = catch_into_result<erased_error>(
        [&] { return value_or_throw(re); },
        [] { return erased_error(0, generic_domain()); });
    if (back.has_value() || back.error().value() != 3) return 19;
  }

  // 7. the from_optional / from_condition adapters collapse the optional-like and
  // boolean-plus-value boundaries into one call.
  {
    result<std::string, error> present = from_optional<error>(
        std::optional<std::string>("localhost"), [] { return error(missing); });
    if (!present.has_value() || present.value() != "localhost") return 20;
    result<std::string, error> empty = from_optional<error>(
        std::optional<std::string>(std::nullopt), [] { return error(missing); });
    if (empty.has_value() || empty.error().code != missing) return 21;
    result<int, error> cond_ok =
        from_condition<error>(true, [] { return 42; }, [] { return error(bad); });
    if (!cond_ok.has_value() || cond_ok.value() != 42) return 22;
    result<int, error> cond_no =
        from_condition<error>(false, [] { return 0; }, [] { return error(bad); });
    if (cond_no.has_value() || cond_no.error().code != bad) return 23;
  }

  std::printf("interop/roundtrip: bridges round-trip losslessly (expected bridge %s)\n",
              JMPXX_INTEROP_HAS_EXPECTED ? "exercised" : "absent here");
  return 0;
}
