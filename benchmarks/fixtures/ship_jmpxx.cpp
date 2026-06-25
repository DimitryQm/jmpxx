// SPDX-License-Identifier: MIT
// The jmpxx gate fixture: one inlinable depth-8 chain in the configuration that ships.
// The size-delta, codegen, and compile-cost gates weigh this against ship_handwritten.cpp,
// which does the same work by hand, so the difference between the two objects is the
// library's contribution. The frames are left inlinable on purpose, because the zero-overhead
// claim is about the optimized, inlined result a real build produces, and at -O2 this chain
// folds to the same branchless select the hand-written baseline does. The benchmark kernels
// under ../kernels measure a different thing, the per-frame mechanism cost across real frames,
// so they keep their frames non-inlined; the two fixtures are separate because they check
// separate properties.
#include "../kernels/spec.hpp"

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
  return r.has_value() ? r.value() : -r.error().code;
}
