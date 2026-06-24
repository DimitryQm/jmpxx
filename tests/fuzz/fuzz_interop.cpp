// SPDX-License-Identifier: MIT
// Fuzz target for the interop boundary and bridges. libFuzzer feeds arbitrary byte
// buffers; the harness decodes each through the untrusted-input boundary and then
// drives the interop bridges with whatever it decoded, so a buffer that survives
// validation still exercises the bridges with adversarial values. Built with the
// address and undefined-behavior sanitizers, so any out-of-bounds read, signed
// overflow, or other undefined behavior on any input is a crash the fuzzer reports.
// The harness never calls std::expected::value(), so it never trips the abort path
// that member takes under -fno-exceptions; it reads through operator* and error().
#include "../interop/decode_boundary.hpp"

#include "jmpxx/core.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/expected.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <system_error>

using namespace jmpxx;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
  // 1. The untrusted-input boundary: decode must never read out of bounds and must
  // return a defined result for every input.
  result<boundary::wire_error, error> decoded =
      boundary::decode(reinterpret_cast<const unsigned char*>(data), size);

  if (decoded.has_value()) {
    // 2. Feed the decoded values through the bridges, which must handle any value.
    const boundary::wire_error& w = decoded.value();
    error e(w.code, w.domain);
    std::error_code ec = to_error_code(e);
    error back = from_error_code(ec);
    if (back != e) __builtin_trap();  // a round-trip failure here is a defect

#if JMPXX_INTEROP_HAS_EXPECTED
    // 3. The std::expected bridge over the decoded value and a derived failure.
    std::expected<int, error> ev = to_expected(result<int, error>(w.code));
    result<int, error> rv = from_expected(ev);
    if (!rv.has_value() || rv.value() != w.code) __builtin_trap();
    std::expected<int, error> ee = to_expected(result<int, error>(fail(e)));
    if (ee.has_value()) __builtin_trap();
#endif
  } else {
    // 4. The failure path: a malformed buffer carries a defined decode failure that
    // also bridges without fault.
    error f = decoded.error();
    std::error_code ec = to_error_code(f);
    (void)from_error_code(ec);
  }

  // 5. Drive the optional-like adapter from the raw bytes too, so the adapter path
  // is fuzzed independently of the decode.
  std::optional<int> maybe =
      size ? std::optional<int>(data[0]) : std::optional<int>(std::nullopt);
  result<int, error> a =
      from_optional<error>(maybe, [] { return error(1); });
  (void)a.value_or(-1);
  return 0;
}
