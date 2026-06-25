// SPDX-License-Identifier: MIT
// MemorySanitizer tier over the portable surface that is most sensitive to
// uninitialized reads: the transport union, placement-new state changes, fixed
// diagnostic buffers, and byte-decoded bridge boundaries.
#include "../interop/decode_boundary.hpp"

#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"

#include <cstdio>

using namespace jmpxx;

namespace {

result<int, rich_error> leaf(int x) {
  if (x < 0) return fail(rich_error(23, 5));
  return x;
}
result<int, rich_error> chain(int x) {
  JMPXX_TRY(v, leaf(x));
  return v + 1;
}

}  // namespace

int main() {
  volatile int sink = 0;
  for (int i = 0; i < 4096; ++i) {
    result<int, error> r = i;
    result<int, error> e = fail(error(i, i % 13));
    r = e;
    if (r.has_value()) return 1;
    sink += r.error().code;
    e = i + 1;
    if (!e.has_value()) return 2;
    sink += e.value();
  }

#if JMPXX_DIAGNOSTICS_ENABLED
  {
    landing root;
    result<int, rich_error> bad = chain(-1);
    if (bad.has_value()) return 3;
    diagnostic::context c = diagnostic::inspect(bad.error());
    if (!c.available || c.hop_count != 1) return 4;
    sink += c.hop_count;
  }
#endif

  unsigned char good[] = {3, 0x78, 0x56, 0x34, 0x12, 2, 'o', 'k'};
  result<boundary::wire_error, error> decoded = boundary::decode(good, sizeof(good));
  if (!decoded.has_value()) return 5;
  sink += decoded.value().payload[0];

  erased_error erased(9);
  sink += erased.value();

  std::printf("msan portable surface clean: %d\n", static_cast<int>(sink));
  return 0;
}
