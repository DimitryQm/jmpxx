// SPDX-License-Identifier: MIT
// Known-bad fixture for the release-diff gate's self-test. jx_rich_leaky stands in
// for a diagnostic facility that was not fenced out of release: it captures a
// source location and folds it into the result even under NDEBUG. That capture adds
// instructions and materializes the function-name string into read-only data, so
// the rich function's release code diverges from the minimal function's. If the
// gate passes this fixture, the dual-personality zero-cost proof has no teeth.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"

#include <source_location>

using namespace jmpxx;

template <class E>
static result<int, E> producer(int x) {
  if (x == 0) return fail(E(9));
  return x + 1;
}

extern "C" result<int, error> jx_min_op(int x) {
  JMPXX_TRY(v, producer<error>(x));
  return v * 10;
}

// A volatile sink the leaked location escapes through, so the optimizer cannot fold
// the function-name string away and must materialize it into read-only data.
volatile const char* g_leak_sink;

extern "C" result<int, rich_error> jx_rich_leaky(int x) {
  // A source location captured and used unconditionally, the leak the gate exists to
  // catch. The line is folded into the result, and the function name escapes through
  // the volatile sink and lands in read-only data, so the instruction diff catches
  // the added code and the forbidden-string scan catches the leaked string.
  std::source_location loc = std::source_location::current();
  g_leak_sink = loc.function_name();
  JMPXX_TRY(v, producer<rich_error>(x));
  return v * 10 + static_cast<int>(loc.line());
}
