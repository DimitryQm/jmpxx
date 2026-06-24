// SPDX-License-Identifier: MIT
// Fixture: a failure propagates from a leaf through eight frames to a single
// landing boundary using JMPXX_TRY. At -O2 the frames inline; the success
// return must carry no stack spill.
#include "jmpxx/core.hpp"

using namespace jmpxx;

static result<int, int> leaf(int x) {
  if (x < 0) return fail(1);
  return x * 2;
}
static result<int, int> w1(int x) { JMPXX_TRY(v, leaf(x)); return v + 1; }
static result<int, int> w2(int x) { JMPXX_TRY(v, w1(x)); return v + 1; }
static result<int, int> w3(int x) { JMPXX_TRY(v, w2(x)); return v + 1; }
static result<int, int> w4(int x) { JMPXX_TRY(v, w3(x)); return v + 1; }
static result<int, int> w5(int x) { JMPXX_TRY(v, w4(x)); return v + 1; }
static result<int, int> w6(int x) { JMPXX_TRY(v, w5(x)); return v + 1; }
static result<int, int> chain8(int x) { JMPXX_TRY(v, w6(x)); return v + 1; }

extern "C" int jx_propagate_eight(int x) {
  auto r = chain8(x);
  return r.value_or(-1);
}
