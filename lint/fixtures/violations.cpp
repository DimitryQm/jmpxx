// SPDX-License-Identifier: MIT
// Consumer code that violates the jmpxx usage discipline. Every function here is a known
// positive: the companion check must flag it. The matching clean.cpp holds the negatives the
// check must pass. The lint is run over both, and the test asserts the exact set of findings.
#include <jmpxx/core.hpp>

using namespace jmpxx;

result<int, error> make(int x);  // produces a result; defined elsewhere

// A produced failure dropped on the floor. [[nodiscard]] catches the bare form as a warning;
// the lint catches it as a finding and also catches the (void)-cast that silences the warning.
void discard_bare() {
  make(1);  // want: jmpxx-discarded-result
}

void discard_void_cast() {
  (void)make(2);  // want: jmpxx-discarded-result
}

// The value pulled out through the narrow accessor with no preceding success check. assume_value
// states a precondition the caller has not established here, so on a failure this is undefined
// behavior outside a hardened build.
int unchecked_assume() {
  result<int, error> r = make(3);
  return r.assume_value();  // want: jmpxx-unchecked-access
}

// The hand-written propagation the single-construct form expresses in one line. The lint flags
// it and points at JMPXX_TRY.
result<int, error> manual_propagation(int x) {
  result<int, error> r = make(x);
  if (!r) return fail(r.error());  // want: jmpxx-manual-propagation
  return r.assume_value() + 1;     // not a finding: r is checked by the if above
}
