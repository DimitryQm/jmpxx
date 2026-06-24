// SPDX-License-Identifier: MIT
// Compile gate for the arm's unwind-tables precondition. Under -fno-exceptions,
// including jmpxx/unwind.hpp is a compile-time refusal, because without cleanup tables a
// forced unwind would skip destructors. Under exceptions, the same source compiles and
// uses the arm. The CMake compile-fail tier builds it both ways and asserts each outcome.
#include "jmpxx/unwind.hpp"
#include "jmpxx/core.hpp"

using namespace jmpxx;

int main() {
  auto r = unwind::escape_scope<error>(
      []() -> int { return unwind::eject(error(1)); });
  return r.has_value() ? 0 : 1;
}
