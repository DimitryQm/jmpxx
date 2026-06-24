<!-- SPDX-License-Identifier: MIT -->
# Migrating from std::error_code

A codebase that signals failure with `std::error_code`, returned through an
out-parameter or paired with a value, adopts jmpxx without giving up the
`error_code` vocabulary, because the transport is generic over its error type.

## Carry the error_code verbatim

The simplest adoption keeps `std::error_code` as the error type and gains the value
pairing and the propagation construct. `result<T, std::error_code>` carries the code
unchanged, so its category identity and value are preserved exactly:

```cpp
#include <jmpxx/core.hpp>
#include <system_error>

result<Socket, std::error_code> open(const Endpoint& e) {
  std::error_code ec = try_open(e);
  if (ec) return jmpxx::fail(ec);   // the error_code travels verbatim
  return Socket(e);
}

result<Channel, std::error_code> connect(const Endpoint& e) {
  JMPXX_TRY(sock, open(e));         // forwards the error_code on failure
  return handshake(sock);
}
```

A function pairing a value with an out-parameter `error_code` becomes a function
returning `result<T, std::error_code>`, and the out-parameter and the
`if (ec) return;` after every call go away.

## Expose jmpxx errors to error_code-based code

Where the rest of the system speaks `std::error_code`, a `jmpxx::error` is presented as
one and recovered exactly:

```cpp
#include <jmpxx/interop/error_code.hpp>

std::error_code ec = jmpxx::to_error_code(jmpxx::error{code, domain});
jmpxx::error back = jmpxx::from_error_code(ec);   // equal to the original
```

`jmpxx::is_jmpxx(ec)` reports whether a code is in a jmpxx category, so a caller
distinguishes a jmpxx-originated code from a foreign one and branches on which it has.

## The identity caveat

A `std::error_category` is identified by the address of its singleton. Within one binary
two conversions of the same jmpxx error compare equal. Across a shared-library boundary
built with hidden visibility a category can be duplicated, the same hazard that affects
`std::error_category` and `typeid` across modules, so a program comparing jmpxx-derived
codes across such a boundary compares on the value and domain recovered through
`from_error_code` rather than on category identity. This bridge is hosted, because
`<system_error>` is not part of the freestanding subset. See
[interop.md](../reference/interop.md).
