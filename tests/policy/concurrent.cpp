// SPDX-License-Identifier: MIT
// Concurrency tier for the diagnostic store, run under the thread sanitizer. Many
// threads create rich failures, propagate them through deep chains, and read their
// origin and causal chain at the same time. The out-of-band store is per-thread, so
// concurrent failures never touch the same memory and there is nothing to
// synchronize; this file exercises that hard so the thread sanitizer can confirm it
// rather than the design merely asserting it. Each thread also checks its own
// results, so a per-thread store that leaked across threads would corrupt a chain
// length and fail the run even without a sanitizer.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"

#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

using namespace jmpxx;

namespace {

result<int, rich_error> leaf(int x) {
  if (x < 0) return fail(rich_error(x, 1));
  return x;
}
result<int, rich_error> a(int x) { JMPXX_TRY(v, leaf(x)); return v + 1; }
result<int, rich_error> b(int x) { JMPXX_TRY(v, a(x)); return v + 1; }
result<int, rich_error> c(int x) { JMPXX_TRY(v, b(x)); return v + 1; }

std::atomic<long long> g_failures{0};
std::atomic<int> g_violations{0};

void worker(int seed) {
  long long local_fail = 0;
  for (int i = 0; i < 20000; ++i) {
    landing root;
    int x = ((i + seed) % 5 == 0) ? -(i + 1) : (i + 1);
    result<int, rich_error> r = c(x);
    if (r.has_value()) continue;
    ++local_fail;
    // The leaf plus a and b each add a hop on the way to c: three propagation sites.
    diagnostic::context cx = diagnostic::inspect(r.error());
    if (!cx.available || cx.hop_count != 3) ++g_violations;
    if (r.error().code != x) ++g_violations;  // the failure carried this thread's value
  }
  g_failures += local_fail;
}

}  // namespace

int main() {
  const int n = 8;
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back(worker, i * 7);
  for (auto& t : ts) t.join();

  if (g_violations.load() != 0) {
    std::fprintf(stderr, "concurrent: %d per-thread store violations\n",
                 g_violations.load());
    return 1;
  }
  std::printf("policy/concurrent: %lld concurrent failures, no race, no violation\n",
              g_failures.load());
  return 0;
}
