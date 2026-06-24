// SPDX-License-Identifier: MIT
// config_validate: read a TOML configuration, validate its [server], [limits], and
// [policy] sections, and report the first problem. Every fallible step returns a
// result and propagates its failure with JMPXX_TRY to one landing boundary in main.
//
// The program composes three error representations over one transport and one
// propagation construct, and two real third-party libraries. The [server] pipeline
// uses the application's selected error policy (app::error), minimal or rich, so the
// same source compiles under either and a deep failure surfaces its origin and chain
// in the rich build. The [limits] section is validated by a component that reports
// type-erased errors from its own domain. The [policy] section is an access policy
// expressed as JSON, parsed by glaze, whose std::expected result is converted into a
// jmpxx result through the interop bridge.
//
// The boundaries where toml++ reports absence, a std::optional from a field lookup
// and a parse_result from parsing, are bridged into jmpxx with the from_optional and
// from_condition adapters rather than a hand-written branch at each site. The program
// is built with exceptions and RTTI disabled; toml++ runs in its no-exceptions mode
// and glaze returns std::expected without throwing.
#include "app_policy.hpp"
#include "limits_check.hpp"
#include "policy_check.hpp"

#include <jmpxx/interop/adapt.hpp>

#include <toml++/toml.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

namespace {

// Error domains and codes for the [server] pipeline. These travel in the app::error
// code and domain fields under either policy.
enum domain { dom_parse = 1, dom_schema = 2 };
enum schema_code { missing = 1, wrong_type = 2, out_of_range = 3 };

struct config {
  std::string host;
  int port;
  int workers;
};

// The functions below are identical source under the minimal and the rich policy.
// Only the error type the app::error alias selects differs.

// toml++'s parse_result is boolean-testable and yields its table on success, so
// from_condition adapts it: on success the table is moved out, on failure the parse
// error's line becomes the app error. One call replaces the if/return boundary.
app::result<toml::table> load(std::string_view path) {
  toml::parse_result parsed = toml::parse_file(path);
  bool ok = static_cast<bool>(parsed);
  int line = ok ? 0 : static_cast<int>(parsed.error().source().begin.line);
  return jmpxx::from_condition<app::error>(
      ok, [&] { return std::move(parsed).table(); },
      [&] { return app::error(line, dom_parse); });
}

// A field lookup returns a std::optional, which from_optional adapts directly: the
// value when present, the supplied failure when absent.
app::result<std::string> field_string(const toml::table& t,
                                      std::string_view section,
                                      std::string_view key) {
  return jmpxx::from_optional<app::error>(
      t[section][key].value<std::string>(),
      [] { return app::error(missing, dom_schema); });
}

app::result<int> field_int(const toml::table& t, std::string_view section,
                           std::string_view key, int lo, int hi) {
  JMPXX_TRY(v, jmpxx::from_optional<app::error>(
                   t[section][key].value<std::int64_t>(),
                   [] { return app::error(missing, dom_schema); }));
  if (v < lo || v > hi) return jmpxx::fail(app::error(out_of_range, dom_schema));
  return static_cast<int>(v);
}

app::result<config> validate(const toml::table& t) {
  JMPXX_TRY(host, field_string(t, "server", "host"));
  JMPXX_TRY(port, field_int(t, "server", "port", 1, 65535));
  JMPXX_TRY(workers, field_int(t, "server", "workers", 1, 1024));
  return config{host, port, workers};
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: config_validate <file.toml>\n");
    return 2;
  }
  // The landing scope owns the diagnostic context of any failure produced under it.
  // In the minimal policy it is an empty object; the declaration is the same.
  app::landing root;

  app::result<toml::table> table = load(argv[1]);
  if (!table) {
    std::printf("could not load %s\n", argv[1]);
    app::report(table.error(), stdout);
    return 1;
  }

  app::result<config> cfg = validate(*table);
  if (!cfg) {
    std::printf("invalid [server] section\n");
    app::report(cfg.error(), stdout);
    return 1;
  }

  // The [limits] component returns a type-erased error. The boundary reads its
  // domain name, message, and value without naming limits::code.
  jmpxx::result<limits::config, jmpxx::erased_error> lim = limits::check(*table);
  if (!lim) {
    jmpxx::erased_error e = lim.error();
    std::printf("invalid [limits] section\n");
    std::printf("error in domain '%s': %s (value %d)\n", e.domain_name(),
                e.message(), e.value());
    return 1;
  }

  // The [policy] section is JSON parsed by glaze, a third-party library that returns
  // std::expected. The expected bridge adopts that result into jmpxx, so the policy
  // boundary carries glaze's own error type, read here without jmpxx knowing glaze's
  // internals. First the JSON text is read from the config with the same adapter.
  app::result<std::string> policy_json = field_string(*table, "policy", "access");
  if (!policy_json) {
    std::printf("missing [policy].access\n");
    app::report(policy_json.error(), stdout);
    return 1;
  }
  jmpxx::result<policy::access, glz::error_ctx> pol = policy::parse(*policy_json);
  if (!pol) {
    std::string detail = glz::format_error(pol.error(), *policy_json);
    std::printf("invalid [policy] access JSON: %s\n", detail.c_str());
    return 1;
  }

  std::printf(
      "valid config: host=%s port=%d workers=%d max_connections=%d timeout_ms=%d "
      "policy.mode=%s policy.max_sessions=%d policy.allow=%zu rules\n",
      cfg.value().host.c_str(), cfg.value().port, cfg.value().workers,
      lim.value().max_connections, lim.value().timeout_ms, pol.value().mode.c_str(),
      pol.value().max_sessions, pol.value().allow.size());
  return 0;
}
