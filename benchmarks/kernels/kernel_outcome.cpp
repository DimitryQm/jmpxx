// SPDX-License-Identifier: MIT
// The Boost.Outcome kernel. Outcome's default result throws std::system_error on a bad
// access, which the exception-free niche cannot use, so this kernel uses outcome::unchecked
// (the all_narrow no-value policy), whose accessors never throw, and reads through
// assume_value / assume_error. Propagation is BOOST_OUTCOME_TRY, Outcome's own early-return
// macro, the direct counterpart of JMPXX_TRY. The error is a two-int POD so the transport
// stays trivially copyable and the comparison is against a payload the same shape as jmpxx's.
#include "spec.hpp"

#if defined(__has_include) && __has_include(<boost/outcome.hpp>)

#include <boost/outcome.hpp>

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

namespace {

struct oc_error {
  int code;
  int domain;
};

template <int D>
JXB_FRAME_ATTR outcome::unchecked<int, oc_error> oc_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) return oc_error{jxb_fail_code, jxb_fail_domain};
    return x * 2;
  } else {
    BOOST_OUTCOME_TRY(int v, oc_frame<D - 1>(x));
    return v + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  outcome::unchecked<int, oc_error> r = oc_frame<D>(x);
  return r.has_value() ? r.assume_value() : -r.assume_error().code;
}

}  // namespace

extern "C" int outcome_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}

#else  // Boost.Outcome not present: stub so the suite still links and reports it unavailable.
extern "C" int outcome_chain(int, int) { return -jxb_fail_code; }
#endif
