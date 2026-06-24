// SPDX-License-Identifier: MIT
// Compile-fail: discarding a transport is a diagnostic (an error under -Werror).
#include "jmpxx/core.hpp"

using namespace jmpxx;

result<int> producer() { return 1; }

int main() {
  producer();  // discarded transport
  return 0;
}
