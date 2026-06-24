// SPDX-License-Identifier: MIT
// A self-contained component that validates the [limits] section of the config and
// reports failures as type-erased errors. It derives the error domain from its own enum
// with the reflection forward layer instead of hand-writing a name table: the boundary
// in main reads a domain name and a per-value message that come from the enum's type
// name and enumerator names, with nothing to maintain as the enum grows. The
// orchestrator still reads the erased error uniformly, never naming this component's
// enum, so the boundary is unchanged; only the source of the metadata is.
//
// On a C++20 toolchain the reflection layer's hand-written fallback derives the names by
// parsing the compiler signature; on a reflection-capable toolchain C++26 reflection
// does, with identical results. The component is independent of the application's chosen
// error policy, so the [server] pipeline can run minimal or rich while this boundary
// stays type-erased.
#ifndef JMPXX_APP_LIMITS_CHECK_HPP
#define JMPXX_APP_LIMITS_CHECK_HPP

#include <jmpxx/core.hpp>
#include <jmpxx/erased.hpp>
#include <jmpxx/reflect.hpp>

#include <toml++/toml.hpp>

#include <cstdint>

namespace limits {

// The component's failure enum. With the reflection layer its enumerator names become
// the erased error's messages and its type name becomes the domain name, so the enum is
// named for what the boundary should read; a component that wants a custom domain name
// or prose messages writes its own error_domain instead.
enum class limit_error { missing_section = 1, missing_key = 2, out_of_range = 3 };

struct config {
  int max_connections;
  int timeout_ms;
};

// Validate [limits], returning either the parsed limits or a type-erased error derived
// from limit_error. reflect::as_erased tags the value with the enum's derived domain, so
// there is no name switch to maintain here; the propagation construct is the same
// JMPXX_TRY the application uses elsewhere, and only the error representation differs.
inline jmpxx::result<config, jmpxx::erased_error> check(const toml::table& t) {
  auto section = t["limits"];
  if (!section.is_table())
    return jmpxx::fail(jmpxx::reflect::as_erased(limit_error::missing_section));

  auto read = [&](const char* key, int lo,
                  int hi) -> jmpxx::result<int, jmpxx::erased_error> {
    auto v = section[key].template value<std::int64_t>();
    if (!v) return jmpxx::fail(jmpxx::reflect::as_erased(limit_error::missing_key));
    if (*v < lo || *v > hi)
      return jmpxx::fail(jmpxx::reflect::as_erased(limit_error::out_of_range));
    return static_cast<int>(*v);
  };

  JMPXX_TRY(max_connections, read("max_connections", 1, 1000000));
  JMPXX_TRY(timeout_ms, read("timeout_ms", 1, 600000));
  return config{max_connections, timeout_ms};
}

}  // namespace limits

#endif  // JMPXX_APP_LIMITS_CHECK_HPP
