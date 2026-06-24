// SPDX-License-Identifier: MIT
// The optional reflection forward layer. It derives error metadata from an enum so a
// program does not hand-maintain it: the enumerator name from a value and back, the
// static set of failures the enum declares, and an error_domain whose per-value message
// is the enumerator name. The same code builds on a C++20 toolchain, where a
// hand-written path computes the metadata, and on a reflection-capable one, where C++26
// reflection does, with identical results.
#include <jmpxx/core.hpp>
#include <jmpxx/erased.hpp>
#include <jmpxx/reflect.hpp>

#include <cstdio>

namespace net {
// A component's failure enum. Defining it is all the program writes; the names, the
// failure set, and the domain message are derived from it.
enum class status { ok = 0, refused = 3, unreachable = 5, timed_out = 8 };
}  // namespace net

int main() {
  using net::status;
  std::printf("reflection path: %s\n", JMPXX_REFLECTION ? "C++26" : "C++20 fallback");

  // Enumerator name from a value, and the value back from a name.
  auto v = jmpxx::reflect::enum_name(status::unreachable);
  std::printf("name(unreachable) = %.*s\n", (int)v.size(), v.data());
  auto parsed = jmpxx::reflect::enum_cast<status>("timed_out");
  std::printf("cast(\"timed_out\") = %d\n", parsed ? (int)*parsed : -1);

  // The static failure set: what a unit reporting this enum can fail with, derived.
  std::printf("status declares %zu failures:\n", jmpxx::reflect::enum_count<status>());
  for (auto f : jmpxx::reflect::failures<status>())
    std::printf("  %d -> %.*s\n", f.value, (int)f.name.size(), f.name.data());

  // A type-erased error keyed by the enum: the boundary reads the enumerator name as
  // the message with no hand-written table in the component.
  jmpxx::erased_error e = jmpxx::reflect::as_erased(status::refused);
  std::printf("erased: domain=%s message=%s value=%d\n", e.domain_name(), e.message(),
              e.value());
  return 0;
}
