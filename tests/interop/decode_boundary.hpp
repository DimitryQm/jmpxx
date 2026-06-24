// SPDX-License-Identifier: MIT
// A representative untrusted-input boundary, shared by the behavioral boundary test
// and the fuzz target. It stands in for the real situation a networking, storage, or
// IPC component meets: a serialized error arrives from another process or host, as a
// byte buffer that may be truncated, oversized, or otherwise malformed, and must be
// turned into a jmpxx failure without ever reading out of bounds. Every field is
// validated against the remaining input before it is read, so a malformed buffer
// yields a defined jmpxx failure rather than undefined behavior, which is the
// validation this boundary exists to perform. The decoded record then feeds the
// interop bridges, so the fuzzer drives those with adversarial values as well.
//
// Wire format (all little-endian, fixed): byte 0 is a domain tag, bytes 1..4 are a
// signed 32-bit code, byte 5 is a payload length, and that many payload bytes follow.
// The format is deliberately small, since it exercises the validation rather than
// serving as a real schema.
#ifndef JMPXX_TEST_DECODE_BOUNDARY_HPP
#define JMPXX_TEST_DECODE_BOUNDARY_HPP

#include "jmpxx/core.hpp"

#include <cstddef>
#include <cstdint>

namespace boundary {

// The decode's own failure codes, in its own domain, so a malformed input is a
// named, defined failure.
enum decode_error {
  truncated_header = 1,  // fewer than the six header bytes are present
  bad_domain = 2,        // the domain tag is outside the accepted range
  payload_overruns = 3,  // the declared payload length exceeds the remaining bytes
};
inline constexpr int decode_domain = 0x0DEC;

inline constexpr int max_payload = 32;

struct wire_error {
  int domain;
  int code;
  unsigned char payload[max_payload];
  int payload_len;
};

// Decode an untrusted buffer into a wire_error, validating before every read. The
// buffer pointer may be null when size is zero, which is handled as truncated rather
// than dereferenced.
[[nodiscard]] inline jmpxx::result<wire_error, jmpxx::error> decode(
    const unsigned char* data, std::size_t size) noexcept {
  constexpr std::size_t header = 6;  // domain(1) + code(4) + payload_len(1)
  if (size < header)
    return jmpxx::fail(jmpxx::error(truncated_header, decode_domain));

  wire_error w{};
  w.domain = data[0];
  // Only a bounded set of domain tags is accepted; anything else is malformed.
  if (w.domain > 16)
    return jmpxx::fail(jmpxx::error(bad_domain, decode_domain));

  // Reassemble the signed 32-bit code from four bytes without reading past them.
  std::uint32_t raw = static_cast<std::uint32_t>(data[1]) |
                      (static_cast<std::uint32_t>(data[2]) << 8) |
                      (static_cast<std::uint32_t>(data[3]) << 16) |
                      (static_cast<std::uint32_t>(data[4]) << 24);
  w.code = static_cast<int>(raw);

  std::size_t declared = data[5];
  std::size_t available = size - header;
  if (declared > available || declared > static_cast<std::size_t>(max_payload))
    return jmpxx::fail(jmpxx::error(payload_overruns, decode_domain));

  w.payload_len = static_cast<int>(declared);
  for (std::size_t i = 0; i < declared; ++i) w.payload[i] = data[header + i];
  return w;
}

}  // namespace boundary

#endif  // JMPXX_TEST_DECODE_BOUNDARY_HPP
