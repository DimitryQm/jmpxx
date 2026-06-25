// SPDX-License-Identifier: MIT
#include "jmpxx/erased.hpp"

#include <cstring>

extern "C" const char* jx_erased_domain_probe(const jmpxx::erased_error& e) {
  return e.domain_name();
}

int main() {
  jmpxx::erased_error e(1);
  std::memset(&e, 0, sizeof(e));
  const char* name = jx_erased_domain_probe(e);
  return name ? 0 : 1;
}
