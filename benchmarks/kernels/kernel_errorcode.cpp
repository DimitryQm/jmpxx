// SPDX-License-Identifier: MIT
// The std::error_code kernel: the depth-8 chain expressed the way a codebase threads
// the standard error facility by hand, returning a std::error_code and passing the
// value through an out-parameter, with a check-and-return at every frame. std::error_code
// is a 16-byte value (an int plus a category pointer), heavier than the 8-byte jmpxx
// error, which the comparison reports as one axis where the standard facility costs more
// to move. <system_error> is hosted; this is a dev-only benchmark translation unit, so
// the hosted include is fine here.
#include "spec.hpp"

#include <system_error>

namespace {

template <int D>
JXB_FRAME_ATTR std::error_code ec_frame(int x, int& out) {
  if constexpr (D == 0) {
    if (x < 0) return std::make_error_code(std::errc::invalid_argument);
    out = x * 2;
    return {};
  } else {
    int v = 0;
    std::error_code ec = ec_frame<D - 1>(x, v);
    if (ec) return ec;  // the manual propagation
    out = v + jxb_frame_step;
    return {};
  }
}

template <int D>
int run(int x) {
  int v = 0;
  std::error_code ec = ec_frame<D>(x, v);
  return ec ? -jxb_fail_code : v;
}

}  // namespace

extern "C" int errorcode_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}
