// SPDX-License-Identifier: MIT
// Correct use: inspecting the transport compiles cleanly.
#include "jmpxx/core.hpp"

using namespace jmpxx;

result<int> producer() { return 1; }

int main() {
  auto r = producer();
  return r.has_value() ? 0 : 1;
}
