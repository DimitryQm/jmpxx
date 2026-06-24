// SPDX-License-Identifier: MIT
// The tl::expected kernel, Sy Brand's pre-C++23 expected. Its value() throws (or terminates
// under -fno-exceptions), so the kernel reads through operator*, the unchecked accessor, and
// threads the failure with a hand-written check-and-return, the same in-band model as
// std::expected. The error rides inside the return value, so this kernel and std::expected
// are the in-band contenders the heavy-payload cell separates from the out-of-band designs.
#include "spec.hpp"

#if defined(__has_include) && __has_include(<tl/expected.hpp>)

#include <tl/expected.hpp>

namespace {

struct tl_error {
  int code;
  int domain;
};

template <int D>
JXB_FRAME_ATTR tl::expected<int, tl_error> tl_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) return tl::unexpected(tl_error{jxb_fail_code, jxb_fail_domain});
    return x * 2;
  } else {
    auto r = tl_frame<D - 1>(x);
    if (!r) return tl::unexpected(r.error());
    return *r + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  auto r = tl_frame<D>(x);
  return r.has_value() ? *r : -r.error().code;
}

}  // namespace

extern "C" int tlexpected_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}

#else  // tl::expected not present: stub so the suite still links and reports it unavailable.
extern "C" int tlexpected_chain(int, int) { return -jxb_fail_code; }
#endif
