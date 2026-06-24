// SPDX-License-Identifier: MIT
// Compile-fail: a sub-call's failure ignored inside a function that itself
// returns a result is a diagnostic (an error under -Werror).
#include "jmpxx/core.hpp"

using namespace jmpxx;

result<int> producer() { return 1; }

result<int> consumer() {
  producer();  // failure ignored at a propagation site
  return 0;
}
