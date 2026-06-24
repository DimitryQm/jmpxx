<!-- SPDX-License-Identifier: MIT -->
# jmpxx

jmpxx is a header-only C++20 library for non-local, RAII-correct, exception-free
error propagation. A failure returned deep in a call chain travels to a single
typed handling point, every destructor along the way runs, and the success path
compiles to the same machine code as a hand-written branch on a status flag.

It is built for code compiled with `-fno-exceptions`: embedded firmware, game
engines, and low-latency systems, where binary size and tail latency matter and
the generated code has to be inspectable.

## The problem it solves

With exceptions disabled, returning an error from a deep call chain means
threading a status value back through every function by hand. `longjmp` skips
that boilerplate, but it also skips C++ destructors, which is undefined behavior.
Exceptions keep cleanup correct but add a non-deterministic slow path and unwind
tables that this audience often cannot afford.

jmpxx removes the manual threading while keeping destructors correct and the
cost bounded. The error value is stored out of band, so the functions in the
middle of the chain never grow an error type in their signatures.

## Example

```cpp
#include <jmpxx/core.hpp>

jmpxx::result<int> parse_port(std::string_view text);

jmpxx::result<Config> load_config(std::string_view text) {
  JMPXX_TRY(port, parse_port(text));  // on failure, return it from load_config
  Config cfg;
  cfg.port = port;
  return cfg;
}
```

`result<T>` holds either a `T` or an error. `JMPXX_TRY` evaluates an expression,
binds its value on success, and returns its error to the caller on failure.

## Status

v0.1.0, under active development. This release provides the value-or-error
`result` type, three error representation policies over one transport (the
minimal freestanding error, the rich error that carries a failure's origin and
causal chain in debug and is free in release, and the type-erased boundary
error), the debug-only diagnostic layer, single-construct propagation to a typed
handling point, and a verification tool whose codegen, size, and allocation
gates, including a release diff that proves the rich policy is free in release,
are checked against known-bad inputs so they cannot pass silently. It also
provides interoperability bridges to `std::expected`, `std::error_code`, and
language exceptions, with validation of untrusted input at the boundary, and a
platform abstraction that fences target-specific code behind one unit. An
experimental, opt-in non-local escape returns a failure from arbitrary call depth
to one landing boundary while the platform unwinder runs every destructor on the
path, so the intermediate frames carry no propagation construct. It requires unwind
tables, is reached only through `jmpxx/unwind.hpp`, and runs on four ABIs including
a bare-metal target. The full benchmark suite and the wider packaging surfaces are
planned for later releases.

## How performance claims are backed

Every statement about size, speed, or generated code is measured and gated in
CI, and a regression fails the build. The `jmpxx-verify` tool compiles a fixture,
emits its optimized machine code, compares it against a committed reference, and
reports transport size and allocation counts as JSON. Claims without a backing
artifact are not made.

## License

MIT. See [LICENSE](LICENSE).
