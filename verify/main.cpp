// SPDX-License-Identifier: MIT
// jmpxx-verify: the verification harness. It exposes every observable property
// of the core through one command-line surface with a human and a structured
// (JSON) output mode. Runtime probes measure transport size against a
// no-overhead budget, count allocations on the paths declared allocation-free,
// count destructors across a deep propagation, exercise the transport semantics,
// and report the propagation level costs. The codegen probe compiles a fixture
// with a pinned compiler, normalizes the emitted assembly, and compares it to a
// committed golden while reporting instruction count and stack-spill presence.
//
// Usage: jmpxx-verify <command> [--format=json]
//   size | alloc | destructors | semantics | levels | all
//   codegen --fixture <f> --symbol <s> --golden <g> [--arch x86_64|aarch64]
//           [--forbid-spill] [--max-insns N] [--update]
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"
#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/exception.hpp"
#include "jmpxx/interop/expected.hpp"
#include "jmpxx/platform.hpp"
#include "jmpxx/unwind.hpp"
#include "reporter.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jmpxx;
using jv::Fmt;
using jv::Report;

// Allocation counting. The global operator new is replaced so a probe can prove
// a path performs no heap allocation. Counting is off except inside the measured
// region, so the harness's own allocations are not counted.
namespace {
long long g_allocs = 0;
bool g_count = false;
}  // namespace

void* operator new(std::size_t n) {
  if (g_count) ++g_allocs;
  void* p = std::malloc(n ? n : 1);
  if (!p) std::abort();
  return p;
}
void* operator new[](std::size_t n) {
  if (g_count) ++g_allocs;
  void* p = std::malloc(n ? n : 1);
  if (!p) std::abort();
  return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

// An 8-frame propagation chain (leaf + 7 wrappers) shared by several probes.
result<int, error> leaf(int x) {
  if (x < 0) return fail(error(42));
  return x * 2;
}
result<int, error> w1(int x) { JMPXX_TRY(v, leaf(x)); return v + 1; }
result<int, error> w2(int x) { JMPXX_TRY(v, w1(x)); return v + 1; }
result<int, error> w3(int x) { JMPXX_TRY(v, w2(x)); return v + 1; }
result<int, error> w4(int x) { JMPXX_TRY(v, w3(x)); return v + 1; }
result<int, error> w5(int x) { JMPXX_TRY(v, w4(x)); return v + 1; }
result<int, error> w6(int x) { JMPXX_TRY(v, w5(x)); return v + 1; }
result<int, error> chain8(int x) { JMPXX_TRY(v, w6(x)); return v + 1; }

// One source body, parameterized by the error policy and naming no error type. The
// policies and diagnostics probes instantiate it for each policy, and the alloc
// probe runs it for the rich and erased policies, so a single chain proves that the
// policy is the only thing that changes across the representations.
template <class E>
result<int, E> pol_leaf(int x) {
  if (x < 0) return fail(E(42, 7));
  return x * 2;
}
template <class E>
result<int, E> pol_w1(int x) {
  JMPXX_TRY(v, pol_leaf<E>(x));
  return v + 1;
}
template <class E>
result<int, E> pol_chain(int x) {
  JMPXX_TRY(v, pol_w1<E>(x));
  return v + 1;
}

// size: the transport adds nothing beyond the discriminant and natural padding.
// The budget is the size of a hand-written union-plus-flag with the same payload.
template <class V, class E>
struct hand_written {
  union {
    V v;
    E e;
  };
  bool ok;
};

int probe_size(Fmt fmt) {
  Report r(fmt, "size");
  auto check = [&](const char* name, std::size_t got, std::size_t budget,
                   bool triv_inputs, bool triv_result) {
    r.num(std::string(name) + ".sizeof", static_cast<long long>(got));
    r.num(std::string(name) + ".budget", static_cast<long long>(budget));
    if (got > budget)
      r.fail(std::string(name) + ": transport exceeds the no-overhead budget");
    if (triv_inputs && !triv_result)
      r.fail(std::string(name) +
             ": trivially copyable inputs did not yield a trivially copyable "
             "transport");
  };
#define SIZE_CHECK(T, E)                                                     \
  do {                                                                       \
    using R = result<T, E>;                                                  \
    using V = std::conditional_t<std::is_void_v<T>, detail::unit, T>;        \
    check(#T "," #E, sizeof(R), sizeof(hand_written<V, E>),                  \
          std::is_trivially_copyable_v<V> && std::is_trivially_copyable_v<E>,\
          std::is_trivially_copyable_v<R>);                                  \
  } while (0)
  SIZE_CHECK(int, int);
  SIZE_CHECK(int, error);
  SIZE_CHECK(double, error);
  SIZE_CHECK(char, char);
  SIZE_CHECK(void, error);
#undef SIZE_CHECK
  return r.finish();
}

// alloc: construction, inspection, extraction, assignment, and propagation over
// trivial payloads perform no heap allocation.
int probe_alloc(Fmt fmt) {
  volatile long long sink = 0;
  g_allocs = 0;
  g_count = true;
  for (int i = 0; i < 1000; ++i) {
    result<int, error> a = i;
    result<int, error> b = fail(error(i));
    if (a) sink += a.value();
    if (!b) sink += b.error().code;
    result<int, error> ok = chain8(i);
    result<int, error> bad = chain8(-i - 1);
    sink += ok.value_or(0);
    sink += bad.has_value() ? 0 : 1;
    a = b;             // cross-state assignment (value -> error)
    b = i;             // cross-state assignment (error -> value)
    sink += a.has_value() ? 0 : b.value();

    // The no-heap promise holds under every policy, including the rich policy with
    // its diagnostic layer live: its out-of-band store is a fixed per-thread arena,
    // so capturing an origin and a causal chain allocates nothing. The landing
    // scope opening and closing is part of the measured region.
    {
      landing root;
      result<int, rich_error> rok = pol_chain<rich_error>(i);
      result<int, rich_error> rbad = pol_chain<rich_error>(-i - 1);
      sink += rok.value_or(0);
      sink += rbad.has_value() ? 0 : rbad.error().code;
    }
    result<int, erased_error> eok = pol_chain<erased_error>(i);
    result<int, erased_error> ebad = pol_chain<erased_error>(-i - 1);
    sink += eok.value_or(0);
    sink += ebad.has_value() ? 0 : ebad.error().value();
  }
  g_count = false;
  Report r(fmt, "alloc");
  r.num("allocations", g_allocs);
  r.num("iterations", 1000);
  r.num("diagnostics_enabled", JMPXX_DIAGNOSTICS_ENABLED);
  r.boolean("allocation_free", g_allocs == 0);
  if (g_allocs != 0)
    r.fail("a policy path allocated heap memory");
  (void)sink;
  return r.finish();
}

// destructors: a failure propagated through eight frames runs the destructor of
// every automatic object on the path exactly once. An instrumented type counts
// constructions and destructions; they must balance.
struct Counted {
  static long long ctor;
  static long long dtor;
  int v;
  explicit Counted(int x) : v(x) { ++ctor; }
  Counted(const Counted& o) : v(o.v) { ++ctor; }
  Counted(Counted&& o) noexcept : v(o.v) { ++ctor; }
  Counted& operator=(const Counted&) = default;
  Counted& operator=(Counted&&) = default;
  ~Counted() { ++dtor; }
};
long long Counted::ctor = 0;
long long Counted::dtor = 0;

result<int, error> d_leaf(int x) {
  Counted guard(x);
  if (x < 0) return fail(error(7));
  return guard.v * 2;
}
result<int, error> d1(int x) { Counted g(x); JMPXX_TRY(v, d_leaf(x)); return v + g.v; }
result<int, error> d2(int x) { Counted g(x); JMPXX_TRY(v, d1(x)); return v + g.v; }
result<int, error> d3(int x) { Counted g(x); JMPXX_TRY(v, d2(x)); return v + g.v; }
result<int, error> d4(int x) { Counted g(x); JMPXX_TRY(v, d3(x)); return v + g.v; }
result<int, error> d5(int x) { Counted g(x); JMPXX_TRY(v, d4(x)); return v + g.v; }
result<int, error> d6(int x) { Counted g(x); JMPXX_TRY(v, d5(x)); return v + g.v; }
result<int, error> d_chain(int x) { Counted g(x); JMPXX_TRY(v, d6(x)); return v + g.v; }

int probe_destructors(Fmt fmt) {
  Report r(fmt, "destructors");
  Counted::ctor = 0;
  Counted::dtor = 0;
  result<int, error> bad = d_chain(-1);  // fails at the leaf, propagates up
  long long after_fail_ctor = Counted::ctor;
  long long after_fail_dtor = Counted::dtor;
  Counted::ctor = 0;
  Counted::dtor = 0;
  result<int, error> ok = d_chain(5);  // succeeds through all frames
  r.num("failure.constructed", after_fail_ctor);
  r.num("failure.destructed", after_fail_dtor);
  r.num("success.constructed", Counted::ctor);
  r.num("success.destructed", Counted::dtor);
  r.num("frames", 8);
  if (after_fail_ctor != after_fail_dtor)
    r.fail("failure path skipped or double-ran a destructor");
  if (Counted::ctor != Counted::dtor)
    r.fail("success path skipped or double-ran a destructor");
  if (after_fail_ctor < 8)
    r.fail("fewer than eight frames were exercised on the failure path");
  if (bad.has_value() || bad.error().code != 7)
    r.fail("the originating failure was not delivered to the landing boundary");
  if (!ok.has_value())
    r.fail("the success value was not delivered to the landing boundary");
  return r.finish();
}

// semantics: the exactly-one contract and the access surface behave as specified.
int probe_semantics(Fmt fmt) {
  Report r(fmt, "semantics");
  auto expect = [&](bool cond, const char* what) {
    if (!cond) r.fail(what);
  };
  long long checks = 0;
  auto C = [&](bool cond, const char* what) {
    ++checks;
    expect(cond, what);
  };

  result<int, error> v = 7;
  C(v.has_value() && static_cast<bool>(v) && v.value() == 7 && *v == 7,
    "value access");
  result<int, error> e = fail(error(9));
  C(!e.has_value() && !static_cast<bool>(e) && e.error().code == 9,
    "error access");
  C(v.value_or(0) == 7 && e.value_or(-1) == -1, "value_or");

  result<int, error> c = v;
  C(c.has_value() && c.value() == 7, "copy keeps the value");
  result<int, error> m = static_cast<result<int, error>&&>(c);
  C(m.has_value() && m.value() == 7, "move keeps the value");

  v = fail(error(5));
  C(!v.has_value() && v.error().code == 5, "value-to-error assignment");
  v = 11;
  C(v.has_value() && v.value() == 11, "error-to-value assignment");

  result<void, error> ok{};
  C(ok.has_value(), "void success");
  result<void, error> verr = fail(error(2));
  C(!verr.has_value() && verr.error().code == 2, "void failure");

  C(result<int, error>(3) == 3, "equality against a value");
  C(result<int, error>(fail(error(4))) == failure<error>(error(4)),
    "equality against a failure");
  C(result<int, error>(3) == result<int, error>(3), "equality against a result");

  r.num("checks", checks);
  return r.finish();
}

// levels: the declared cost and reach of each propagation level. The doc-claim
// tier compares these to the reference documentation.
int probe_levels(Fmt fmt) {
  Report r(fmt, "levels");
  r.str("try.cost", "one conditional branch; no allocation; no frame");
  r.str("try.reach", "propagates one frame per use and composes to any depth");
  r.str("scope.cost", "one call frame unless inlined; no allocation");
  r.str("scope.reach", "one typed landing boundary for a region");
  return r.finish();
}

int probe_policies(Fmt fmt) {
  Report r(fmt, "policies");
  auto behaves = [&](const char* name, auto good, auto bad) {
    bool ok = good.has_value() && good.value() == 12 && !bad.has_value();
    r.boolean(std::string(name) + ".identical_behavior", ok);
    if (!ok)
      r.fail(std::string(name) +
             ": a policy changed observable behavior over identical source");
  };
  behaves("minimal", pol_chain<error>(5), pol_chain<error>(-1));
  behaves("rich", pol_chain<rich_error>(5), pol_chain<rich_error>(-1));
  behaves("erased", pol_chain<erased_error>(5), pol_chain<erased_error>(-1));

  // The same propagation construct serves every policy, and every policy keeps the
  // transport trivially copyable and register-sized.
  r.boolean("rich.trivially_copyable", std::is_trivially_copyable_v<rich_error>);
  r.boolean("erased.trivially_copyable",
            std::is_trivially_copyable_v<erased_error>);
  r.boolean("result_rich.trivially_copyable",
            std::is_trivially_copyable_v<result<int, rich_error>>);
  r.boolean("result_erased.trivially_copyable",
            std::is_trivially_copyable_v<result<int, erased_error>>);
  r.num("sizeof.result_minimal",
        static_cast<long long>(sizeof(result<int, error>)));
  r.num("sizeof.result_rich",
        static_cast<long long>(sizeof(result<int, rich_error>)));
  r.num("diagnostics_enabled", JMPXX_DIAGNOSTICS_ENABLED);
  if (!std::is_trivially_copyable_v<result<int, rich_error>> ||
      !std::is_trivially_copyable_v<result<int, erased_error>>)
    r.fail("a policy made the transport non-trivially-copyable");
  return r.finish();
}

// An 8-frame chain over the rich policy, so the diagnostics probe reports a chain
// with real depth rather than a token one.
result<int, rich_error> di_leaf(int x) {
  if (x < 0) return fail(rich_error(7, 3));
  return x * 2;
}
result<int, rich_error> di1(int x) { JMPXX_TRY(v, di_leaf(x)); return v + 1; }
result<int, rich_error> di2(int x) { JMPXX_TRY(v, di1(x)); return v + 1; }
result<int, rich_error> di3(int x) { JMPXX_TRY(v, di2(x)); return v + 1; }
result<int, rich_error> di4(int x) { JMPXX_TRY(v, di3(x)); return v + 1; }
result<int, rich_error> di5(int x) { JMPXX_TRY(v, di4(x)); return v + 1; }
result<int, rich_error> di6(int x) { JMPXX_TRY(v, di5(x)); return v + 1; }
result<int, rich_error> di_chain(int x) { JMPXX_TRY(v, di6(x)); return v + 1; }

// diagnostics: in a debug build the rich policy attaches a failure's origin and the
// causal chain it accumulated. The probe drives an 8-frame failure and reports the
// captured origin and per-hop chain; in a release build it reports that the layer
// is absent, which is itself the zero-cost personality.
int probe_diagnostics(Fmt fmt) {
  Report r(fmt, "diagnostics");
  r.num("diagnostics_enabled", JMPXX_DIAGNOSTICS_ENABLED);
#if JMPXX_DIAGNOSTICS_ENABLED
  landing root;
  result<int, rich_error> bad = di_chain(-1);  // fails at the leaf, 7 hops up
  if (bad.has_value()) {
    r.fail("the failure did not propagate to the landing boundary");
    return r.finish();
  }
  diagnostic::context c = diagnostic::inspect(bad.error());
  r.boolean("context_available", c.available);
  if (!c.available) {
    r.fail("no diagnostic context was captured for the failure");
    return r.finish();
  }
  r.str("origin.function", c.origin.function_name());
  r.num("origin.line", static_cast<long long>(c.origin.line()));
  r.num("chain.hops", c.hop_count);
  r.boolean("chain.truncated", c.hops_truncated);
  // The leaf plus six wrappers each contribute one hop as the failure climbs to
  // di_chain: seven propagation sites.
  if (c.hop_count != 7)
    r.fail("the causal chain did not record each propagation hop (expected 7, got " +
           std::to_string(c.hop_count) + ")");
  r.boolean("trace_available", platform::trace_available());

  // The success path attaches nothing, and the rich failure decays to the minimal
  // error losslessly in its in-band fields.
  result<int, rich_error> ok = di_chain(5);
  if (!ok.has_value()) r.fail("the success path did not deliver its value");
  error decayed = bad.error().base();
  if (decayed.code != 7 || decayed.domain != 3)
    r.fail("the rich error did not decay to the minimal error losslessly");
#else
  r.note(
      "diagnostic layer compiled out in this build; the rich policy is identical "
      "to the minimal policy");
#endif
  return r.finish();
}

// codegen: compile a fixture with a pinned compiler, slice and normalize the
// target function's assembly, and compare it to a committed golden.
std::string trim(const std::string& s) {
  std::size_t a = s.find_first_not_of(" \t");
  if (a == std::string::npos) return "";
  std::size_t b = s.find_last_not_of(" \t");
  return s.substr(a, b - a + 1);
}
std::string read_file(const std::string& p) {
  std::ifstream f(p);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}
bool write_file(const std::string& p, const std::string& c) {
  std::ofstream f(p);
  f << c;
  return static_cast<bool>(f);
}

struct Normalized {
  std::string text;
  long long instructions = 0;
  bool spilled = false;
  bool found = false;
};

// Keep instruction and label lines of one function; drop directives, comments,
// and blank lines. For a pinned compiler and flags the output is reproducible,
// so local label names are stable and need no renaming. Flag any stack traffic
// (a spill or a frame) so a zero-overhead fixture can require its absence.
Normalized normalize(const std::string& asm_text, const std::string& symbol,
                     const std::string& arch) {
  Normalized n;
  std::istringstream in(asm_text);
  std::string line;
  bool inside = false;
  const bool x86 = (arch == "x86_64");
  const std::string start = symbol + ":";
  while (std::getline(in, line)) {
    std::string t = trim(line);
    if (!inside) {
      if (t == start) {
        inside = true;
        n.found = true;
      }
      continue;
    }
    if (t.rfind(".size", 0) == 0 || t.rfind(".cfi_endproc", 0) == 0) break;
    auto hash = t.find('#');  // GCC uses '#' for asm comments on x86
    if (hash != std::string::npos) t = trim(t.substr(0, hash));
    if (t.empty()) continue;
    if (t[0] == '.' && t.back() != ':') continue;  // assembler directive
    n.text += t;
    n.text += '\n';
    if (t.back() == ':') continue;  // label, not an instruction
    ++n.instructions;
    if (x86) {
      if (t.find("(%rsp)") != std::string::npos ||
          t.find("(%rbp)") != std::string::npos ||
          t.rfind("push", 0) == 0 || t.rfind("pop", 0) == 0)
        n.spilled = true;
    } else {
      if (t.find("[sp") != std::string::npos ||
          t.find("[x29") != std::string::npos || t.rfind("stp", 0) == 0 ||
          t.rfind("ldp", 0) == 0)
        n.spilled = true;
    }
  }
  return n;
}

// Rename compiler-assigned local labels (.L<n>, .LC<n>) to a canonical sequence by
// first appearance. Two functions that differ only in label numbering, for example
// because one is emitted after the other in the same translation unit, then compare
// equal. Label names carry no semantics; the control flow they encode is preserved
// because every occurrence of one name maps to the same canonical token. This is
// applied only when diffing two functions against each other, never against a
// committed golden, so the golden tier keeps comparing exact text.
std::string canon_labels(const std::string& body) {
  auto id = [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '$';
  };
  std::map<std::string, std::string> seen;
  std::string out;
  out.reserve(body.size());
  for (std::size_t i = 0, n = body.size(); i < n;) {
    if (body[i] == '.' && i + 2 < n && body[i + 1] == 'L' && id(body[i + 2])) {
      std::size_t j = i + 2;
      while (j < n && id(body[j])) ++j;
      std::string tok = body.substr(i, j - i);
      auto it = seen.find(tok);
      if (it == seen.end())
        it = seen.emplace(tok, ".L#" + std::to_string(seen.size())).first;
      out += it->second;
      i = j;
    } else {
      out += body[i++];
    }
  }
  return out;
}

int probe_codegen(Fmt fmt, const std::vector<std::string>& args) {
  std::string fixture, symbol, golden, arch = "x86_64";
  bool update = false, forbid_spill = false;
  long long max_insns = -1;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    auto next = [&]() { return (i + 1 < args.size()) ? args[++i] : std::string(); };
    if (a == "--fixture") fixture = next();
    else if (a == "--symbol") symbol = next();
    else if (a == "--golden") golden = next();
    else if (a == "--arch") arch = next();
    else if (a == "--update") update = true;
    else if (a == "--forbid-spill") forbid_spill = true;
    else if (a == "--max-insns") max_insns = std::atoll(next().c_str());
  }
  Report r(fmt, "codegen");
  r.str("fixture", fixture);
  r.str("symbol", symbol);
  r.str("arch", arch);
  if (fixture.empty() || symbol.empty() || golden.empty()) {
    r.fail("codegen requires --fixture, --symbol, and --golden");
    return r.finish();
  }

  // Pinned compiler per architecture; overridable for diagnosis via the
  // environment. Goldens are reproducible only against the pinned toolchain.
  std::string cxx;
  if (const char* env = std::getenv("JMPXX_CG_CXX"))
    cxx = env;
  else if (arch == "x86_64")
    cxx = "g++-13";
  else if (arch == "aarch64")
    cxx = "aarch64-linux-gnu-g++";
  else {
    r.fail("unknown --arch (expected x86_64 or aarch64)");
    return r.finish();
  }

  const std::string asm_out = "/tmp/jmpxx_cg.s";
  const std::string err_out = "/tmp/jmpxx_cg.err";
  std::string cmd = cxx +
      " -std=c++20 -O2 -fno-exceptions -fno-rtti"
      " -fno-asynchronous-unwind-tables -fno-ident -S"
      " -I" JMPXX_INCLUDE_DIR " -o " + asm_out + " " + fixture + " 2> " + err_out;
  r.str("compiler", cxx);
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    r.fail("fixture failed to compile: " + trim(read_file(err_out)));
    return r.finish();
  }

  Normalized n = normalize(read_file(asm_out), symbol, arch);
  if (!n.found) {
    r.fail("symbol '" + symbol + "' not found in the emitted assembly");
    return r.finish();
  }
  r.num("instructions", n.instructions);
  r.boolean("spilled", n.spilled);

  // The spill and instruction-count assertions hold regardless of --update, so
  // a known-bad fixture is rejected even while its golden is being written.
  if (forbid_spill && n.spilled)
    r.fail("a stack spill or frame appeared on a path declared zero-overhead");
  if (max_insns >= 0 && n.instructions > max_insns)
    r.fail("instruction count " + std::to_string(n.instructions) +
           " exceeds the budget " + std::to_string(max_insns));

  if (update) {
    if (!write_file(golden, n.text))
      r.fail("could not write golden " + golden);
    else
      r.note("golden updated: " + golden);
    return r.finish();
  }

  std::string want = read_file(golden);
  bool matches = (want == n.text);
  r.boolean("golden_match", matches);
  if (!matches)
    r.fail("optimized codegen diverged from the committed golden");
  return r.finish();
}

// release-diff: the headline proof of the dual-personality promise. Two functions
// in one fixture perform the same operation, one under the minimal policy and one
// under the rich policy. Compiled in a release configuration the diagnostic layer
// is gone, so the two must generate identical code. The gate compares their
// label-canonicalized bodies, and an inverted run asserts a known-bad fixture whose
// diagnostic call survives into release is caught. It also scans for forbidden
// strings, because a source location materializes its file and function names into
// read-only data even where the value is unused, a leak an instruction diff alone
// would miss.
int probe_release_diff(Fmt fmt, const std::vector<std::string>& args) {
  std::string fixture, sym_a, sym_b, arch = "x86_64", expect = "equal";
  std::vector<std::string> forbid;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    auto next = [&]() { return (i + 1 < args.size()) ? args[++i] : std::string(); };
    if (a == "--fixture") fixture = next();
    else if (a == "--symbol-a") sym_a = next();
    else if (a == "--symbol-b") sym_b = next();
    else if (a == "--arch") arch = next();
    else if (a == "--expect") expect = next();
    else if (a == "--forbid-string") forbid.push_back(next());
  }
  Report r(fmt, "release-diff");
  r.str("fixture", fixture);
  r.str("symbol_a", sym_a);
  r.str("symbol_b", sym_b);
  r.str("expect", expect);
  if (fixture.empty() || sym_a.empty() || sym_b.empty()) {
    r.fail("release-diff requires --fixture, --symbol-a, and --symbol-b");
    return r.finish();
  }

  std::string cxx;
  if (const char* env = std::getenv("JMPXX_CG_CXX"))
    cxx = env;
  else if (arch == "x86_64")
    cxx = "g++-13";
  else if (arch == "aarch64")
    cxx = "aarch64-linux-gnu-g++";
  else {
    r.fail("unknown --arch (expected x86_64 or aarch64)");
    return r.finish();
  }

  const std::string asm_out = "/tmp/jmpxx_rd.s";
  const std::string err_out = "/tmp/jmpxx_rd.err";
  // A release configuration: NDEBUG turns the diagnostic layer off, which is the
  // configuration under which the rich policy must equal the minimal policy.
  std::string cmd = cxx +
      " -std=c++20 -O2 -DNDEBUG -fno-exceptions -fno-rtti"
      " -fno-asynchronous-unwind-tables -fno-ident -S"
      " -I" JMPXX_INCLUDE_DIR " -o " + asm_out + " " + fixture + " 2> " + err_out;
  r.str("compiler", cxx);
  if (std::system(cmd.c_str()) != 0) {
    r.fail("fixture failed to compile: " + trim(read_file(err_out)));
    return r.finish();
  }

  std::string text = read_file(asm_out);
  Normalized a = normalize(text, sym_a, arch);
  Normalized b = normalize(text, sym_b, arch);
  if (!a.found || !b.found) {
    r.fail("a target symbol was not found in the emitted assembly");
    return r.finish();
  }
  std::string ca = canon_labels(a.text);
  std::string cb = canon_labels(b.text);
  bool identical = (ca == cb);
  r.num(sym_a + ".instructions", a.instructions);
  r.num(sym_b + ".instructions", b.instructions);
  r.boolean("bodies_identical", identical);

  if (expect == "equal" && !identical)
    r.fail("release codegen of the rich policy diverged from the minimal policy");
  if (expect == "differ" && identical)
    r.fail("expected the policies to diverge but their release codegen was identical");

  // Forbidden-string scan: a source location, or any debug-only marker, must not
  // reach a release build's read-only data. Scan the emitted string directives.
  for (const std::string& mark : forbid) {
    bool leaked = false;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
      std::string t = trim(line);
      if ((t.rfind(".string", 0) == 0 || t.rfind(".ascii", 0) == 0) &&
          t.find(mark) != std::string::npos) {
        leaked = true;
        break;
      }
    }
    r.boolean("leaked." + mark, leaked);
    if (leaked)
      r.fail("a debug-only string leaked into release read-only data: " + mark);
  }
  return r.finish();
}

// interop: exercise each bridge and report its round-trip fidelity, so the bridges
// are observable through the surface rather than only by reading source. The
// std::expected and exception bridges are reported as available or not, because each
// is present only in the configuration that supports it.
int probe_interop(Fmt fmt) {
  Report r(fmt, "interop");

  r.boolean("expected.available", JMPXX_INTEROP_HAS_EXPECTED);
#if JMPXX_INTEROP_HAS_EXPECTED
  {
    result<int, error> rv = 7;
    result<int, error> re = fail(error(9, 3));
    bool ok = from_expected(to_expected(rv)).value() == 7 &&
              from_expected(to_expected(re)).error() == error(9, 3);
    r.boolean("expected.roundtrip_lossless", ok);
    if (!ok) r.fail("std::expected round-trip lost information");
  }
#endif

  {
    error e{5, 2};
    std::error_code ec = to_error_code(e);
    bool expose = from_error_code(ec) == e && is_jmpxx(ec);
    std::error_code foreign = std::make_error_code(std::errc::invalid_argument);
    result<int, std::error_code> carried = fail(foreign);
    bool verbatim = carried.error() == foreign && !is_jmpxx(foreign);
    bool identity = &error_category(2) == &error_category(2) &&
                    &error_category(2) != &error_category(3);
    r.boolean("error_code.expose_recover_lossless", expose);
    r.boolean("error_code.verbatim_carry", verbatim);
    r.boolean("error_code.category_identity_stable", identity);
    if (!expose || !verbatim || !identity)
      r.fail("std::error_code bridge lost fidelity");
  }

  r.boolean("exception.available", JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE);
#if JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE
  {
    result<int, error> bad = fail(error(42, 7));
    result<int, error> back = catch_into_result<error>(
        [&] { return value_or_throw(bad); }, [] { return error(-1); });
    bool ok = !back.has_value() && back.error() == error(42, 7);
    r.boolean("exception.roundtrip_lossless", ok);
    if (!ok) r.fail("exception bridge round-trip lost information");
  }
#endif

  {
    int v = 7;
    int* present = &v;
    int* absent = nullptr;
    bool opt = from_optional<error>(present, [] { return error(1); }).value() == 7 &&
               !from_optional<error>(absent, [] { return error(2); }).has_value();
    bool cond = from_condition<error>(true, [] { return 5; },
                                      [] { return error(3); }).value() == 5;
    r.boolean("adapt.from_optional", opt);
    r.boolean("adapt.from_condition", cond);
    if (!opt || !cond) r.fail("optional-like adapter failed");
  }
  return r.finish();
}

// unwind: drive the experimental non-local unwind arm and report what only execution
// can show: the destructor count over a deep escape and the sad-path latency
// distribution. The destructor count proves the arm runs every cleanup on the path;
// the distribution, reported by median and high percentiles, is the evidence behind
// the bounded-sad-path claim. A C++ throw at the same depth is measured alongside so
// the comparison is measured rather than asserted: the arm's sad path is bounded, not
// uniformly faster than a throw.
//
// The gate compares the arm's 99th-percentile latency to a C++ throw's at the same depth.
// An absolute nanosecond bound would not travel across machines, and an absolute
// percentile-to-median ratio flakes on a shared runner, where a rare scheduling stall
// inflates the high percentile of any operation. Measuring both against the same runner
// cancels that noise: a stall that lengthens the arm's tail lengthens the throw's tail
// too, so the ratio between them stays bounded. The gate has teeth: --inject-jitter makes
// the arm's cleanup path, and not the throw's, non-deterministic, which drives the ratio
// past the bound and fails the gate, proving the gate is not vacuous.
namespace uw {

volatile long long g_sink = 0;
bool g_jitter = false;
long long g_jitter_counter = 0;
constexpr int depth = 8;  // at least eight frames on the escape path

// A guard whose destructor does real work. The Jitter parameter selects whether a known
// non-deterministic cleanup may be injected. It is injected on the forced-unwind path
// only, so the gate that compares the arm's tail latency to a C++ throw's catches it,
// while ordinary scheduling noise affects both paths together and does not.
template <bool Jitter>
struct GuardT {
  int v;
  ~GuardT() {
    g_sink = g_sink + v;
    // Inject jitter on one cleanup per escape, the leaf (v == 0), rather than once per
    // frame, so the cost is one long cleanup per affected escape. A fraction of escapes
    // then become far costlier than the rest, the non-determinism the gate catches. The
    // loop dwarfs an ordinary escape on any runner, including a slow one with a high
    // baseline, so the injected ratio clears the bound by a wide margin.
    if (Jitter && g_jitter && v == 0 && (g_jitter_counter++ & 31) == 0) {
      volatile long long s = 0;
      for (int i = 0; i < 500000; ++i) s = s + i;
      g_sink = g_sink + s;
    }
  }
};

// The escape chains are template recursions, so the depth is a compile-time bound that
// expands to distinct frames rather than a runtime self-recursion the compiler cannot
// prove terminates. The forced-unwind chain carries the jittered guard; the C++ throw
// chain, the baseline, carries the plain one.
template <int D>
int fu_chain() {
  GuardT<true> g{D};
  if constexpr (D <= 0)
    return jmpxx::unwind::eject(jmpxx::error(42));
  else
    return fu_chain<D - 1>() + g.v;
}
int fu_escape() {
  auto r = jmpxx::unwind::escape_scope<jmpxx::error>([] { return fu_chain<depth>(); });
  return r.has_value() ? 0 : r.error().code;
}

struct ex {
  int code;
};
template <int D>
int th_chain() {
  GuardT<false> g{D};
  if constexpr (D <= 0)
    throw ex{42};
  else
    return th_chain<D - 1>() + g.v;
}
int th_escape() {
  try {
    return th_chain<depth>();
  } catch (const ex& e) {
    return e.code;
  }
}

struct dist {
  long long median, p90, p99, max;
};

dist summarize(std::vector<long long>& ns) {
  std::sort(ns.begin(), ns.end());
  auto at = [&](double p) { return ns[static_cast<std::size_t>(p * (ns.size() - 1))]; };
  return {at(0.5), at(0.9), at(0.99), ns.back()};
}

// Measure two escape paths back to back in one loop so they sample the same contention.
// The gate compares their tails, so the measurement must expose both paths to the same
// stalls. Timing them microseconds apart in each iteration, rather than in two separate
// loops, makes a stall that spans an iteration inflate both samples, which keeps the
// relative bound from flaking when a shared runner is briefly busy.
template <class A, class B>
std::pair<dist, dist> measure_pair(A fa, B fb, int iters) {
  for (int i = 0; i < 2000; ++i) g_sink = g_sink + fa() + fb();  // warm up both
  std::vector<long long> na, nb;
  na.reserve(iters);
  nb.reserve(iters);
  using clock = std::chrono::steady_clock;
  auto elapsed = [](clock::time_point s, clock::time_point e) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();
  };
  for (int i = 0; i < iters; ++i) {
    auto t0 = clock::now();
    g_sink = g_sink + fa();
    auto t1 = clock::now();
    g_sink = g_sink + fb();
    auto t2 = clock::now();
    na.push_back(elapsed(t0, t1));
    nb.push_back(elapsed(t1, t2));
  }
  return {summarize(na), summarize(nb)};
}

// Count constructions and destructions over a deep escape, to prove the arm runs every
// cleanup exactly once. Kept apart from the timed guard so timing carries no counters.
long long c_ctor = 0, c_dtor = 0;
struct Counted {
  Counted() { ++c_ctor; }
  ~Counted() { ++c_dtor; }
};
template <int D>
int cnt_chain() {
  Counted g;
  if constexpr (D <= 0)
    return jmpxx::unwind::eject(jmpxx::error(7));
  else
    return cnt_chain<D - 1>();
}

}  // namespace uw

int probe_unwind(Fmt fmt, const std::vector<std::string>& args) {
  int iters = 50000;
  // The committed determinism bound: the arm's 99th-percentile sad path stays within this
  // multiple of a C++ throw's at the same depth. It is set generously above the observed
  // ratio (near 1.1 to 1.9 across compilers) so ordinary scheduling noise on a shared
  // runner does not flake it, while still well below the ratio an injected
  // non-deterministic cleanup produces, which gives the gate teeth.
  double bound = 3.0;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    auto next = [&]() { return (i + 1 < args.size()) ? args[++i] : std::string(); };
    if (a == "--inject-jitter") uw::g_jitter = true;
    else if (a == "--iters") iters = std::atoi(next().c_str());
    else if (a == "--bound-factor") bound = std::atof(next().c_str());
  }
  Report r(fmt, "unwind");
  r.boolean("available", unwind::available());
  if (!unwind::available()) {
    r.note("the unwind arm has no backend on this target; nothing to drive");
    return r.finish();
  }
#if JMPXX_UNWIND_BACKEND_ITANIUM
  r.str("backend", "itanium");
#elif JMPXX_UNWIND_BACKEND_WASM
  r.str("backend", "wasm");
#elif JMPXX_UNWIND_BACKEND_SEH
  r.str("backend", "seh");
#endif

  // Destructor count over a deep escape: every cleanup runs exactly once.
  uw::c_ctor = uw::c_dtor = 0;
  auto cr = unwind::escape_scope<error>([] { return uw::cnt_chain<uw::depth>(); });
  r.num("escape.frames", uw::depth + 1);
  r.num("escape.constructed", uw::c_ctor);
  r.num("escape.destructed", uw::c_dtor);
  r.boolean("escape.balanced",
            uw::c_ctor == uw::c_dtor && uw::c_ctor == uw::depth + 1);
  if (cr.has_value() || cr.error().code != 7)
    r.fail("the escape did not deliver its error to the landing");
  if (uw::c_ctor != uw::c_dtor || uw::c_ctor != uw::depth + 1)
    r.fail("a destructor was skipped or double-run on the escape path");

  // Sad-path distribution, forced unwind against a C++ throw at the same depth. The gate
  // compares the arm's tail to the throw's, so shared-runner scheduling noise that
  // inflates both tails together cancels rather than flaking the gate.
  auto [fu, th] = uw::measure_pair(uw::fu_escape, uw::th_escape, iters);
  double ratio =
      static_cast<double>(fu.p99) / static_cast<double>(th.p99 ? th.p99 : 1);
  r.num("sad_path.iters", iters);
  r.num("sad_path.median_ns", fu.median);
  r.num("sad_path.p90_ns", fu.p90);
  r.num("sad_path.p99_ns", fu.p99);
  r.num("sad_path.max_ns", fu.max);
  r.num("cxx_throw.median_ns", th.median);
  r.num("cxx_throw.p99_ns", th.p99);
  r.num("sad_path.p99_vs_throw_p99_x100", static_cast<long long>(ratio * 100));
  r.num("sad_path.bound_x100", static_cast<long long>(bound * 100));
  r.boolean("sad_path.bounded", ratio <= bound);
  r.note(uw::g_jitter ? "jitter injected: a non-deterministic cleanup is expected to "
                        "exceed the bound"
                      : "the sad path is bounded; its tail is comparable to a C++ throw's, "
                        "not uniformly faster");
  if (ratio > bound)
    r.fail("the sad-path p99 exceeded the committed multiple of a C++ throw's p99");
  return r.finish();
}

// platform: report the build's matrix cell and the capabilities the configuration
// enables, so a continuous-integration run records each cell's status from the same
// surface. The compiler version is supplied by the runner, which knows it; the
// surface reports what the translation unit can determine on its own.
int probe_platform(Fmt fmt) {
  Report r(fmt, "platform");
  r.str("compiler", platform::compiler_name());
  r.str("os", platform::os_name());
  r.str("arch", platform::arch_name());
  r.boolean("hosted", platform::is_hosted());
  r.num("cpp_standard", static_cast<long long>(JMPXX_CPLUSPLUS));
  r.boolean("diagnostics_enabled", JMPXX_DIAGNOSTICS_ENABLED);
  r.boolean("exceptions_enabled", JMPXX_HAS_EXCEPTIONS);
  r.boolean("expected_bridge", JMPXX_INTEROP_HAS_EXPECTED);
  r.boolean("exception_bridge", JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE);
  return r.finish();
}

}  // namespace

int main(int argc, char** argv) {
  Fmt fmt = Fmt::human;
  std::string cmd;
  std::vector<std::string> rest;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--format=json")
      fmt = Fmt::json;
    else if (a == "--format=human")
      fmt = Fmt::human;
    else if (cmd.empty())
      cmd = a;
    else
      rest.push_back(a);
  }

  if (cmd == "size") return probe_size(fmt);
  if (cmd == "alloc") return probe_alloc(fmt);
  if (cmd == "destructors") return probe_destructors(fmt);
  if (cmd == "semantics") return probe_semantics(fmt);
  if (cmd == "levels") return probe_levels(fmt);
  if (cmd == "policies") return probe_policies(fmt);
  if (cmd == "diagnostics") return probe_diagnostics(fmt);
  if (cmd == "interop") return probe_interop(fmt);
  if (cmd == "platform") return probe_platform(fmt);
  if (cmd == "unwind") return probe_unwind(fmt, rest);
  if (cmd == "codegen") return probe_codegen(fmt, rest);
  if (cmd == "release-diff") return probe_release_diff(fmt, rest);
  if (cmd == "all") {
    int rc = 0;
    rc |= probe_size(fmt);
    rc |= probe_alloc(fmt);
    rc |= probe_destructors(fmt);
    rc |= probe_semantics(fmt);
    rc |= probe_levels(fmt);
    rc |= probe_policies(fmt);
    rc |= probe_diagnostics(fmt);
    rc |= probe_interop(fmt);
    rc |= probe_platform(fmt);
    rc |= probe_unwind(fmt, {});
    return rc;
  }
  std::fprintf(stderr,
               "usage: jmpxx-verify <size|alloc|destructors|semantics|levels|"
               "policies|diagnostics|interop|platform|unwind|all|codegen|"
               "release-diff> [--format=json]\n");
  return 2;
}
