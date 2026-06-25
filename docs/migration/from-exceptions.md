<!-- SPDX-License-Identifier: MIT -->
# Migrating from exceptions

Moving a codebase off exceptions, or introducing an exception-free component into one
that still throws, happens at a boundary. jmpxx provides that boundary so the two styles
coexist while you convert, and so an exception-free region can call into, or be called
from, code that throws.

## The boundary bridge

The exception bridge exists only where exceptions are enabled, so it is the boundary itself,
not the exception-free code on either side of it. `value_or_throw` turns a failure into a
throw at the point an exception-free region hands a result to throwing code;
`catch_into_result` runs a callable that may throw and converts the outcome back into a
result, so the rest of the program keeps propagating failures the jmpxx way:

```cpp
#include <jmpxx/interop/exception.hpp>

// Exception-free code calling into a component that throws.
result<Value, Error> read(Source& s) {
  return jmpxx::catch_into_result<Error>(
      [&] { return throwing_parser(s); },          // may throw
      [] { return Error(parse_failed); });          // maps a foreign throw to an Error
}

// Handing a failure to code that expects a throw.
Value require(result<Value, Error> r) {
  return jmpxx::value_or_throw(std::move(r));        // throws error_exception<Error> on failure
}
```

A failure that crosses out as a throw and is caught at another such boundary round-trips
losslessly, because it travels inside `error_exception<E>`, which carries the error
unchanged. `error_exception<E>` derives from `std::exception`, so an upstream generic
handler still catches it.

## Converting a region

Inside the region you are converting, a function that threw becomes a function returning
`result<T, E>`: replace `throw E{...}` with `return jmpxx::fail(E{...})`, and replace the
implicit propagation a throw gave you with `JMPXX_TRY` at each call. The compiler then
enforces what exceptions left implicit: a failure cannot be dropped without a
compile-time diagnostic. Build the region with `-fno-exceptions` once it is converted, so
a stray throw is a build error rather than a latent path.

## What you gain and what you give up

You gain a deterministic, inspectable failure path: a failure returns as an ordinary
value with a cost you can read in the generated code, rather than an unwinder walk whose
tail you cannot bound. You give up the zero-cost-until-thrown happy path of exceptions.
On a non-inlined chain every frame that returns a `result` does a small amount of work an
exception skips on success. [comparison.md](../comparison.md) states that trade with
numbers. For code that wants the source-oblivious intermediate frames a throw gives,
without the exception model, the experimental [unwind arm](../reference/unwind.md) is
the closest approach, at a sad-path cost it measures.

## A bug is not a failure

Exceptions are sometimes used for programming errors as well as recoverable ones. jmpxx
is for recoverable, expected failures only. A precondition violation or a broken
invariant is a bug for a contract or a termination to handle, not a failure to propagate.
Route those to a checked termination, and reserve `result` for the failures a caller can
sensibly act on.
