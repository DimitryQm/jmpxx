// SPDX-License-Identifier: MIT
// A deliberately slowed jmpxx kernel for the perf gate's inverted self-test. It runs the same
// chain as kernel_jmpxx.cpp and then does extra work on the result, so it is measurably
// slower than the hand-written baseline. The perf gate bounds the jmpxx-to-baseline ratio,
// and swapping this kernel in drives the ratio past the bound, which the gate must reject.
// A gate that passed a slowed kernel would pass a real performance regression, so this
// known-bad input checks the perf gate's negative path, the same discipline the codegen
// and size gates already hold.
#include "spec.hpp"

#include <jmpxx/core.hpp>

using jmpxx::error;
using jmpxx::fail;
using jmpxx::result;

namespace {

template <int D>
JXB_FRAME_ATTR result<int, error> slow_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) return fail(error(jxb_fail_code, jxb_fail_domain));
    return x * 2;
  } else {
    JMPXX_TRY(v, slow_frame<D - 1>(x));
    return v + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  result<int, error> r = slow_frame<D>(x);
  int v = r.has_value() ? r.value() : -r.error().code;
  // Injected work the baseline does not do, enough that the ratio clears the gate's bound
  // by a wide margin on any runner. It depends on the runtime result so it is not folded away.
  for (int i = 0; i < 96; ++i) v = (v * 1103515245 + 12345) ^ (v >> 3);
  return v;
}

}  // namespace

extern "C" int jmpxx_slow_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}
