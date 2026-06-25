<!-- SPDX-License-Identifier: MIT -->
# Interoperability bridges

jmpxx converts to and from the standard error types: `std::expected`,
`std::error_code`, and language exceptions. Each bridge lives in its own header under
`jmpxx/interop/`, and `jmpxx/interop.hpp` includes them all for a hosted consumer.
Each conversion preserves the information its entry below names and loses nothing
else.

## `std::expected` (`jmpxx/interop/expected.hpp`)
`jmpxx::to_expected(result<T, E>)` returns a `std::expected<T, E>`, and
`jmpxx::from_expected(std::expected<T, E>)` returns a `result<T, E>`. Both directions
are lossless: the conversion preserves which alternative is held and the contained
value or error of type `E` unchanged, so a value or failure crossing the bridge and
returning equals the original. Each direction has a const-reference and an
rvalue-reference overload, and both are usable in constant evaluation. `T` may be
`void`.

Availability: the bridge is defined when the standard library provides
`std::expected`, detected by `__cpp_lib_expected`, and `JMPXX_INTEROP_HAS_EXPECTED`
is 1 there and 0 otherwise. It does not require `__cpp_lib_freestanding_expected`,
which a current libstdc++ defines only from GCC 14 and libc++ does not define.

Freestanding: including the bridge adds no non-freestanding dependency to the
minimal core, because the bridge is a separate header that `jmpxx/core.hpp` never
pulls in. The conversions touch only the freestanding-clean surface of
`std::expected`; they never call `std::expected::value()`, the one member that can
throw and that a strict freestanding implementation deletes, so the bridge is usable
with exceptions disabled and in a freestanding build on a toolchain whose
`<expected>` compiles there.

## `std::error_code` (`jmpxx/interop/error_code.hpp`)
Hosted extension: `<system_error>` is not part of the freestanding subset. The bridge
serves two directions.

Adopt a foreign code. A `std::error_code` from another library is carried into jmpxx
without loss as a `result<T, std::error_code>`, because the transport is generic over
its error type. The `error_code` travels verbatim, so its category identity and value
are preserved exactly. `jmpxx::fail(ec)` builds the failure, and a
`std::expected<T, std::error_code>` bridges through `from_expected` into the same
`result<T, std::error_code>`. Use this lossless path for a code that originates
outside jmpxx and needs nothing from this header.

Expose a jmpxx error. `jmpxx::to_error_code(error)` presents a `jmpxx::error` to
error-code-based code as a `std::error_code` in a jmpxx-owned category, with the
error's code as the value and its domain selecting the category, so the pair
round-trips losslessly back through `jmpxx::from_error_code`. `jmpxx::make_error_code`
performs the same conversion under the conventional name. Both are explicit: jmpxx does
not specialize `std::is_error_code_enum`, so `std::error_code ec = err;` does not compile,
and a `jmpxx::error` is converted by calling `to_error_code` or `make_error_code`, which
keeps the change of form visible.
`jmpxx::error_category(domain)` returns the category for a domain, and
`jmpxx::is_jmpxx(ec)` reports whether a code is in a jmpxx category.
`from_error_code` recovers the code and domain exactly for a code in a jmpxx
category; for a foreign category it keeps the value and tags the generic domain,
which is lossy for the foreign category, so a program that must preserve a foreign
code carries it verbatim as `result<T, std::error_code>` rather than narrowing it.

Category identity caveat. A `std::error_category` is identified by the address of its
singleton. The jmpxx categories are held in one process-wide table, so within a
single binary two conversions of the same jmpxx error compare equal. Across a
shared-library boundary built with hidden visibility a category can be duplicated,
the same hazard that affects `std::error_category` and `typeid` across modules; a
program that compares jmpxx-derived error codes across such a boundary should compare
on the code and domain recovered through `from_error_code` rather than on category
identity.

## Language exceptions (`jmpxx/interop/exception.hpp`)
The opt-in boundary between a propagated failure and a thrown exception, for the point
where exception-free code must call into, or be called from, exception-based code.
`jmpxx::value_or_throw(result<T, E>)` returns the value on success and throws
`jmpxx::error_exception<E>` carrying the error on failure.
`jmpxx::catch_into_result<E>(callable, make_error)` runs the callable and returns a
`result`: a normal return becomes a success, a thrown `error_exception<E>` is
recovered losslessly as its error, and any other exception is mapped to an `E` by
`make_error`. The function never propagates an exception itself.
`error_exception<E>` derives from `std::exception`, so an upstream generic handler
still catches it.

Availability: the whole bridge exists only where exceptions are enabled, detected by
`__cpp_exceptions`, `__EXCEPTIONS`, or `_CPPUNWIND`, and
`JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE` is 1 there and 0 otherwise. Including the header
in a `-fno-exceptions` or freestanding build declares nothing and is not an error, so
a translation unit can include it unconditionally and query the macro.

## Optional-like adapters (`jmpxx/interop/adapt.hpp`)
`jmpxx::from_optional<E>(opt, on_empty)` adapts a value that is contextually
convertible to bool and dereferenceable with `operator*`, a `std::optional`, a raw
pointer, or a library's node accessor, into a `result`: the dereferenced value when
present, the failure `on_empty` produces when empty.
`jmpxx::from_condition<E>(present, take, on_absent)` is the more general form for a
library whose success flag and value are separate, taking the value from `take()`
when `present` is true and the failure from `on_absent()` otherwise. Both collapse
the recurring `if (!o) return fail(...); return *o;` boundary into one call. The
header duck-types its argument and pulls in nothing outside the freestanding subset,
so it is usable in the minimal core's configuration; it is still interop, selected by
a consumer that has an external value to adapt, and is not pulled in by
`jmpxx/core.hpp`.
