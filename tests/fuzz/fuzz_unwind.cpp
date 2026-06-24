// SPDX-License-Identifier: MIT
// Fuzz target for the unwind arm's payload boundary. The error an eject carries to its
// landing is a less-trusted value when it is derived from external input, so any value,
// including an adversarial one, must cross the eject boundary and reach the landing as a
// defined failure rather than undefined behavior. libFuzzer feeds arbitrary byte
// buffers; the harness derives an error from them, drives a real escape, and checks the
// payload round-trips. Built with the address and undefined-behavior sanitizers, so any
// out-of-bounds access or other undefined behavior on the eject or landing path on any
// input is a crash the fuzzer reports. The arm requires exceptions, which the fuzz build
// leaves enabled.
//
// The decode boundary from the interop tier is reused so a malformed buffer that does
// not even decode is exercised too, and a decoded record is then ejected, so the fuzzer
// drives the arm with values it did not choose.
#include "../interop/decode_boundary.hpp"

#include "jmpxx/unwind.hpp"
#include "jmpxx/core.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

using namespace jmpxx;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
  // 1. Derive an error directly from the raw bytes: any (code, domain) must round-trip
  // through an eject to the landing.
  int code = 0, domain = 0;
  if (size >= 4) std::memcpy(&code, data, 4);
  if (size >= 8) std::memcpy(&domain, data + 4, 4);
  auto direct = unwind::escape_scope<error>(
      [code, domain]() -> int { return unwind::eject(error(code, domain)); });
  if (direct.has_value() || direct.error().code != code ||
      direct.error().domain != domain)
    __builtin_trap();  // a lost or corrupted payload is a defect

  // 2. Route the bytes through the untrusted-input decode boundary, then eject whatever
  // it produced, so the arm carries a value validated at one boundary across another.
  result<boundary::wire_error, error> decoded =
      boundary::decode(reinterpret_cast<const unsigned char*>(data), size);
  error carried = decoded.has_value()
                      ? error(decoded.value().code, decoded.value().domain)
                      : decoded.error();
  auto landed = unwind::escape_scope<error>(
      [carried]() -> int { return unwind::eject(carried); });
  if (landed.has_value() || landed.error() != carried) __builtin_trap();
  return 0;
}
