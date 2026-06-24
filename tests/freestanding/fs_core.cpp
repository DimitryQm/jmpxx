// SPDX-License-Identifier: MIT
// Freestanding fixture: the minimal core builds and runs with exceptions and
// RTTI disabled and uses no hosted facility. It reports success through the exit
// code so it needs no I/O.
#include "jmpxx/core.hpp"

using namespace jmpxx;

static result<int> leaf(int x) {
  if (x < 0) return fail(error(42));
  return x * 2;
}
static result<int> w1(int x) { JMPXX_TRY(v, leaf(x)); return v + 1; }
static result<int> w2(int x) { JMPXX_TRY(v, w1(x)); return v + 1; }

int main() {
  auto ok = w2(5);
  if (!ok || ok.value() != 12) return 1;
  auto bad = w2(-1);
  if (bad || bad.error().code != 42) return 2;
  return 0;
}
