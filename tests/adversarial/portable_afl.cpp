// SPDX-License-Identifier: MIT
#include "portable_fuzz_driver.hpp"

#include <cstdio>
#include <cstdint>

namespace {

int run_file(const char* path) {
  unsigned char buf[4096];
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return 2;
  std::size_t n = std::fread(buf, 1, sizeof(buf), f);
  std::fclose(f);
  adversarial::exercise(buf, n);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1) return run_file(argv[1]);
  unsigned char buf[4096];
  std::size_t n = std::fread(buf, 1, sizeof(buf), stdin);
  adversarial::exercise(buf, n);
  return 0;
}
