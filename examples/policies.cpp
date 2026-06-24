// SPDX-License-Identifier: MIT
// One transport, three error representations. The same function body, templated on the
// error type, compiles and behaves identically under the minimal error, the rich error,
// and the type-erased error. A program selects a policy by the error type it names,
// usually through one alias, with no change at the call sites.
#include <jmpxx/core.hpp>
#include <jmpxx/diagnostics.hpp>
#include <jmpxx/erased.hpp>

#include <cstdio>

using jmpxx::erased_error;
using jmpxx::error;
using jmpxx::fail;
using jmpxx::result;
using jmpxx::rich_error;

// Identical source for every policy: only E changes.
template <class E>
result<int, E> checked_div(int a, int b) {
  if (b == 0) return fail(E(1, 2));  // code 1, domain tag 2
  return a / b;
}
template <class E>
result<int, E> run(int a, int b) {
  JMPXX_TRY(q, checked_div<E>(a, b));
  return q + 1;
}

int main() {
  // Minimal policy: a bare code and domain, freestanding and smallest.
  result<int, error> m = run<error>(10, 0);
  std::printf("minimal: ok=%d code=%d\n", m.has_value(), m.error().code);

  // Rich policy: the same code and domain in band, plus out-of-band context in debug.
  // The success behavior is identical to the minimal policy.
  result<int, rich_error> r = run<rich_error>(10, 2);
  std::printf("rich:    ok=%d value=%d\n", r.has_value(), r.value());

  // Type-erased policy: a boundary reads the value and a rendered message without
  // knowing the component's error enum.
  result<int, erased_error> e = run<erased_error>(10, 0);
  std::printf("erased:  ok=%d value=%d message=%s\n", e.has_value(), e.error().value(),
              e.error().message());
  return 0;
}
