// SPDX-License-Identifier: MIT
#ifndef JMPXX_TEST_PORTABLE_FUZZ_DRIVER_HPP
#define JMPXX_TEST_PORTABLE_FUZZ_DRIVER_HPP

#include "../interop/decode_boundary.hpp"

#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/expected.hpp"
#include "jmpxx/reflect.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

#if JMPXX_INTEROP_HAS_EXPECTED
#include <expected>
#endif

namespace adversarial {

[[noreturn]] inline void reject(const char* reason) noexcept {
  jmpxx::platform::fail_fast(reason);
}

class bytes {
  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t pos_ = 0;

 public:
  bytes(const std::uint8_t* data, std::size_t size) noexcept
      : data_(data), size_(size) {}

  [[nodiscard]] std::uint8_t take() noexcept {
    if (pos_ >= size_) return 0;
    return data_[pos_++];
  }

  [[nodiscard]] int take_int() noexcept {
    unsigned raw = static_cast<unsigned>(take()) |
                   (static_cast<unsigned>(take()) << 8) |
                   (static_cast<unsigned>(take()) << 16) |
                   (static_cast<unsigned>(take()) << 24);
    return static_cast<int>(raw);
  }

  [[nodiscard]] bool magic() const noexcept {
    return size_ >= 4 && data_[0] == 'J' && data_[1] == 'M' &&
           data_[2] == 'P' && data_[3] == 'X';
  }
};

struct model_result {
  bool has_value;
  int value;
  jmpxx::error err;
};

inline void compare(const jmpxx::result<int, jmpxx::error>& r,
                    const model_result& m) {
  if (r.has_value() != m.has_value) reject("transport oracle state diverged");
  if (m.has_value) {
    if (r.value() != m.value) reject("transport oracle value diverged");
  } else {
    if (r.error() != m.err) reject("transport oracle error diverged");
  }
}

[[nodiscard]] inline int wrapped_add(int a, int b) noexcept {
  return static_cast<int>(static_cast<unsigned>(a) + static_cast<unsigned>(b));
}

inline void exercise_transport(bytes& in) {
  jmpxx::result<int, jmpxx::error> r = 0;
  model_result m{true, 0, jmpxx::error{}};

  for (int i = 0; i < 128; ++i) {
    switch (in.take() % 10) {
      case 0: {
        int v = in.take_int();
        r = v;
        m = {true, v, {}};
        break;
      }
      case 1: {
        jmpxx::error e(in.take_int(), static_cast<int>(in.take() % 17));
        r = jmpxx::fail(e);
        m = {false, 0, e};
        break;
      }
      case 2: {
        jmpxx::result<int, jmpxx::error> c = r;
        compare(c, m);
        r = c;
        break;
      }
      case 3: {
        jmpxx::result<int, jmpxx::error> moved =
            static_cast<jmpxx::result<int, jmpxx::error>&&>(r);
        compare(moved, m);
        r = moved;
        break;
      }
      case 4: {
        jmpxx::result<int, jmpxx::error> other = in.take_int();
        r = other;
        m = {true, other.value(), {}};
        break;
      }
      case 5: {
        jmpxx::error e(in.take_int(), static_cast<int>(in.take() % 17));
        jmpxx::result<int, jmpxx::error> other = jmpxx::fail(e);
        r = other;
        m = {false, 0, e};
        break;
      }
      case 6: {
        int fallback = in.take_int();
        int got = r.value_or(fallback);
        int want = m.has_value ? m.value : fallback;
        if (got != want) reject("value_or diverged from oracle");
        break;
      }
      case 7: {
        jmpxx::error fallback(in.take_int(), static_cast<int>(in.take() % 17));
        jmpxx::error got = r.error_or(fallback);
        jmpxx::error want = m.has_value ? fallback : m.err;
        if (got != want) reject("error_or diverged from oracle");
        break;
      }
      case 8: {
        auto mapped = r.transform([](int v) { return wrapped_add(v, 1); });
        model_result mm = m.has_value ? model_result{true, wrapped_add(m.value, 1), {}}
                                      : model_result{false, 0, m.err};
        compare(mapped, mm);
        break;
      }
      default: {
        auto recovered = r.or_else([](jmpxx::error e) {
          return jmpxx::result<int, jmpxx::error>(wrapped_add(e.code, e.domain));
        });
        model_result mm =
            m.has_value ? m : model_result{true, wrapped_add(m.err.code, m.err.domain), {}};
        compare(recovered, mm);
        break;
      }
    }
    compare(r, m);
  }
}

inline jmpxx::result<int, jmpxx::rich_error> diag_leaf(int x) {
  if (x < 0) return jmpxx::fail(jmpxx::rich_error(11, 4));
  return x;
}
inline jmpxx::result<int, jmpxx::rich_error> diag_a(int x) {
  JMPXX_TRY(v, diag_leaf(x));
  return v + 1;
}
inline jmpxx::result<int, jmpxx::rich_error> diag_b(int x) {
  JMPXX_TRY(v, diag_a(x));
  return v + 1;
}
inline jmpxx::result<int, jmpxx::rich_error> diag_chain(int x) {
  JMPXX_TRY(v, diag_b(x));
  return v + 1;
}

inline void exercise_diagnostics(bytes& in) {
#if JMPXX_DIAGNOSTICS_ENABLED
  jmpxx::landing root;
  jmpxx::result<int, jmpxx::rich_error> bad = diag_chain(-1);
  if (bad.has_value()) reject("diagnostic chain unexpectedly succeeded");
  jmpxx::diagnostic::context c = jmpxx::diagnostic::inspect(bad.error());
  if (!c.available || c.hop_count != 3)
    reject("diagnostic chain did not capture each hop");
#if JMPXX_STACKTRACE
  if (jmpxx::platform::trace_available() && !c.trace_captured)
    reject("stack trace backend was available but no trace was captured");
#endif

  jmpxx::rich_error e(77, 9);
  int hops = jmpxx::diagnostic::max_chain + static_cast<int>(in.take() % 8) + 1;
  for (int i = 0; i < hops; ++i) jmpxx::detail::note_propagation(e);
  jmpxx::diagnostic::context full = jmpxx::diagnostic::inspect(e);
  if (!full.available || !full.hops_truncated ||
      full.hop_count != jmpxx::diagnostic::max_chain)
    reject("diagnostic hop truncation failed");

  for (int i = 0; i < jmpxx::diagnostic::max_inflight + 4; ++i) {
    jmpxx::rich_error overflow(i, 3);
    (void)overflow;
  }
#else
  (void)in;
#endif
}

enum class fuzz_status { ok = 0, denied = 3, timeout = 7 };

inline void exercise_erased_and_reflect(bytes& in) {
  static constexpr struct component_domain final : jmpxx::error_domain {
    const char* name() const noexcept override { return "adversarial.component"; }
    const char* message(int v) const noexcept override {
      return v == 5 ? "five" : "component error";
    }
  } component{};

  jmpxx::erased_error custom(5, component);
  if (!custom.in_domain(component) || custom.in_domain(jmpxx::generic_domain()))
    reject("erased domain identity failed");
  if (std::string_view(custom.domain_name()) != "adversarial.component" ||
      std::string_view(custom.message()) != "five")
    reject("erased domain dispatch failed");

  jmpxx::erased_error folded(in.take_int(), in.take_int());
  if (!folded.in_domain(jmpxx::generic_domain()))
    reject("generic erased fold left the generic domain");
  (void)folded.value();

  fuzz_status st = (in.take() & 1) ? fuzz_status::timeout : fuzz_status::denied;
  jmpxx::erased_error reflected = jmpxx::reflect::as_erased(st);
  if (std::string_view(reflected.domain_name()) != "fuzz_status")
    reject("reflection-derived domain name changed");
  if (st == fuzz_status::timeout &&
      std::string_view(reflected.message()) != "timeout")
    reject("reflection-derived enumerator message changed");
}

inline void exercise_interop(bytes& in, const std::uint8_t* data,
                             std::size_t size) {
  jmpxx::result<boundary::wire_error, jmpxx::error> decoded =
      boundary::decode(reinterpret_cast<const unsigned char*>(data), size);
  if (decoded.has_value()) {
    const boundary::wire_error& w = decoded.value();
    jmpxx::error e(w.code, w.domain);
    std::error_code ec = jmpxx::to_error_code(e);
    if (!jmpxx::is_jmpxx(ec) || jmpxx::from_error_code(ec) != e)
      reject("std::error_code round-trip diverged");
#if JMPXX_INTEROP_HAS_EXPECTED
    jmpxx::result<int, jmpxx::error> rv(w.code);
    if (jmpxx::from_expected(jmpxx::to_expected(rv)).value() != w.code)
      reject("std::expected value round-trip diverged");
    jmpxx::result<int, jmpxx::error> re = jmpxx::fail(e);
    if (jmpxx::from_expected(jmpxx::to_expected(re)).error() != e)
      reject("std::expected error round-trip diverged");
#endif
  } else {
    std::error_code ec = jmpxx::to_error_code(decoded.error());
    (void)jmpxx::from_error_code(ec);
  }

  std::optional<int> maybe =
      (in.take() & 1) ? std::optional<int>(in.take_int()) : std::nullopt;
  jmpxx::result<int, jmpxx::error> adapted =
      jmpxx::from_optional<jmpxx::error>(maybe, [] { return jmpxx::error(71); });
  if (maybe) {
    if (!adapted.has_value() || adapted.value() != *maybe)
      reject("optional adapter value diverged");
  } else if (adapted.has_value() || adapted.error().code != 71) {
    reject("optional adapter failure diverged");
  }
}

inline void exercise(const std::uint8_t* data, std::size_t size) {
  bytes in(data, size);
#if defined(JMPXX_ADVERSARIAL_INJECT_DEFECT) && JMPXX_ADVERSARIAL_INJECT_DEFECT
  if (in.magic()) reject("injected fuzz defect reached");
#endif
  exercise_transport(in);
  exercise_diagnostics(in);
  exercise_erased_and_reflect(in);
  exercise_interop(in, data, size);
}

}  // namespace adversarial

#endif  // JMPXX_TEST_PORTABLE_FUZZ_DRIVER_HPP
