// SPDX-License-Identifier: MIT
#include "portable_fuzz_driver.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  adversarial::exercise(data, size);
  return 0;
}
