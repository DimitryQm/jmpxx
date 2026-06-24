// SPDX-License-Identifier: MIT
// config_resolve: read a TOML configuration, resolve every value in its [values] table
// by recursively interpolating ${other} references, and report each resolved value or
// the first failure that resolving it hit. It is the reference application's opt-in use
// of the experimental non-local unwind arm: a failure found deep in the recursion
// escapes to a single landing in the resolver while the recursion's intermediate frames
// carry no error-threading construct, and the resolver's state stays clean across the
// escape so the next key still resolves.
//
// Unlike config_validate, which is built for the exception-free niche, this program is
// built with exception cleanup tables, because the arm runs destructors during a forced
// unwind only where they are present. The program consumes jmpxx through find_package and
// parses with toml++. Removing jmpxx removes the escape and the landing and nothing
// resolves; removing toml++ removes the configuration there is to resolve.
#include "resolve.hpp"

#include <toml++/toml.hpp>

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: config_resolve <file.toml>\n");
    return 2;
  }
  toml::parse_result parsed = toml::parse_file(argv[1]);
  if (!parsed) {
    std::printf("could not parse %s: %s\n", argv[1],
                std::string(parsed.error().description()).c_str());
    return 1;
  }
  toml::table& root = parsed.table();
  toml::table* values = root["values"].as_table();
  if (!values) {
    std::printf("%s: no [values] table to resolve\n", argv[1]);
    return 1;
  }

  resolve::resolver r(*values);
  int failures = 0;
  // Resolve every key in order. The resolver is reused across keys on purpose: a clean
  // resolution after a failed one is the visible proof that the escape unwound the
  // cycle-detection guards and left no residue, which a bare longjmp would not.
  for (auto&& [key, node] : *values) {
    (void)node;
    std::string name(key.str());
    jmpxx::result<std::string, jmpxx::error> resolved = r.resolve_key(key.str());
    if (resolved) {
      std::printf("  %-12s = %s\n", name.c_str(), resolved.value().c_str());
    } else {
      ++failures;
      std::printf("  %-12s : FAILED, %s (at key '%s', depth %d)\n", name.c_str(),
                  resolve::kind_text(resolved.error().code), r.error_key().c_str(),
                  resolved.error().domain);
    }
  }
  std::printf("%s: %zu value(s), %d failure(s)\n", argv[1], values->size(), failures);
  return failures == 0 ? 0 : 1;
}
