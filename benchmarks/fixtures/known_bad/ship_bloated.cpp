// SPDX-License-Identifier: MIT
// A deliberately bloated gate fixture for the size-delta gate's inverted self-test. It is
// ship_jmpxx.cpp plus extra arithmetic the hand-written baseline never does, so its object is
// materially larger and the delta goes positive past a zero budget. A gate that passed this
// would pass a real size regression, so this known-bad input checks the negative path, the
// same discipline the codegen gate's spilling fixture enforces.
#include "../../kernels/spec.hpp"

#include <jmpxx/core.hpp>

using jmpxx::error;
using jmpxx::fail;
using jmpxx::result;

namespace {

template <int D>
result<int, error> jx(int x) {
  if constexpr (D == 0) {
    if (x < 0) return fail(error(jxb_fail_code, jxb_fail_domain));
    return x * 2;
  } else {
    JMPXX_TRY(v, jx<D - 1>(x));
    return v + jxb_frame_step;
  }
}

}  // namespace

extern "C" int sized_chain(int x) {
  result<int, error> r = jx<jxb_depth>(x);
  int v = r.has_value() ? r.value() : -r.error().code;
  // A loop-carried recurrence the optimizer cannot fold to a constant, absent from the lean
  // baseline, so the object grows and the delta goes positive.
  for (int i = 0; i < 64; ++i) v = (v * 1103515245 + 12345) ^ (v >> 3);
  return v;
}
