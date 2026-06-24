// SPDX-License-Identifier: MIT
// Constexpr tier: construction, inspection, extraction, and propagation all
// evaluate in a constant expression. Building this file is the test.
#include "jmpxx/core.hpp"

using namespace jmpxx;

constexpr result<int> kValue = 5;
static_assert(kValue.has_value() && kValue.value() == 5 && *kValue == 5);

constexpr result<int> kError = fail(error(7));
static_assert(!kError.has_value() && kError.error().code == 7);
static_assert(kError.value_or(99) == 99);

constexpr result<void> kVoid{};
static_assert(kVoid.has_value());

constexpr result<int> producer(int x) {
  if (x < 0) return fail(error(1));
  return x * 2;
}
constexpr result<int> consumer(int x) {
  JMPXX_TRY(y, producer(x));
  return y + 1;
}
static_assert(consumer(5).has_value() && consumer(5).value() == 11);
static_assert(!consumer(-1).has_value() && consumer(-1).error().code == 1);

constexpr result<void> void_consumer(int x) {
  JMPXX_TRYV(producer(x));
  return {};
}
static_assert(void_consumer(5).has_value());
static_assert(!void_consumer(-1).has_value());

int main() { return 0; }
