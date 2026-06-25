// SPDX-License-Identifier: MIT
// A swallowing catch-all on the escape path is never a silent wrong landing. A
// non-cooperative catch(...) that does not rethrow is handled differently by different
// C++ runtimes, and this fixture checks every safe outcome. libstdc++ and libc++abi
// let the catch-all consume the escape, which the arm turns into a defined termination;
// libcxxrt transits the catch-all so the escape reaches its landing. The one outcome the
// arm must never produce is a silent loss, where the scope returns a value the program
// never made. The fixture provokes the swallow and reports which safe outcome occurred,
// or a defect if the escape was silently lost.
#include "jmpxx/unwind.hpp"
#include "jmpxx/core.hpp"

#include <cstdio>

using namespace jmpxx;

namespace {
int leaf() {
  unwind::eject(error(7));
  return 0;  // not reached
}
int swallow() {
  try {
    return leaf();
  } catch (...) {
    return 0;  // non-cooperative: does not rethrow
  }
}
}  // namespace

int main() {
  auto r = unwind::escape_scope<error>([] { return swallow(); });
  // Reached only if the runtime did not fail-fast on the swallow. Either it transited the
  // catch-all and the escape landed as its error, which is safe, or the escape was
  // silently lost and the scope returned a value the program never produced, the defect.
  if (r.has_value()) {
    std::printf("DEFECT: a swallowed escape was silently lost (value=%d)\n", r.value());
    return 1;
  }
  std::printf("the catch-all transited the escape; it landed as error code=%d\n",
              r.error().code);
  return 0;
}
