// SPDX-License-Identifier: MIT
// Behavioral tier for the reflection forward layer. Building and running this file
// exercises every derived-metadata surface, and it is built on both a C++20 toolchain
// (the hand-written fallback) and a reflection-capable one (the reflection path) so a
// run on each proves the two produce the same results. The expected values below are
// the metadata of the test enum; whichever path computed them, they must match. The
// program prints what it observed and exits non-zero on any mismatch.
#include <jmpxx/core.hpp>
#include <jmpxx/erased.hpp>
#include <jmpxx/reflect.hpp>

#include <cstdio>
#include <optional>
#include <string_view>

using namespace jmpxx;

namespace app {
// A component's error enum, sparse and not declared in value order, so the test
// exercises unmatched lookup and the value-ordered failure set rather than a trivial
// 0..n run.
enum class status { ok = 0, denied = 7, not_found = 2, timeout = 5 };
}  // namespace app

namespace {
int failures = 0;
void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++failures;
  }
}
bool sv_eq(std::string_view a, const char* b) { return a == std::string_view(b); }
}  // namespace

int main() {
  using app::status;

  // Enumerator to text, including the unmatched case.
  check(sv_eq(reflect::enum_name(status::not_found), "not_found"), "enum_name not_found");
  check(sv_eq(reflect::enum_name(status::timeout), "timeout"), "enum_name timeout");
  check(sv_eq(reflect::enum_name(status::ok), "ok"), "enum_name ok");
  check(reflect::enum_name(static_cast<status>(99)).empty(), "enum_name unmatched is empty");

  // Text to enumerator.
  check(reflect::enum_cast<status>("denied") == std::optional<status>(status::denied),
        "enum_cast denied");
  check(reflect::enum_cast<status>("ok") == std::optional<status>(status::ok),
        "enum_cast ok");
  check(!reflect::enum_cast<status>("nonesuch").has_value(), "enum_cast unknown is empty");

  // Type name and count.
  check(sv_eq(reflect::type_name<status>(), "status"), "type_name");
  check(reflect::enum_count<status>() == 4, "enum_count");

  // The static, value-ordered failure set: ok=0, not_found=2, timeout=5, denied=7.
  auto fs = reflect::failures<status>();
  check(fs.size() == 4, "failures size");
  if (fs.size() == 4) {
    check(fs[0].value == 0 && sv_eq(fs[0].name, "ok"), "failures[0] ok");
    check(fs[1].value == 2 && sv_eq(fs[1].name, "not_found"), "failures[1] not_found");
    check(fs[2].value == 5 && sv_eq(fs[2].name, "timeout"), "failures[2] timeout");
    check(fs[3].value == 7 && sv_eq(fs[3].name, "denied"), "failures[3] denied");
  }

  // The derived error_domain: a type-erased error keyed by the enum reports the
  // enumerator name as its message with no hand-written table.
  erased_error e = reflect::as_erased(status::timeout);
  check(sv_eq(e.domain_name(), "status"), "erased domain name");
  check(sv_eq(e.message(), "timeout"), "erased message timeout");
  check(e.value() == 5, "erased value");
  erased_error unknown = reflect::as_erased(static_cast<status>(42));
  check(sv_eq(unknown.message(), "unknown error"), "erased message unknown");

  // Constant-evaluable: the derived metadata is usable at compile time.
  static_assert(reflect::enum_name(status::ok) == std::string_view("ok"));
  static_assert(reflect::enum_count<status>() == 4);

  std::printf("type_name=%.*s count=%zu reflection=%d\n",
              (int)reflect::type_name<status>().size(),
              reflect::type_name<status>().data(), reflect::enum_count<status>(),
              JMPXX_REFLECTION);
  for (auto f : reflect::failures<status>())
    std::printf("  failure value=%d name=%.*s\n", f.value, (int)f.name.size(),
                f.name.data());
  std::printf("as_erased(timeout): domain=%s message=%s value=%d\n", e.domain_name(),
              e.message(), e.value());
  std::printf("%s\n", failures == 0 ? "REFLECT TEST PASSED" : "REFLECT TEST FAILED");
  return failures == 0 ? 0 : 1;
}
