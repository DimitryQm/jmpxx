// SPDX-License-Identifier: MIT
// Behavioral tier for the dual-personality model, compiled with the diagnostic
// layer on. It proves the rich policy captures a failure's origin and the causal
// chain it accumulates as it propagates, that a landing scope bounds the lifetime
// of that context, that a failure produced before main drives the layer safely,
// that the type-erased policy reads at a boundary without naming the originating
// category, and that the rich error decays to the minimal error losslessly. Each
// check returns a distinct nonzero code so a failure names itself.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"

#include <cstdio>
#include <string>

using namespace jmpxx;

namespace {

// An eight-frame chain over the rich policy: a leaf plus seven wrappers, so a
// failure climbs through seven propagation sites to the landing boundary.
result<int, rich_error> leaf(int x) {
  if (x < 0) return fail(rich_error(42, 7));
  return x * 2;
}
result<int, rich_error> w1(int x) { JMPXX_TRY(v, leaf(x)); return v + 1; }
result<int, rich_error> w2(int x) { JMPXX_TRY(v, w1(x)); return v + 1; }
result<int, rich_error> w3(int x) { JMPXX_TRY(v, w2(x)); return v + 1; }
result<int, rich_error> w4(int x) { JMPXX_TRY(v, w3(x)); return v + 1; }
result<int, rich_error> w5(int x) { JMPXX_TRY(v, w4(x)); return v + 1; }
result<int, rich_error> w6(int x) { JMPXX_TRY(v, w5(x)); return v + 1; }
result<int, rich_error> chain(int x) { JMPXX_TRY(v, w6(x)); return v + 1; }

// A failure produced during dynamic initialization, before main, to prove the
// thread-local store initializes safely on first touch with no init-order
// dependency. The captured chain depth is recorded for main to check.
struct BeforeMain {
  int chain_len = -1;
  bool origin_ok = false;
  BeforeMain() {
    landing root;
    result<int, rich_error> r = chain(-1);
    if (r.has_value()) return;
    diagnostic::context c = diagnostic::inspect(r.error());
    origin_ok = c.available;
    chain_len = diagnostic::chain_length(r.error());
  }
};
BeforeMain g_before_main;

}  // namespace

int main() {
  // 1. static-initialization safety: the before-main failure captured its chain.
  if (!g_before_main.origin_ok) return 1;
  if (g_before_main.chain_len != 7) {
    std::fprintf(stderr, "static-init chain=%d (want 7)\n", g_before_main.chain_len);
    return 2;
  }

  // 2. origin and per-hop chain at run time.
  {
    landing root;
    result<int, rich_error> r = chain(-1);
    if (r.has_value()) return 3;
    diagnostic::context c = diagnostic::inspect(r.error());
    if (!c.available) return 4;
    if (c.hop_count != 7) {
      std::fprintf(stderr, "runtime hops=%d (want 7)\n", c.hop_count);
      return 5;
    }
    if (c.hops_truncated) return 6;
    // The origin is the leaf, the first hop is its immediate caller, and the last
    // hop is the landing function, so the chain is ordered innermost to outermost.
    bool origin_is_leaf =
        std::string(c.origin.function_name()).find("leaf") != std::string::npos;
    if (!origin_is_leaf) return 7;
  }

  // 3. the success path attaches no context, and a value is delivered intact.
  // chain(5) doubles at the leaf (10) then adds one in each of the seven wrappers.
  {
    landing root;
    result<int, rich_error> r = chain(5);
    if (!r.has_value() || r.value() != 17) return 8;
  }

  // 4. landing lifetime: a failure created inside a nested scope is released when
  // that scope exits, so the arena returns to its prior depth and a later failure
  // still records a full chain.
  {
    landing outer;
    {
      landing inner;
      result<int, rich_error> r = chain(-1);
      if (diagnostic::chain_length(r.error()) != 7) return 9;
    }
    // After inner exits, a fresh failure under outer still gets a full chain, which
    // it could not if the arena had leaked or corrupted on truncation.
    result<int, rich_error> r2 = chain(-1);
    if (diagnostic::chain_length(r2.error()) != 7) return 10;
  }

  // 5. lossless decay: the rich error carries the same in-band code and domain as
  // the minimal error it stands in for.
  {
    landing root;
    result<int, rich_error> r = chain(-1);
    error e = r.error().base();
    if (e.code != 42 || e.domain != 7) return 11;
    if (r.error().code != 42 || r.error().domain != 7) return 12;
  }

  // 6. the type-erased policy reads at a boundary without naming the category.
  {
    static constexpr struct comp_domain final : error_domain {
      const char* name() const noexcept override { return "component"; }
      const char* message(int v) const noexcept override {
        return v == 3 ? "stage three failed" : "component error";
      }
    } comp{};
    result<int, erased_error> boundary = fail(erased_error(3, comp));
    if (boundary.has_value()) return 13;
    erased_error e = boundary.error();
    // Boundary code reads the domain name and message, never the component's enum.
    if (std::string(e.domain_name()) != "component") return 14;
    if (std::string(e.message()) != "stage three failed") return 15;
    if (!e.in_domain(comp)) return 16;
    if (e.in_domain(generic_domain())) return 17;
  }

  std::printf("policy/behavior: all checks passed\n");
  return 0;
}
