// SPDX-License-Identifier: MIT
// The hand-written baseline for the gate fixtures: the same inlinable depth-8 chain as
// ship_jmpxx.cpp, threaded by hand with a union of the value and the error discriminated by a
// flag, returned by value, with a manual check-and-return at every frame. Same 12-byte layout
// as result<int, error>, so the delta the gates measure is the abstraction's cost and not a
// data-layout choice. The claim is that the delta is zero.
#include "../kernels/spec.hpp"

namespace {

struct hw_result {
  union {
    int value;
    struct {
      int code;
      int domain;
    } err;
  };
  bool ok;
};

template <int D>
hw_result hw(int x) {
  if constexpr (D == 0) {
    if (x < 0) return hw_result{.err = {jxb_fail_code, jxb_fail_domain}, .ok = false};
    return hw_result{.value = x * 2, .ok = true};
  } else {
    hw_result r = hw<D - 1>(x);
    if (!r.ok) return r;
    return hw_result{.value = r.value + jxb_frame_step, .ok = true};
  }
}

}  // namespace

extern "C" int sized_chain(int x) {
  hw_result r = hw<jxb_depth>(x);
  return r.ok ? r.value : -r.err.code;
}
