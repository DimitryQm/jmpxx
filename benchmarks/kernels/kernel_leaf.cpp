// SPDX-License-Identifier: MIT
// The Boost.LEAF kernel, the closest design rival to jmpxx. leaf::result<T> carries only a
// flag and an integer error id; the error object itself is moved to thread-local storage and
// gathered at the landing, independent of call depth, the same out-of-band shape jmpxx uses
// for its diagnostic context. Propagation is BOOST_LEAF_AUTO and the landing is
// try_handle_all with a type-matched handler and a catch-all, which is LEAF's idiom and needs
// no language try/catch, so it compiles in the exception-free niche. This kernel is the one to
// watch on the heavy-error, high-failure cell, where an out-of-band design keeps the payload
// off the return path that in-band result types copy through every frame.
#include "spec.hpp"

#if defined(__has_include) && __has_include(<boost/leaf.hpp>)

#include <boost/leaf.hpp>

namespace leaf = boost::leaf;

namespace {

struct e_code {
  int code;
  int domain;
};

template <int D>
JXB_FRAME_ATTR leaf::result<int> leaf_frame(int x) {
  if constexpr (D == 0) {
    if (x < 0) return leaf::new_error(e_code{jxb_fail_code, jxb_fail_domain});
    return x * 2;
  } else {
    BOOST_LEAF_AUTO(v, leaf_frame<D - 1>(x));
    return v + jxb_frame_step;
  }
}

template <int D>
int run(int x) {
  return leaf::try_handle_all(
      [&]() -> leaf::result<int> {
        BOOST_LEAF_AUTO(v, leaf_frame<D>(x));
        return v;
      },
      [](e_code const& e) { return -e.code; },
      []() { return -jxb_fail_code; });
}

}  // namespace

extern "C" int leaf_chain(int x, int depth) {
  return depth >= jxb_depth_deep ? run<jxb_depth_deep>(x) : run<jxb_depth>(x);
}

#else  // Boost.LEAF not present: stub so the suite still links and reports it unavailable.
extern "C" int leaf_chain(int, int) { return -jxb_fail_code; }
#endif
