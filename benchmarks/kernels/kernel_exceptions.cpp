// SPDX-License-Identifier: MIT
// The language-exceptions kernel: the depth-8 chain expressed with throw and a single
// catch at the landing. Its intermediate frames carry nothing on the success path, no
// discriminant and no check, which is the zero-cost-on-success property of exceptions,
// so on a non-inlined happy path this kernel does less per-frame work than any result
// type and the comparison reports that honestly. The cost moves to the failure path,
// where the throw drives the unwinder, and that is where the distribution is wide.
//
// This translation unit must be compiled with exceptions enabled even though the rest
// of the suite is built in the exception-free niche, so the build gives it -fexceptions
// while leaving every other kernel under -fno-exceptions. The thrown type never escapes
// this unit; the entry catches it and returns a plain int.
#include "spec.hpp"

namespace {

struct chain_error {
  int code;
  int domain;
};

template <int D>
JXB_FRAME_ATTR int ex_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) throw chain_error{jxb_fail_code, jxb_fail_domain};
    return x * 2;
  } else {
    return ex_frame<D - 1>(x) + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  try {
    return ex_frame<D>(x);
  } catch (const chain_error& e) {
    return -e.code;
  }
}

}  // namespace

extern "C" int exceptions_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}
