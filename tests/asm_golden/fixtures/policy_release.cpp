// SPDX-License-Identifier: MIT
// Fixture for the release-diff gate. The same operation is written once under the
// minimal policy and once under the rich policy. In a release configuration the
// diagnostic layer is compiled out, so the rich policy's error becomes layout- and
// codegen-identical to the minimal policy's, and the two functions must generate
// identical machine code. The gate compares their label-canonicalized bodies.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"

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

extern "C" result<int, rich_error> jx_rich_op(int x) {
  JMPXX_TRY(v, producer<rich_error>(x));
  return v * 10;
}
