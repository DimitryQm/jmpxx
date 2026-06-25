<!-- SPDX-License-Identifier: MIT -->
# Cookbook

Each recipe is drawn from a program under [examples/](../examples/README.md) or the
[reference application](../reference_app/README.md), which build and run as tests, so
the code here is code that compiles and runs. Each recipe names its runnable source.

## Return a failure and propagate it

A function that can fail returns `result<T>`. The `fail` helper builds a failure, and
`JMPXX_TRY` binds the value on success and returns the failure to the caller otherwise.
One landing point handles it, with no per-call check.

```cpp
#include <jmpxx/core.hpp>
using namespace jmpxx;

result<int> parse_port(std::string_view text);

result<Config> load(std::string_view text) {
  JMPXX_TRY(port, parse_port(text));   // on failure, return it from load
  Config cfg;
  cfg.port = port;
  return cfg;
}
```

Runnable: [examples/result_basics.cpp](../examples/result_basics.cpp).

## Select an error policy without changing the call sites

One transport serves three error representations. A program selects one by the error
type it names, usually through an alias, and the call sites do not change.

```cpp
using error = jmpxx::rich_error;                 // or jmpxx::error, or jmpxx::erased_error
template <class T> using result = jmpxx::result<T, error>;
```

The reference application defines `app::error` this way in `app_policy.hpp` and builds
the same source under the minimal and the rich policy. Runnable:
[examples/policies.cpp](../examples/policies.cpp).

## Learn where a failure began, in debug only

The rich policy records a failure's origin and the path it propagated. A `landing` scope
bounds that context, and `diagnostic::print` renders it. In release the layer compiles
to nothing.

```cpp
#include <jmpxx/diagnostics.hpp>
jmpxx::landing root;
result<int, jmpxx::rich_error> r = deep_call();
if (!r) jmpxx::diagnostic::print(r.error(), stderr);
```

Runnable: [examples/diagnostics.cpp](../examples/diagnostics.cpp). Reference:
[diagnostics.md](reference/diagnostics.md).

## Adopt jmpxx at a library boundary

The bridges convert to and from `std::expected` and `std::error_code` without loss, and
the adapters turn another library's absent value into a failure in one call.

```cpp
#include <jmpxx/interop/expected.hpp>
#include <jmpxx/interop/adapt.hpp>

result<T, E> r = jmpxx::from_expected(library_call());          // std::expected -> result
auto v = jmpxx::from_optional<E>(maybe, [] { return E(...); }); // empty optional -> failure
```

The reference application adopts glaze's `std::expected` and toml++'s optionals this
way. Runnable: [examples/interop.cpp](../examples/interop.cpp). Reference:
[interop.md](reference/interop.md).

## Derive error names and a domain from an enum

The reflection layer renders an enumerator's name, the set of failures an enum declares,
and an `error_domain` whose messages are the enumerator names, with no hand-written
table. The same code runs on a C++20 toolchain and a reflection-capable one.

```cpp
#include <jmpxx/reflect.hpp>
enum class limit_error { missing_section = 1, missing_key = 2, out_of_range = 3 };

auto name = jmpxx::reflect::enum_name(limit_error::out_of_range);   // "out_of_range"
jmpxx::erased_error e = jmpxx::reflect::as_erased(limit_error::out_of_range);
// e.domain_name() == "limit_error", e.message() == "out_of_range"
```

The reference application's `[limits]` component reports its errors this way. Runnable:
[examples/reflection.cpp](../examples/reflection.cpp). Reference:
[reflect.md](reference/reflect.md).

## Escape non-locally while running destructors

The experimental, opt-in unwind arm ejects a failure from any depth to one landing while
every destructor on the path runs, so the intermediate frames carry no propagation
construct. It requires unwind tables, so the translation unit is built with exceptions
enabled.

```cpp
#include <jmpxx/unwind.hpp>
auto r = jmpxx::unwind::escape_scope<jmpxx::error>([&] {
  return resolve(root, 0);              // a deep recursion that may eject
});
```

The reference application's `config_resolve` resolves nested references this way.
Runnable: [examples/unwind_arm.cpp](../examples/unwind_arm.cpp). Reference:
[unwind.md](reference/unwind.md).
