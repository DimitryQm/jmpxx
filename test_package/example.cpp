// SPDX-License-Identifier: MIT
// Built and run by `conan create` against the packaged jmpxx, checking that the Conan
// package is consumable through find_package and the jmpxx::jmpxx target.
#include <jmpxx/core.hpp>

using namespace jmpxx;

static result<int> half(int x) {
  if (x % 2 != 0) return fail(error(1));
  return x / 2;
}
static result<int> step(int x) {
  JMPXX_TRY(v, half(x));
  return v + 1;
}

int main() {
  auto ok = step(10);
  auto bad = step(3);
  return (ok && ok.value() == 6 && !bad && bad.error().code == 1) ? 0 : 1;
}
