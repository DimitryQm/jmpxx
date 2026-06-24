<!-- SPDX-License-Identifier: MIT -->
# Migrating from std::expected

A codebase already returning `std::expected<T, E>` adopts jmpxx with little churn,
because `result<T, E>` carries exactly the same thing, a value or the same error type
`E`, and the conversion between them is lossless in both directions.

## At a seam, without rewriting

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
happy path then generates the same code as a hand-written branch, proven by the codegen
gate. A failure cannot be dropped without a compile-time diagnostic, which a bare
`std::expected` does not enforce at the propagation site.

## What changes

`result` carries a value or an error and nothing else. The monadic combinators
`and_then`, `or_else`, and `transform` are not part of its surface, because the
propagation construct covers the case they are most used for. Keep `std::expected` where
you rely on those combinators, and bridge at the seam. See [interop.md](../reference/interop.md).
