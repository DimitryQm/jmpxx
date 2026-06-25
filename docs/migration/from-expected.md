<!-- SPDX-License-Identifier: MIT -->
# Migrating from std::expected

A codebase already returning `std::expected<T, E>` adopts jmpxx with little churn,
because `result<T, E>` carries exactly the same thing, a value or the same error type
`E`, and the conversion between them is lossless in both directions.

## At a boundary, without rewriting

Convert at the boundary and keep the rest of the file as it is. `from_expected` adopts a
function that returns `std::expected`, and `to_expected` hands a `result` back out:

```cpp
#include <jmpxx/interop/expected.hpp>

result<Config, ParseError> load(std::string_view text) {
  return jmpxx::from_expected(library_that_returns_expected(text));
}

std::expected<Config, ParseError> load_expected(std::string_view text) {
  return jmpxx::to_expected(load(text));
}
```

Both directions preserve which alternative is held and the contained value or error
unchanged, and both are usable in constant evaluation. `T` may be `void`. The bridge is
available wherever `std::expected` is, and stays usable with exceptions disabled and in a
freestanding build, because it never calls `std::expected::value()`.

## Replacing the type

Where you replace `std::expected<T, E>` with `result<T, E>` outright, the gains are the
propagation construct and the cost profile. The manual forward

```cpp
auto r = step();
if (!r) return std::unexpected(r.error());
auto value = *r;
```

becomes one line:

```cpp
JMPXX_TRY(value, step());
```

For a trivially copyable `T` and `E`, `result<T, E>` is trivially copyable and returned
in registers, where `std::expected`'s assignment operators make it non-trivial. The
happy path then generates the same code as a hand-written branch, verified by the codegen
gate. A failure cannot be dropped without a compile-time diagnostic, which a bare
`std::expected` does not enforce at the propagation site.

## Monadic composition

`result` provides the same monadic combinators as `std::expected` (P2505), so a pipeline
written against `std::expected` carries over unchanged:

```cpp
result<int, error> r = parse(text)
    .transform([](int n) { return n * 2; })          // map the value
    .and_then([](int n) { return validate(n); })     // chain a result-returning step
    .or_else([](error e) { return recover(e); });    // handle the failure
```

`and_then`, `transform`, `or_else`, and `transform_error` match `std::expected`'s
semantics across the value and error states, with the same lvalue, const, and rvalue
overloads, and a `result<void, E>` invokes the value-side callable with no argument. The
one difference is that jmpxx invokes the callable directly as `f(value)` rather than
through `std::invoke`, which keeps the combinators in the freestanding core with no
`<functional>` dependency; a function, a lambda, or a function object works. `JMPXX_TRY`
remains the cheaper, exception-free path for straight-line propagation, and the
combinators are there when a pipeline reads better. See [interop.md](../reference/interop.md).
