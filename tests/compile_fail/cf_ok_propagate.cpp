// SPDX-License-Identifier: MIT
// Correct use: propagating with JMPXX_TRY compiles cleanly.
#include "jmpxx/core.hpp"

using namespace jmpxx;

result<int> producer() { return 1; }

result<int> consumer() {
  JMPXX_TRY(v, producer());
  return v + 1;
}

int main() { return consumer().value_or(-1) == 2 ? 0 : 1; }
