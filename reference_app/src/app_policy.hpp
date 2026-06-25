// SPDX-License-Identifier: MIT
// The application's error policy, selected once at build time. APP_POLICY_RICH
// picks the rich policy and the diagnostic layer; otherwise the minimal policy is
// used. Everything below the aliases here, and every call site in the application,
// is identical source under either policy: the build flag is the only difference.
// The application-side half of the dual-personality guarantee keeps the two builds on
// the same source file, so a diff between them is empty.
#ifndef JMPXX_APP_POLICY_HPP
#define JMPXX_APP_POLICY_HPP

#include <jmpxx/core.hpp>

#include <cstdio>

#if defined(APP_POLICY_RICH)
#include <jmpxx/diagnostics.hpp>
#endif

namespace app {

#if defined(APP_POLICY_RICH)
using error = jmpxx::rich_error;
using landing = jmpxx::landing;
#else
using error = jmpxx::error;
// In the minimal policy there is no diagnostic scope to own, so the landing is an
// empty object that the same main() can still declare.
struct landing {};
#endif

// The application's result type. Call sites write app::result<T>, which selects the
// policy's error without naming it.
template <class T>
using result = jmpxx::result<T, error>;

// Report a failed result's error to out. Both policies print the bare code and
// domain; the rich policy additionally prints where the failure began and the path
// it propagated, which is the context the minimal policy does not carry. The
// branch lives here, not at any call site.
inline void report(const error& e, std::FILE* out) {
  std::fprintf(out, "error: code=%d domain=%d\n", e.code, e.domain);
#if defined(APP_POLICY_RICH)
  jmpxx::diagnostic::print(e, out);
#endif
}

}  // namespace app

#endif  // JMPXX_APP_POLICY_HPP
