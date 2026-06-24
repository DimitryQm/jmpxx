// SPDX-License-Identifier: MIT
// No-RTTI cell. The type-erased policy reaches its domain descriptors through
// virtual functions, which work with RTTI disabled because only typeid and
// dynamic_cast need it. This file builds and runs the whole engine, including the
// erased policy's virtual dispatch and the rich policy, under -fno-rtti
// -fno-exceptions, and returns nonzero from the first check that fails.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"

#include <cstdio>

using namespace jmpxx;

namespace {

enum net_code { timeout = 1, refused = 2 };

struct net_domain final : error_domain {
  const char* name() const noexcept override { return "net"; }
  const char* message(int v) const noexcept override {
    switch (v) {
      case timeout: return "connection timed out";
      case refused: return "connection refused";
    }
    return "network error";
  }
};
constexpr net_domain net{};

result<int, erased_error> dial(int host) {
  if (host == 0) return fail(erased_error(refused, net));
  if (host < 0) return fail(erased_error(timeout, net));
  return host;
}

result<int, erased_error> session(int host) {
  JMPXX_TRY(fd, dial(host));  // identical propagation construct, erased policy
  return fd + 100;
}

}  // namespace

int main() {
  // Virtual dispatch through the domain descriptor resolves the right family and
  // message with RTTI disabled.
  result<int, erased_error> a = session(0);
  if (a.has_value()) return 1;
  erased_error e = a.error();
  if (e.value() != refused) return 2;
  if (e.domain_name()[0] != 'n') return 3;  // "net"
  const char* msg = e.message();
  if (msg[0] != 'c') return 4;  // "connection refused"
  if (!e.in_domain(net)) return 5;

  result<int, erased_error> b = session(5);
  if (!b.has_value() || b.value() != 105) return 6;

  // The rich policy also builds and runs under -fno-rtti.
  landing root;
  result<int, rich_error> r = fail(rich_error(9));
  if (r.has_value() || r.error().code != 9) return 7;

  std::printf("policy/erased_nortti: virtual dispatch and rich policy OK under -fno-rtti\n");
  return 0;
}
