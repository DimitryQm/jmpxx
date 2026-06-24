// SPDX-License-Identifier: MIT
// Consumer code that obeys the jmpxx usage discipline, including tricky shapes that resemble a
// violation but are not. The companion check must report no finding over this file. A false verdict
// on correct code is the failure the check must avoid, so these negatives matter as much as the
// positives in violations.cpp.
#include <jmpxx/core.hpp>

using namespace jmpxx;

result<int, error> make(int x);

// The result is inspected, not discarded: it is bound and its state is read.
int inspected() {
  result<int, error> r = make(1);
  if (!r) return -1;
  return r.assume_value();  // guarded by the early-return check above
}

// The narrow access sits inside the success branch of a check on the same result.
int guarded_inside() {
  result<int, error> r = make(2);
  if (r.has_value()) return r.assume_value();
  return 0;
}

// Proper single-construct propagation. JMPXX_TRY is the sanctioned form, so it is not a manual
// propagation finding even though it expands to a checked early return.
result<int, error> proper_try(int x) {
  JMPXX_TRY(v, make(x));
  return v + 1;
}

// The checked accessors value() and operator* verify the state and terminate on misuse, a
// defined event, so an unguarded use of them is a deliberate must-succeed assertion rather than
// the undefined narrow access the check targets. The check does not flag them.
int checked_accessor() {
  result<int, error> r = make(3);
  return r.value();
}

// A conditional return of a failure that is NOT the inspected result's error. It resembles the
// manual-propagation pattern but forwards a different error, so the single-construct form would
// not express it and the check must not flag it.
result<int, error> different_error(int x) {
  result<int, error> r = make(x);
  if (!r) return fail(error(99));
  return r.assume_value();
}

// The result feeds value_or, which needs no separate check, and is not discarded.
int via_value_or() {
  return make(4).value_or(-1);
}
