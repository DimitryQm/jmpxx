// SPDX-License-Identifier: MIT
// A consumer of the single-header amalgamation. It includes only <jmpxx.hpp> and uses
// the transport, the rich policy, and the reflection forward layer, so a successful
// build checks that the one generated header is self-contained across the hosted surface.
#include <jmpxx.hpp>

#include <string_view>

using namespace jmpxx;

enum class state { idle = 0, busy = 1, failed = 2 };

static result<int, rich_error> read(int raw) {
  if (raw < 0) return fail(rich_error(7));
  return raw;
}

// Single-construct propagation lives in a function returning a result, the shape every
// real caller uses; main inspects the outcome.
static result<int, rich_error> run() {
  JMPXX_TRY(v, read(5));
  return v + 1;
}

int main() {
  landing root;
  auto ok = run();
  if (!ok || ok.value() != 6) return 1;
  auto bad = read(-1);
  if (bad || bad.error().code != 7) return 2;
  if (reflect::enum_name(state::busy) != std::string_view("busy")) return 3;
  erased_error e = reflect::as_erased(state::failed);
  if (std::string_view(e.message()) != std::string_view("failed")) return 4;
  return 0;
}
