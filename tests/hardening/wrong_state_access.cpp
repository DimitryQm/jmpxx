// SPDX-License-Identifier: MIT
#include "jmpxx/core.hpp"

extern "C" int jx_assume_error_probe(jmpxx::result<int, jmpxx::error>& r) {
  return r.assume_error().code;
}

int main() {
  jmpxx::result<int, jmpxx::error> r = 7;
  return jx_assume_error_probe(r);
}
