// SPDX-License-Identifier: MIT
// The debug-only diagnostic layer. In a debug build a rich_error records where the
// failure began and the path it propagated; a landing scope bounds that context's
// lifetime, and diagnostic::print renders it. In a release build the layer compiles to
// nothing and rich_error is identical to the minimal error, so the same program pays
// nothing for the context it no longer carries.
#include <jmpxx/core.hpp>
#include <jmpxx/diagnostics.hpp>

#include <cstdio>

using jmpxx::fail;
using jmpxx::result;
using jmpxx::rich_error;

result<int, rich_error> leaf(int x) {
  if (x < 0) return fail(rich_error(7));  // origin captured at this construction site
  return x;
}
result<int, rich_error> mid(int x) {
  JMPXX_TRY(v, leaf(x));  // records a propagation hop in debug
  return v + 1;
}
result<int, rich_error> top(int x) {
  JMPXX_TRY(v, mid(x));
  return v + 1;
}

int main() {
  // The landing scope owns the diagnostic context of failures created under it.
  jmpxx::landing root;

  result<int, rich_error> r = top(-1);
  if (!r) {
    std::printf("failure: code=%d\n", r.error().code);
    // In debug this prints the origin and each propagation hop; in release it is a
    // no-op because the context does not exist.
    jmpxx::diagnostic::print(r.error(), stdout);
#if JMPXX_DIAGNOSTICS_ENABLED
    std::printf("chain length: %d hop(s)\n",
                jmpxx::diagnostic::chain_length(r.error()));
#else
    std::printf("release build: the diagnostic layer is compiled out\n");
#endif
  }
  return 0;
}
