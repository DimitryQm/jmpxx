// SPDX-License-Identifier: MIT
// Monadic-ops tier: and_then, or_else, transform, and transform_error behave as
// std::expected's do (P2505), across value and error states, a void value, chaining,
// move-through, an error-type change, and constant evaluation. Compiled with
// exceptions and RTTI off, the strict configuration the library targets.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"  // rich_error policy
#include "jmpxx/erased.hpp"       // type-erased policy

#include <cstdio>
#include <string>

using namespace jmpxx;
using R = result<int, error>;

// Constant evaluation: the value-side and error-side ops fold at compile time.
constexpr R dbl(int x) { return R(x * 2); }
static_assert(R(5).and_then(dbl).value() == 10);
static_assert(R(5).transform([](int x) { return x + 1; }).value() == 6);
static_assert(R(fail(error(9))).and_then(dbl).error().code == 9);
static_assert(R(fail(error(9))).or_else([](error) { return R(42); }).value() == 42);
static_assert(R(fail(error(2))).transform_error([](error e) {
                return error(e.code + 50);
              }).error().code == 52);

int main() {
  int fails = 0;
  auto check = [&](bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++fails; }
  };

  // and_then: chain on the value, short-circuit on the failure (f not called).
  check(R(5).and_then(dbl).value() == 10, "and_then maps value");
  bool called = false;
  check(R(fail(error(7))).and_then([&](int x) { called = true; return R(x); })
            .error().code == 7 && !called,
        "and_then short-circuits on error");

  // transform: map a plain value; propagate the failure; collapse to void.
  check(R(5).transform([](int x) { return x + 10; }).value() == 15, "transform maps value");
  check(R(fail(error(3))).transform([](int x) { return x + 10; }).error().code == 3,
        "transform propagates error");
  check(R(5).transform([](int) {}).has_value(), "transform to void");

  // or_else: recover from the failure; pass the value through.
  check(R(fail(error(1))).or_else([](error e) { return R(e.code + 100); }).value() == 101,
        "or_else recovers from error");
  check(R(5).or_else([](error) { return R(-1); }).value() == 5, "or_else passes value");

  // transform_error: map the error; pass the value through.
  auto te = R(fail(error(2))).transform_error([](error e) { return error(e.code + 50); });
  check(!te.has_value() && te.error().code == 52, "transform_error maps error");
  check(R(7).transform_error([](error) { return error(999); }).value() == 7,
        "transform_error passes value");

  // A result<void, E> value-side callable takes no argument.
  check(result<void, error>().and_then([] { return R(7); }).value() == 7, "void and_then");
  check(result<void, error>().transform([] { return 3; }).value() == 3, "void transform");
  check(result<void, error>(fail(error(8)))
            .or_else([](error) { return result<void, error>(); })
            .has_value(),
        "void or_else recovers");

  // The std::expected-style pipeline: transform, then and_then, then transform.
  auto chain = R(2)
                   .transform([](int x) { return x * 5; })
                   .and_then([](int x) { return x > 5 ? R(x) : R(fail(error(404))); })
                   .transform([](int x) { return x + 1; });
  check(chain.value() == 11, "transform/and_then/transform chain");

  // The value moves through rather than copying, for a non-trivial payload.
  auto moved = result<std::string, error>(std::string("hi"))
                   .transform([](std::string s) { return s + "!"; });
  check(moved.value() == "hi!", "transform moves a string payload");

  // and_then may change the value type while keeping the error type.
  auto changed = R(fail(error(5))).and_then([](int) { return result<long, error>(1L); });
  check(!changed.has_value() && changed.error().code == 5,
        "and_then keeps the error across a value-type change");

  // The ops are generic over the policy: the rich and erased errors work too.
  check(result<int, rich_error>(5).transform([](int x) { return x + 1; }).value() == 6,
        "transform under the rich policy");
  check(result<int, erased_error>(fail(erased_error(4)))
            .or_else([](erased_error) { return result<int, erased_error>(9); })
            .value() == 9,
        "or_else under the erased policy");

  std::printf("monadic: %s (%d failures)\n", fails == 0 ? "ALL PASS" : "FAILURES", fails);
  return fails == 0 ? 0 : 1;
}
