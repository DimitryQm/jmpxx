// SPDX-License-Identifier: MIT
// A minimal consumer of jmpxx pulled in through add_subdirectory or FetchContent. It
// uses the result transport and single-construct propagation, so a successful build
// and run check that the target and its include path are wired correctly through the
// channel. It exits non-zero if the library does not behave as expected.
#include <jmpxx/core.hpp>

using namespace jmpxx;

static result<int> parse(int raw) {
  if (raw < 0) return fail(error(1));
  return raw * 2;
}
static result<int> step(int raw) {
  JMPXX_TRY(v, parse(raw));
  return v + 1;
}

int main() {
  auto ok = step(20);
  auto bad = step(-1);
  return (ok && ok.value() == 41 && !bad && bad.error().code == 1) ? 0 : 1;
}
