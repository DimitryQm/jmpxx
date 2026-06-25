// SPDX-License-Identifier: MIT
// Usage witness for the portable surface. The scenarios use fixed
// buffers, no exceptions, no RTTI, no heap, and a single typed landing pattern.
#include "jmpxx/core.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

void* operator new(std::size_t) {
  std::fprintf(stderr, "usage scenarios: heap allocation observed\n");
  std::abort();
}
void* operator new[](std::size_t) {
  std::fprintf(stderr, "usage scenarios: heap allocation observed\n");
  std::abort();
}

void operator delete(void*) noexcept {}
void operator delete(void*, std::size_t) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete[](void*, std::size_t) noexcept {}

namespace {

[[noreturn]] void die(const char* what) {
  std::fprintf(stderr, "usage scenarios: %s\n", what);
  std::abort();
}

enum class domain : int {
  sensor = 1,
  feed = 2,
  storage = 3,
};

struct sensor_frame {
  std::uint16_t sensor_id;
  std::uint16_t millivolts;
  std::uint8_t flags;
};

struct market_tick {
  std::uint32_t instrument;
  std::uint32_t price_ticks;
  std::uint32_t quantity;
  std::uint8_t side;
};

struct page_header {
  std::uint32_t magic;
  std::uint16_t payload_bytes;
  std::uint16_t checksum;
};

struct cleanup_counter {
  int* count;
  explicit cleanup_counter(int& c) noexcept : count(&c) {}
  ~cleanup_counter() { ++*count; }
};

std::uint16_t read_u16(const std::uint8_t* data) noexcept {
  return static_cast<std::uint16_t>(data[0]) |
         static_cast<std::uint16_t>(data[1] << 8);
}

std::uint32_t read_u32(const std::uint8_t* data) noexcept {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

jmpxx::result<sensor_frame> parse_sensor_frame(const std::uint8_t* data,
                                               std::size_t size) noexcept {
  if (size < 5) return jmpxx::fail(jmpxx::error(10, static_cast<int>(domain::sensor)));
  sensor_frame frame{read_u16(data), read_u16(data + 2), data[4]};
  if (frame.sensor_id == 0)
    return jmpxx::fail(jmpxx::error(11, static_cast<int>(domain::sensor)));
  if (frame.millivolts > 5000)
    return jmpxx::fail(jmpxx::error(12, static_cast<int>(domain::sensor)));
  return frame;
}

jmpxx::result<int> calibrate_sensor(const sensor_frame& frame) noexcept {
  if ((frame.flags & 0x80u) != 0)
    return jmpxx::fail(jmpxx::error(13, static_cast<int>(domain::sensor)));
  return static_cast<int>(frame.millivolts) - 1200;
}

jmpxx::result<int> embedded_control_path(const std::uint8_t* data,
                                         std::size_t size,
                                         int& destructors) noexcept {
  cleanup_counter guard(destructors);
  JMPXX_TRY(frame, parse_sensor_frame(data, size));
  JMPXX_TRY(calibrated, calibrate_sensor(frame));
  if (calibrated < -200)
    return jmpxx::fail(jmpxx::error(14, static_cast<int>(domain::sensor)));
  return calibrated;
}

jmpxx::result<market_tick> decode_market_tick(const std::uint8_t* data,
                                              std::size_t size) noexcept {
  if (size < 13) return jmpxx::fail(jmpxx::error(20, static_cast<int>(domain::feed)));
  market_tick tick{read_u32(data), read_u32(data + 4), read_u32(data + 8), data[12]};
  if (tick.instrument == 0)
    return jmpxx::fail(jmpxx::error(21, static_cast<int>(domain::feed)));
  if (tick.price_ticks == 0 || tick.quantity == 0)
    return jmpxx::fail(jmpxx::error(22, static_cast<int>(domain::feed)));
  if (tick.side != 'B' && tick.side != 'S')
    return jmpxx::fail(jmpxx::error(23, static_cast<int>(domain::feed)));
  return tick;
}

jmpxx::result<std::uint64_t> risk_notional(const market_tick& tick) noexcept {
  std::uint64_t notional = static_cast<std::uint64_t>(tick.price_ticks) *
                           static_cast<std::uint64_t>(tick.quantity);
  if (notional > 50'000'000ull)
    return jmpxx::fail(jmpxx::error(24, static_cast<int>(domain::feed)));
  return notional;
}

jmpxx::result<std::uint64_t> low_latency_feed_path(const std::uint8_t* data,
                                                   std::size_t size) noexcept {
  JMPXX_TRY(tick, decode_market_tick(data, size));
  return risk_notional(tick);
}

std::uint16_t checksum16(const std::uint8_t* data, std::size_t size) noexcept {
  std::uint32_t sum = 0;
  for (std::size_t i = 0; i < size; ++i) sum += data[i];
  return static_cast<std::uint16_t>((sum & 0xffffu) ^ (sum >> 16));
}

jmpxx::result<page_header, jmpxx::erased_error> decode_page_header(
    const std::uint8_t* data, std::size_t size) noexcept {
  static constexpr struct storage_domain final : jmpxx::error_domain {
    const char* name() const noexcept override { return "usage.storage"; }
    const char* message(int) const noexcept override { return "storage page error"; }
  } storage{};

  if (size < 8) return jmpxx::fail(jmpxx::erased_error(30, storage));
  page_header h{read_u32(data), read_u16(data + 4), read_u16(data + 6)};
  if (h.magic != 0x584a4d50u)
    return jmpxx::fail(jmpxx::erased_error(31, storage));
  if (h.payload_bytes > size - 8)
    return jmpxx::fail(jmpxx::erased_error(32, storage));
  if (checksum16(data + 8, h.payload_bytes) != h.checksum)
    return jmpxx::fail(jmpxx::erased_error(33, storage));
  return h;
}

jmpxx::result<std::uint16_t, jmpxx::erased_error> storage_or_network_path(
    const std::uint8_t* data, std::size_t size) noexcept {
  JMPXX_TRY(header, decode_page_header(data, size));
  auto optional_payload_size =
      header.payload_bytes != 0 ? &header.payload_bytes : nullptr;
  return jmpxx::from_optional<jmpxx::erased_error>(
      optional_payload_size,
      [] { return jmpxx::erased_error(34, jmpxx::generic_domain()); });
}

void assert_failure(jmpxx::result<int> r, int code, int domain_tag) {
  if (r.has_value()) die("expected failure reached success");
  if (r.error().code != code || r.error().domain != domain_tag)
    die("failure payload changed");
}

}  // namespace

int main() {
  std::uint8_t good_sensor[] = {0x2a, 0, 0x20, 0x0f, 0};
  int destructors = 0;
  auto calibrated = embedded_control_path(good_sensor, sizeof(good_sensor), destructors);
  if (!calibrated.has_value() || calibrated.value() != 2672 || destructors != 1)
    die("embedded control success path diverged");

  std::uint8_t bad_sensor[] = {0x2a, 0, 0x20, 0x0f, 0x80};
  assert_failure(embedded_control_path(bad_sensor, sizeof(bad_sensor), destructors),
                 13, static_cast<int>(domain::sensor));
  if (destructors != 2) die("RAII cleanup did not run on embedded failure");

  std::uint8_t good_tick[] = {
      0x44, 0x33, 0x22, 0x11, 0x10, 0x27, 0, 0, 0x64, 0, 0, 0, 'B'};
  auto notional = low_latency_feed_path(good_tick, sizeof(good_tick));
  if (!notional.has_value() || notional.value() != 1'000'000ull)
    die("low-latency feed success path diverged");

  std::uint8_t bad_tick[] = {
      0x44, 0x33, 0x22, 0x11, 0x10, 0x27, 0, 0, 0xff, 0xff, 0, 0, 'S'};
  auto rejected = low_latency_feed_path(bad_tick, sizeof(bad_tick));
  if (rejected.has_value() || rejected.error().code != 24)
    die("low-latency risk failure diverged");

  std::uint8_t page[] = {
      'P', 'M', 'J', 'X', 4, 0, 10, 0, 1, 2, 3, 4};
  auto payload = storage_or_network_path(page, sizeof(page));
  if (!payload.has_value() || payload.value() != 4)
    die("storage page success path diverged");

  page[6] = 0;
  auto corrupt = storage_or_network_path(page, sizeof(page));
  if (corrupt.has_value() || corrupt.error().value() != 33 ||
      corrupt.error().in_domain(jmpxx::generic_domain()))
    die("storage page failure diverged");

  std::printf("usage scenarios: fixed-buffer embedded, feed, and storage paths clean\n");
  return 0;
}
