// SPDX-License-Identifier: MIT
// The jmpxx kernel: the depth-8 chain expressed with result<int, error> and the
// single-construct JMPXX_TRY propagation. The size and compile-cost gates weigh this
// translation unit against the hand-written baseline, and the runtime benchmark times
// this function against every incumbent.
#include "spec.hpp"

#include <jmpxx/core.hpp>

using jmpxx::error;
using jmpxx::fail;
using jmpxx::result;

namespace {

// Template recursion gives jxb_depth distinct frames rather than a runtime loop the
// optimizer could turn into a single iteration, so JXB_FRAME_ATTR applies to a real
// frame at each level. The leaf fails on a negative input; each wrapper forwards the
// value with JMPXX_TRY and adds one step.
template <int D>
JXB_FRAME_ATTR result<int, error> jmpxx_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) return fail(error(jxb_fail_code, jxb_fail_domain));
    return x * 2;
  } else {
    JMPXX_TRY(v, jmpxx_frame<D - 1>(x));
    return v + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  result<int, error> r = jmpxx_frame<D>(x);
  return r.has_value() ? r.value() : -r.error().code;
}

}  // namespace

extern "C" int jmpxx_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}
