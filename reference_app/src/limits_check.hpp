// SPDX-License-Identifier: MIT
// A self-contained component that validates the [limits] section of the config and
// reports failures as type-erased errors from its own domain. The orchestrator in
// main reads the domain name, the message, and the value without ever naming this
// component's error codes, which is the type-erased policy at a component boundary:
// the boundary carries a domain-tagged error usable by code that neither knows nor
// cares about the originating category. The component is independent of the
// application's chosen error policy, so the [server] pipeline can run under the
// minimal or the rich policy while this boundary stays type-erased.
#ifndef JMPXX_APP_LIMITS_CHECK_HPP
#define JMPXX_APP_LIMITS_CHECK_HPP

#include <jmpxx/core.hpp>
#include <jmpxx/erased.hpp>

#include <toml++/toml.hpp>

#include <cstdint>

namespace limits {

// This component's own error codes. They never cross the boundary as themselves;
// the orchestrator sees only the erased form.
enum code { missing_section = 1, missing_key = 2, out_of_range = 3 };

// The component's error family. One static descriptor names the family and renders
// each code, reached through virtual dispatch with no RTTI and no allocation.
struct domain final : jmpxx::error_domain {
  const char* name() const noexcept override { return "limits"; }
  const char* message(int v) const noexcept override {
    switch (v) {
      case missing_section: return "the [limits] section is absent";
      case missing_key: return "a required limit is missing";
      case out_of_range: return "a limit is outside its allowed range";
    }
    return "limits error";
  }
};
inline constexpr domain instance{};

struct config {
  int max_connections;
  int timeout_ms;
};

// Validate [limits], returning either the parsed limits or a type-erased error in
// this component's domain. The propagation construct is the same JMPXX_TRY the
// application uses elsewhere; only the error representation differs.
inline jmpxx::result<config, jmpxx::erased_error> check(const toml::table& t) {
  auto section = t["limits"];
  if (!section.is_table())
    return jmpxx::fail(jmpxx::erased_error(missing_section, instance));

  auto read = [&](const char* key, int lo,
                  int hi) -> jmpxx::result<int, jmpxx::erased_error> {
    auto v = section[key].template value<std::int64_t>();
    if (!v) return jmpxx::fail(jmpxx::erased_error(missing_key, instance));
    if (*v < lo || *v > hi)
      return jmpxx::fail(jmpxx::erased_error(out_of_range, instance));
    return static_cast<int>(*v);
  };

  JMPXX_TRY(max_connections, read("max_connections", 1, 1000000));
  JMPXX_TRY(timeout_ms, read("timeout_ms", 1, 600000));
  return config{max_connections, timeout_ms};
}

}  // namespace limits

#endif  // JMPXX_APP_LIMITS_CHECK_HPP
