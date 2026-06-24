// SPDX-License-Identifier: MIT
// Property tier: the exactly-one invariant holds under randomized operation
// sequences, including for a value type whose move can throw. Compiled with
// exceptions enabled so the throwing-move path is exercised; the library itself
// uses no exceptions.
#include "jmpxx/core.hpp"

#include <cstdio>
#include <random>

using namespace jmpxx;

namespace {

// A value type whose move can throw, to probe the never-valueless guarantee.
struct throwing_move {
  int v;
  explicit throwing_move(int x) : v(x) {}
  throwing_move(const throwing_move&) = default;
  throwing_move(throwing_move&& o) : v(o.v) {
    if (o.v % 7 == 0) throw 1;  // some moves throw
  }
  throwing_move& operator=(const throwing_move&) = default;
  throwing_move& operator=(throwing_move&&) = default;
};

}  // namespace

int main() {
  std::mt19937 rng(0xC0FFEE);
  long long violations = 0;
  const int trivial_iters = 200000;

  // Trivial payload: construction, copy, move, and assignment all keep exactly
  // one of value or error, never neither and never both.
  for (int i = 0; i < trivial_iters; ++i) {
    bool want_value = (rng() & 1) != 0;
    result<int, error> r = want_value ? result<int, error>(int(rng() % 1000))
                                       : result<int, error>(fail(error(int(rng() % 1000))));
    if (r.has_value() == r.has_error()) ++violations;
    if (r.has_value() != want_value) ++violations;

    result<int, error> c = r;
    if (c.has_value() != r.has_value()) ++violations;
    result<int, error> m = static_cast<result<int, error>&&>(c);
    if (m.has_value() != r.has_value()) ++violations;

    result<int, error> a = 0;
    a = r;  // assignment, possibly cross-state
    if (a.has_value() != r.has_value()) ++violations;
    if (a.has_value() == a.has_error()) ++violations;
  }

  // Throwing-move payload: a move that throws leaves the source a value and
  // produces no observable valueless object.
  long long throwing_moves = 0;
  for (int i = 1; i <= 2000; ++i) {
    result<throwing_move, error> src(in_place, i);  // no move on construction
    if (!src.has_value()) {
      ++violations;
      continue;
    }
    try {
      result<throwing_move, error> dst =
          static_cast<result<throwing_move, error>&&>(src);
      if (dst.has_value() == dst.has_error()) ++violations;  // dst exactly-one
    } catch (...) {
      ++throwing_moves;
    }
    // Whether or not the move threw, src holds exactly one alternative, a value.
    if (src.has_value() == src.has_error()) ++violations;
    if (!src.has_value()) ++violations;
  }

  std::printf("property: trivial_iters=%d throwing_moves_caught=%lld violations=%lld\n",
              trivial_iters, throwing_moves, violations);
  return violations == 0 ? 0 : 1;
}
